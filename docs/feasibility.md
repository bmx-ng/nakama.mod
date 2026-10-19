# Nakama/GameNet feasibility and implementation plan

## Decision

A source-only BlitzMax integration is viable. The validated transport choice
for desktop platforms is Nakama's curl HTTP adapter plus wslay-over-curl
WebSocket adapter. This avoids the upstream Windows preset's dependency on
Visual Studio, the Windows SDK build tools, and Microsoft's `libHttpClient`.

The normal module build must remain deterministic and offline. Upstream CMake
downloads moving `master` archives and generates API code during configure, so
the integration checks generated C++ into source control instead.

## Pinned inputs

- `nakama-cpp`: `1d688b5decbd7e199a5a87815e228a0c12c4d85c`
  (`2.9.0`, Apache-2.0).
- `nakama-common`: `189a8ea03d9038126437666745a9d5f74ee21c30`
  (source for the checked-in `api.pb.*` and `realtime.pb.*`).
- `nakama`: `7c94b8f3aa3a7550444a0c2cffee0969223f1ed9`
  (recorded because upstream downloaded it during generation; gRPC is disabled
  and the present client build does not compile its generated sources).
- protobuf `3.21.12`, port revision 4 (BSD-3-Clause).
- RapidJSON `a95e013b97ca6523f32da23f5095fcc9dd6067e5`
  (MIT).
- wslay `1.1.1` (MIT).
- HTTP/TLS and zlib are supplied by existing BlitzMax modules.

## Build and test evidence

- Upstream current source built successfully as a universal macOS arm64/x86_64
  SDK after raising its obsolete 10.15 deployment target for Xcode 27.
- Native macOS arm64 and Windows x64 MinGW builds compile `Nakama.Protobuf` and
  `Nakama.SDK` under bmk in debug and release configurations against the
  existing `Net.libcurl`/mbedTLS stack. No MSVC object, Nakama DLL, or import
  library is involved.
- The deterministic SDK test creates, ticks, and destroys a client without a
  server, and verifies the pinned version.
- The disposable local server tests device authentication, two-client realtime
  matchmaking, relayed match join, and match data exchange on macOS and native
  Windows.
- The GameNet integration test completes matchmaking, transport handoff, a
  `TNetSession` host/client handshake, and application message exchange. It
  also checks cancellation and incompatible matchmaking-property isolation.

Linux native builds, macOS x64 execution, and WSS certificate handling have not
yet been validated. The current live suite uses loopback HTTP and WebSocket
connections rather than TLS.

## GameNet mapping

### Direct fit

- `Open`: authenticate a device, connect the real-time socket, and expose the
  Nakama user id/username as `LocalPlayer`.
- `StartMatchmaking`: call `addMatchmaker`; retain the ticket by GameNet
  request id.
- `CancelOperation`: for matchmaking, call `removeMatchmaker`, including when
  cancellation is requested before the ticket callback, and finish with
  `MatchmakingCancelled`.
- Match transport: map match presence joins/leaves to peer events and match
  data to GameNet packets. Nakama can direct a message to selected presences.
- Pumping: call the Nakama client and realtime-client `tick()` functions from
  the GameNet service pump. Callbacks execute on that thread.

Nakama's WebSocket is reliable and ordered. The GameNet envelope must retain
the requested delivery label because `GameNet.Session` distinguishes packet
classes, but it cannot honestly provide UDP-like unreliable delivery. This can
increase latency through head-of-line blocking.

### Requires a small Nakama server runtime

- Stable lobby creation, owner, visibility, joinability, capacity, properties,
  listing, and transition from lobby to game.
- Secure and deterministic host/authority selection shared by all clients.
- Invitation validation, expiry, and access control. Nakama notifications or a
  chat payload can carry an invitation, but there is no Steam/GameKit overlay.
- Reliable reconnect reservations and roster restoration when a realtime
  session id changes.
- Kicking a remote peer and authoritative membership decisions.
- Server-authoritative gameplay, if desired, requires an actual Nakama match
  handler rather than merely relaying packets.

Raw relayed matches do not have a durable lobby owner, searchable lobby
metadata, or a native "start match" transition. Those capabilities must not be
advertised until the runtime implements them.

### No clean common equivalent

- Platform overlay invitations and platform-native friends.
- Host migration as understood by a client-hosted Steam/EOS/GameKit match.
- A dedicated-server network endpoint representable by the current
  `GameNet.SMatchInfo`, which assumes a client host peer.
- Parties, skill queries, latency preferences, session refresh, and resume are
  useful Nakama features but are not represented by the present common
  `IOnlineService` contract.

## Cancellation and lifecycle rules

`GameNet.OnlineFlow` uses the generic `CancelOperation()` path. The adapter must
keep the original operation alive until cancellation is settled, emit exactly
one terminal event, and defer removal when cancellation precedes the Nakama
ticket callback. Once Nakama has begun joining a matched result, cancellation
is rejected rather than pretending that the match was cancelled.
`CancelMatchmaking()` remains a compatibility wrapper around the same logic.

The service and a transferred match transport share one real-time socket. A
reference-counted dispatch context must own that socket so service polling,
transport polling, `Close()`, and `TakeMatchTransport()` cannot race or consume
each other's callbacks.

## Implementation status

1. **Native foundation — complete for the tested desktop targets.**
   Every client dependency builds locally, the BlitzMax surface crosses a C
   ABI, and no build-time package manager or code generator is required.
2. **Initial client/session wrapper — complete.**
   Device authentication, session identity, realtime socket lifecycle, error
   conversion, bounded event queues, and binary match data are exposed.
   Custom/email authentication, session restoration, and refresh are deferred.
3. **Smallest useful GameNet slice — complete.**
   `Open`, `Close`, `Pump`, identity, matchmaking, generic cancellation,
   matched-token join, `MatchReady`, and one-shot transport handoff advertise
   only the implemented Matchmaking and MatchTransport capabilities.
4. **Match transport — complete for non-restoring matches.**
   The adapter includes peer generations, presence events, a reserved wire
   envelope/opcode, directed sends, size limits, leave/disconnect behavior,
   bounded queues, and deterministic backend tests. Unexpected socket loss is
   terminal; transparent reconnection and roster restoration are deferred.
5. **Integration suite — complete for the initial slice.**
   The Docker Compose harness exercises both the SDK and GameNet paths through
   a complete two-client `TNetSession` exchange.
6. **Runtime-backed lobby support — deferred.**
   Add a small TypeScript, Go, or Lua Nakama runtime package for create/list/
   join/start, labels, roster authority, invitations, and reconnect grace.
7. **Release hardening — remaining.**
   Add repeatable clean-machine automation, validate Linux and macOS x64, and
   exercise WSS certificate handling. Continue testing debug and release module
   builds on macOS arm64 and Windows MinGW x64.

## Main risks

- The SDK's generated protobuf API and protobuf runtime must be updated as one
  pinned unit.
- The existing libcurl module uses mbedTLS on all platforms, whereas the
  upstream proof used Secure Transport on macOS and Schannel on Windows. WSS
  certificate behavior must be exercised on every platform.
- Full protobuf makes clean builds large and slow, intentionally accepted in
  favour of an offline, source-only build.
- Upstream does not support or test MinGW. The portable transport patch and
  link ordering are maintained here and must remain in CI.
- Nakama match presence session ids change on reconnect; peer handles require
  generations to prevent stale packets or accidental identity reuse.
