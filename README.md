<p align="center">
    <a href="https://github.com/openworkload/swm-sched/blob/master/LICENSE" alt="License">
        <img src="https://img.shields.io/github/license/openworkload/swm-sched" />
    </a>
    <a href="https://github.com/openworkload/swm-sched" alt="Required C++ version">
        <img src="https://img.shields.io/badge/C++-17-blue.svg" />
    </a>
    <a href="https://github.com/openworkload/swm-sched/actions/workflows/unittests-linux.yml" alt="Test results">
        <img src="https://github.com/openworkload/swm-sched/actions/workflows/unittests-linux.yml/badge.svg?event=push" />
    </a>
</p>

Sky Port scheduler
==================


## Description

[This](https://github.com/openworkload/swm-sched) is a scheduler for Open Workload project,
which core daemon can be found [here](https://github.com/openworkload/swm-core).
Sky Port is an universal bus between user software and compute resources.
It can also be considered as a transportation layer between workload producers and compute resource providers.
Sky Port makes it easy to connect user software to different cloud resources.
Jobs submitted to Sky Port are scheduled by the daemon represented by this repository.


## Build

## Requirements:
* gcc with C++17 support
* cmake version >= 3.16
* Erlang/OTP with `ei` (set `_KERL_ACTIVE_DIR` to the Erlang root that contains `usr/include` and `usr/lib/libei.a`)
* Sibling checkout of [swm-core](https://github.com/openworkload/swm-core) at `../swm-core` (used via `deps/swm-core`)
* `clang-format-14` (for `make format` / CI format check; other major versions can reshuffle whitespace)

### Installing GTest (optional)

Before generating compilation files, perform the following actions:

1. Download and unzip GTest sources (https://github.com/google/googletest).
Set directory with GTest sources as current.

2. Generate GTest's compilation files by CMake tool:
* Linux:
```bash
export GTEST_ROOT=/usr/local/GTest
cmake . -G "Unix Makefiles"
```

3. Compile and install GTest:
```bash
make
make install
```

4. Set up environment variable GTEST_ROOT as `/usr/local/GTest`.

### Compile binaries

Optionally, enable unit tests by installing GTest (see the previous section).
If `GTEST_ROOT` points at an installed GTest, CMake enables the unit tests automatically.

```bash
./build.sh
```

`build.sh` links `deps/swm-core` to `../swm-core` when needed, then runs `cmake` and `make`.
Defaults (`_KERL_ACTIVE_DIR=/usr/erlang`, `GTEST_ROOT=/usr/local/GTest`,
`CMAKE_BUILD_TYPE=RelWithDebInfo`) match the `skyport-dev` container; override
them in the environment for other setups.

Build hardening flags, plugin trust rules, and fuzz instructions are in
[HOWTO/SECURITY.md](HOWTO/SECURITY.md).

### Run unit tests
```bash
./bin/swm-sched-tests
```

### Fuzz smoke (optional, Clang + libFuzzer)
```bash
cmake . -DSWM_ENABLE_FUZZ=ON -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang
make fuzz-smoke
```

### Format C++ sources
Style rules live in [STYLE.md](STYLE.md) and are enforced by [`.clang-format`](.clang-format).

```bash
make format         # rewrite src/ and tests/ in place
make format-check   # fail if any file differs from .clang-format
```

Or without CMake: `./scripts/format-cpp.sh` / `./scripts/format-cpp.sh check`.

### Run github actions
[`scripts/run-act.sh`](scripts/run-act.sh) runs [nektos/act](https://github.com/nektos/act)
against the host **Podman** socket. From the host it starts `skyport-dev` if
needed (sibling `../swm-core` `run-in-dev-container.sh`, same stack as
`make cr`), then runs act inside that container. Nested CI containers still
use the bind-mounted host Podman API.

```bash
# Host (no local cmake): script starts/enters skyport-dev
./scripts/run-act.sh --job unittests --rm
./scripts/run-act.sh --job fuzz_smoke --rm

# Inside skyport-dev only (CMake Makefile needs cmake on PATH)
make act ARGS='--job unittests --rm'
```

Requires a Podman socket (`systemctl --user enable --now podman.socket`) and a
sibling [swm-core](https://github.com/openworkload/swm-core) checkout. Repo
[`.actrc`](.actrc) maps `ubuntu-24.04` to `catthehacker/ubuntu:act-24.04`
(includes Node for `setup-beam`).


## Contributing

We appreciate all contributions. If you are planning to contribute back bug-fixes, please do so without any further discussion. If you plan to contribute new features, utility functions or extensions, please first open an issue and discuss the feature with us.


## License

We use a shared copyright model that enables all contributors to maintain the copyright on their contributions.

This software is licensed under the BSD-3-Clause license. See the [LICENSE](LICENSE) file for details.
