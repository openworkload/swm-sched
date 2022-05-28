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
* gcc with C++17 support or Visual Studio 2017
* cmake version >= 2.8

### Preparations after the repository cloning

1. The project depends on swm-core sources, thus create a sym link to swm-core in ./deps.
For example:
* Linux:
```bash
ln -s ../../swm-core ./deps/
```
* Windows (as administrator):
```bash
mklink /D .\deps\swm-core ..\..\swm-core
```
2. Optionally, you can enable unit tests - just install GTest package.
Instructions are provided in the following section.
3. Generate compilation files by CMake tool. For example:
* Linux:
```bash
cmake -G "Unix Makefiles"
make
```
* Windows:
```bash
cmake -G "Visual Studio 15 2017 Win64"
.\swm-sched.sln
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

* Windows:
```bash
cmake . -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX="C:/Program Files/GTest"
```
3. Compile and install GTest:
* Linux:
```bash
make
make install
```
* Windows (as administrator):
```bash
.\googletest-distribution.sln
```
`Release` mode: change option `C/C++ -> Code Generation -> Runtime Library` from `/MT` to `/MD` both for gtest and for gtest-main projects. Build target `ALL_BUILD`, then target `INSTALL`.
`Debug` mode: change option `C/C++ -> Code Generation -> Runtime Library` from `/MTd` to `/MDd` both for gtest and for gtest-main projects. Build taret `ALL_BUILD` only. Exit Visual Studio.
```bash
copy ".\googlemock\gtest\Debug\gtest.lib" "C:\Program Files\GTest\lib\gtestd.lib"
copy ".\googlemock\gtest\Debug\gtest_main.lib" "C:\Program Files\GTest\lib\gtest_maind.lib"
```
4. Set up environment variable GTEST_ROOT as `/usr/local/GTest` (Linux) or `C:\Program Files\GTest` (Windows).


## Contributing

We appreciate all contributions. If you are planning to contribute back bug-fixes, please do so without any further discussion. If you plan to contribute new features, utility functions or extensions, please first open an issue and discuss the feature with us.


## License

We use a shared copyright model that enables all contributors to maintain the copyright on their contributions.

This software is licensed under the BSD-3-Clause license. See the [LICENSE](LICENSE) file for details.
