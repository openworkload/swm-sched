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
* cmake version >= 2.8

### Preparations after the repository cloning

1. The project depends on swm-core sources, thus create a sym link to swm-core in ./deps:
```bash
cd deps
ln -s ../../swm-core .
```

2. Optionally, you can enable unit tests - just install GTest package.
Instructions are provided in the following section.

3. Generate compilation files by CMake tool:
```bash
cmake -G "Unix Makefiles"
make
```

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

### Run unit tests
```bash
./bin/swm-sched-tests
```

### Run github actions
```bash
act -j unittests
```

## Contributing

We appreciate all contributions. If you are planning to contribute back bug-fixes, please do so without any further discussion. If you plan to contribute new features, utility functions or extensions, please first open an issue and discuss the feature with us.


## License

We use a shared copyright model that enables all contributors to maintain the copyright on their contributions.

This software is licensed under the BSD-3-Clause license. See the [LICENSE](LICENSE) file for details.
