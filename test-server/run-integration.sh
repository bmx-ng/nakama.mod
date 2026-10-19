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
"$blitzmax_dir/bin/bmk" makeapp -r -x "$module_dir/sdk.mod/tests/integration_auth.bmx"
"$blitzmax_dir/bin/bmk" makeapp -r -x "$module_dir/sdk.mod/tests/integration_match.bmx"
"$blitzmax_dir/bin/bmk" makeapp -r -x -o /tmp/gamenet_nakama_unit "$blitzmax_dir/mod/gamenet.mod/nakama.mod/tests/test.bmx"
"$blitzmax_dir/bin/bmk" makeapp -r -x -o /tmp/gamenet_nakama_integration "$blitzmax_dir/mod/gamenet.mod/nakama.mod/tests/integration.bmx"
