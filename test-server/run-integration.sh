#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
module_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
blitzmax_dir=$(CDPATH= cd -- "$module_dir/../.." && pwd)
compose_project=nakama_blitzmax_integration

if docker compose version >/dev/null 2>&1; then
    compose() {
        docker compose "$@"
    }
elif command -v docker-compose >/dev/null 2>&1; then
    compose() {
        docker-compose "$@"
    }
else
    echo "Docker Compose is required (docker compose or docker-compose)." >&2
    exit 1
fi

cleanup() {
    compose --project-name "$compose_project" --file "$script_dir/docker-compose.yml" down --volumes --remove-orphans
}
trap cleanup EXIT INT TERM

if ! compose --project-name "$compose_project" --file "$script_dir/docker-compose.yml" up --detach --wait; then
    compose --project-name "$compose_project" --file "$script_dir/docker-compose.yml" logs --no-color
    exit 1
fi

run_test() {
    expected=$1
    shift

    if output=$("$@" 2>&1); then
        status=0
    else
        status=$?
    fi
    printf '%s\n' "$output"
    [ "$status" -eq 0 ] || return "$status"

    case "$output" in
        *"$expected"*) ;;
        *)
            echo "Expected test success marker not found: $expected" >&2
            return 1
            ;;
    esac
}

run_test "Nakama live device authentication passed" \
    "$blitzmax_dir/bin/bmk" makeapp -r -x "$module_dir/sdk.mod/tests/integration_auth.bmx"
run_test "Nakama two-client realtime matchmaking and data exchange passed" \
    "$blitzmax_dir/bin/bmk" makeapp -r -x "$module_dir/sdk.mod/tests/integration_match.bmx"
run_test "GameNet.Nakama tests passed" \
    "$blitzmax_dir/bin/bmk" makeapp -a -r -x -o /tmp/gamenet_nakama_unit \
    "$blitzmax_dir/mod/gamenet.mod/nakama.mod/tests/test.bmx"
run_test "GameNet.Nakama matchmaking, handoff, session handshake, and message exchange passed" \
    "$blitzmax_dir/bin/bmk" makeapp -a -r -x -o /tmp/gamenet_nakama_integration \
    "$blitzmax_dir/mod/gamenet.mod/nakama.mod/tests/integration.bmx"
