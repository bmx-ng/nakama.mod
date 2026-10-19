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

## Build evidence

- Upstream current source built successfully as a universal macOS arm64/x86_64
  SDK after raising its obsolete 10.15 deployment target for Xcode 27.
- A Windows x64 cross-build compiled protobuf, RapidJSON, curl/Schannel, wslay,
  all generated Nakama API sources, and all 35 selected Nakama translation
  units with MinGW GCC. The final link succeeded after placing `ws2_32` after
  static wslay. No MSVC object or C++ ABI is involved.
- `Nakama.Protobuf` builds under bmk in debug and release configurations.
- `Nakama.SDK` builds under bmk in debug and release configurations against
  the existing `Net.libcurl`/mbedTLS stack.
- The deterministic SDK test creates, ticks, and destroys a client without a
  server, and verifies the pinned version.

Windows execution and Linux native builds still require their native hosts or
CI before the module is advertised as released on those platforms. The MinGW
compile/link proof removes the main source-compatibility risk but does not
replace a WSS runtime test on Windows.

## GameNet mapping

### Direct fit

- `Open`: restore or receive a provider-specific Nakama session, connect the
  real-time socket, and expose the Nakama user id/username as `LocalPlayer`.
- `StartMatchmaking`: call `addMatchmaker`; retain the ticket by GameNet
  request id.
- `CancelOperation`: for matchmaking, call `removeMatchmaker`, suppress late
  success callbacks by generation, leave any match won by a cancel race, and
  finish with `MatchmakingCancelled`.
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
one terminal event, and undo native success that arrives after cancellation.
`CancelMatchmaking()` remains a compatibility wrapper around the same ticket
logic.

The service and a transferred match transport share one real-time socket. A
reference-counted dispatch context must own that socket so service polling,
transport polling, `Close()`, and `TakeMatchTransport()` cannot race or consume
each other's callbacks.

## Staged implementation

1. **Native foundation (started)**
   Build every dependency locally, expose only a C ABI, create/tick/destroy a
   client, and verify the pinned version without network access.
2. **Client/session wrapper (started)**
   Device authentication, session identity, realtime socket lifecycle, and
   error conversion are exposed and covered by the disposable local-server
   test. Add custom/email authentication, session restoration and refresh,
   main-thread event queues, and
   deterministic fake transport tests.
3. **Smallest useful GameNet slice**
   Implement `Open`, `Close`, `Pump`, identity, matchmaking, generic
   cancellation, matched-token join, `MatchReady`, and one-shot transport
   handoff. Initially advertise only Matchmaking and MatchTransport.
4. **Match transport**
   Add peer generations, presence events, a reserved wire envelope/opcode,
   directed sends, size limits, leave/disconnect behavior, and reconnection
   cleanup. Test cancellation-vs-match races and late callbacks deterministically.
5. **Integration smoke test (started)**
   The pinned Docker Compose harness now authenticates two BlitzMax clients,
   connects realtime sockets, matchmakes them, joins a relayed match, and
   exchanges an opcode-tagged message. Extend it to construct GameNet
   host/client sessions and exchange a GameNet user message through the adapter.
6. **Runtime-backed lobby support**
   Add a small TypeScript, Go, or Lua Nakama runtime package for create/list/
   join/start, labels, roster authority, invitations, and reconnect grace.
7. **Release matrix**
   Run native macOS and Linux tests plus native Windows MinGW compile/link/WSS
   tests. Record licenses and test both debug and release applications.

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
