#!/usr/bin/env bash
#
# SPDX-FileCopyrightText: © 2021 Taras Shapovalov
# SPDX-License-Identifier: BSD-3-Clause
#
# Run nektos/act against the host Podman Docker-compatible API (not Docker Engine).
# Repo .actrc supplies runner image defaults.
#
# When invoked on the host, ensure skyport-dev is up (via sibling swm-core
# run-in-dev-container.sh / make cr stack), then re-exec inside that container
# so cmake/act/OTP match the dev image. Nested act containers still use the
# host Podman socket bind-mounted into skyport-dev.
#
# Usage:
#   scripts/run-act.sh --job unittests --rm
#   make act ARGS='--job unittests --rm'   # from inside skyport-dev
#

set -euo pipefail

ME=$(readlink -f "$0")
ROOT_DIR=$(dirname "$(dirname "$ME")")
CORE_NAME="${SWM_ACT_CONTAINER:-skyport-dev}"
HOST_USER="${USER:-$(id -un)}"
SWM_CORE_DIR="${SWM_CORE_DIR:-${ROOT_DIR}/../swm-core}"
ENSURE_SCRIPT="${SWM_CORE_DIR}/scripts/run-in-dev-container.sh"

XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
if [[ -n "${SWM_ACT_PODMAN_SOCK:-}" && -S "${SWM_ACT_PODMAN_SOCK}" ]]; then
    PODMAN_SOCK="${SWM_ACT_PODMAN_SOCK}"
elif [[ -n "${SWM_CONTAINER_PODMAN_SOCK:-}" && -S "${SWM_CONTAINER_PODMAN_SOCK}" ]]; then
    PODMAN_SOCK="${SWM_CONTAINER_PODMAN_SOCK}"
else
    PODMAN_SOCK="${XDG_RUNTIME_DIR}/podman/podman.sock"
fi

swm_act_in_dev_container() {
    [[ -f /run/.containerenv ]] || [[ -f /.dockerenv ]]
}

# From the host: ensure skyport-dev, then run this script inside it.
if [[ "${SWM_ACT_SKIP_CONTAINER_REEXEC:-0}" != "1" ]] && ! swm_act_in_dev_container; then
    if ! command -v podman >/dev/null 2>&1; then
        echo "ERROR: podman not found; cannot start/enter ${CORE_NAME}" >&2
        exit 1
    fi
    if [[ ! -S "${PODMAN_SOCK}" ]]; then
        echo "ERROR: Podman API socket not found at ${PODMAN_SOCK}" >&2
        echo "Start it with: systemctl --user enable --now podman.socket" >&2
        exit 1
    fi
    if [[ ! -x "${ENSURE_SCRIPT}" ]]; then
        echo "ERROR: missing ${ENSURE_SCRIPT}" >&2
        echo "Expected sibling checkout at ${SWM_CORE_DIR} (override with SWM_CORE_DIR)." >&2
        exit 1
    fi

    quoted_args=""
    for a in "$@"; do
        quoted_args+=" $(printf '%q' "${a}")"
    done

    # run-in-dev-container.sh creates/starts the pod + skyport-dev if needed.
    echo "run-act: ensure ${CORE_NAME}, then re-exec as ${HOST_USER}" >&2
    exec "${ENSURE_SCRIPT}" \
        "cd $(printf '%q' "${ROOT_DIR}") && \
         export SWM_ACT_SKIP_CONTAINER_REEXEC=1 \
                XDG_RUNTIME_DIR=$(printf '%q' "${XDG_RUNTIME_DIR}") \
                SWM_CONTAINER_PODMAN_SOCK=$(printf '%q' "${PODMAN_SOCK}") \
                SWM_ACT_PODMAN_SOCK=$(printf '%q' "${PODMAN_SOCK}") && \
         exec $(printf '%q' "${ME}")${quoted_args}"
fi

if [[ ! -S "${PODMAN_SOCK}" ]]; then
    echo "ERROR: Podman API socket not found at ${PODMAN_SOCK}" >&2
    echo "Start it with: systemctl --user enable --now podman.socket" >&2
    echo "Or: podman system service --time=0 unix://${PODMAN_SOCK}" >&2
    echo "skyport-dev must have the host socket bind-mounted (make cr)." >&2
    exit 1
fi

export DOCKER_HOST="unix://${PODMAN_SOCK}"
export XDG_RUNTIME_DIR

exec act \
    --container-daemon-socket "${DOCKER_HOST}" \
    --concurrent-jobs 1 \
    "$@"
