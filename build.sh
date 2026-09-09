#!/bin/bash
set -eo pipefail

cd "$(dirname "$0")"

if [ ! -e deps/swm-core ]; then
    if [ ! -d ../swm-core ]; then
        echo "Error: expected sibling checkout at ../swm-core (creates deps/swm-core symlink)" >&2
        exit 1
    fi
    ln -s ../../swm-core deps/swm-core
fi

if [ -f /usr/erlang/activate ]; then
    # shellcheck disable=SC1091
    source /usr/erlang/activate
fi
export _KERL_ACTIVE_DIR="${_KERL_ACTIVE_DIR:-/usr/erlang}"
export GTEST_ROOT="${GTEST_ROOT:-/usr/local/GTest}"

cmake . -G "Unix Makefiles"
make
