# Nakama modules for BlitzMax

This namespace provides a source-built integration of Heroic Labs' Nakama C++
client SDK for BlitzMax NG.

The normal bmk build does not run CMake, vcpkg, protoc, or a package manager.
All required generated code and third-party source is pinned in this
repository. The first foundational modules are:

- `Nakama.Protobuf`: the pinned protobuf 3.21.12 runtime used by the SDK.
- `Nakama.SDK`: Nakama C++ 2.9.0, RapidJSON, wslay, generated Nakama API code,
  and a small BlitzMax-safe C ABI. It reuses the existing `Net.libcurl` TLS
  stack.

The complete client stack builds from these local sources with BlitzMax's
MinGW x64 toolchain on Windows; no Nakama DLL or pre-built import library is
required.

Build and run the deterministic foundation test from the BlitzMax root:

```text
./bin/bmk makemods -a nakama.sdk
./bin/bmk makeapp -r -x mod/nakama.mod/sdk.mod/tests/test.bmx
```

With Docker available, run the disposable live-server integration suite from
the module repository:

```text
./test-server/run-integration.sh
```

See [`test-server/README.md`](test-server/README.md) for the pinned local
Nakama/PostgreSQL stack. The live suite covers device authentication, realtime
WebSocket connection, two-client matchmaking, relayed match join, and match
data exchange. Docker or Colima is needed only to run integration tests, not
to build or use the client modules.

Native macOS arm64 and Windows MinGW x64 builds and tests have been validated.
Linux, macOS x64, and TLS/WSS integration remain to be exercised before those
configurations are described as release-tested.

The vendored revisions and the GameNet implementation plan are recorded in
[`docs/feasibility.md`](docs/feasibility.md).
