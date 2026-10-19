# Local integration server

This disposable Docker Compose stack runs the pinned Nakama server and
PostgreSQL database used by the BlitzMax integration tests. All published
ports bind to loopback and the test database is removed after each run.

From the `nakama.mod` repository, run:

```text
./test-server/run-integration.sh
```

The runner waits for both services to become healthy, builds the release-mode
BlitzMax tests, authenticates devices through `Nakama.SDK`, connects two
realtime sockets, matchmakes the clients into a relayed match, exchanges a
message, and then stops the services and removes the isolated database volume.

The Nakama console is available at `http://127.0.0.1:7351` while the stack is
running. The local defaults are suitable only for this test environment.

## macOS without Docker Desktop

[Colima](https://colima.run/) provides the Docker-compatible runtime without
Docker Desktop. Install and start it with:

```text
brew install colima docker docker-compose
colima start
```

The runner accepts either the `docker compose` plugin or the standalone
`docker-compose` command installed by Homebrew.
