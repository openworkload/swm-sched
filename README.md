
### Preparations after the repository cloning

1. The project depends on swm-core sources, thus create a sym link to swm-core in ./deps.
For example:

 Linux:
  $> ln -s ../../swm-core ./deps/

 Windows (as administrator):
  C:\..> mklink /D .\deps\swm-core ..\..\swm-core

2. Optionally, you can enable unit tests - just install GTest package.
Instructions are provided in the following section.

3. Generate compilation files by CMake tool. For example:

 Linux:
  $> cmake -G "Unix Makefiles"
  $> make

 Windows:
  C:\..> cmake -G "Visual Studio 15 2017 Win64"
  C:\..> .\swm-sched.sln


### Installing GTest (optional)

Before generating compilation files, perform the following actions:

1. Download and unzip GTest sources (https://github.com/google/googletest).
Set directory with GTest sources as current.

2. Generate GTest's compilation files by CMake tool:

 Linux:
  $> export GTEST_ROOT=/usr/local/GTest
  $> cmake . -G "Unix Makefiles"

 Windows:
  C:\..> cmake . -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX="C:/Program Files/GTest"

3. Compile and install GTest:

 Linux:
  $> make
  $> make install

 Windows (as administrator):
  C:\..> .\googletest-distribution.sln

  Switch to Release mode, change option "C/C++ -> Code Generation ->
  Runtime Library" from "/MT" to "/MD" both for gtest and for gtest-main
  projects. Build target ALL_BUILD, then target INSTALL.

  Switch to Debug mode. change option "C/C++ -> Code Generation ->
  Runtime Library" from "/MTd" to "/MDd" both for gtest and for gtest-main
  projects. Build taret ALL_BUILD only. Exit Visual Studio 2017.

  C:\..> copy ".\googlemock\gtest\Debug\gtest.lib"
              "C:\Program Files\GTest\lib\gtestd.lib" 
  C:\..> copy ".\googlemock\gtest\Debug\gtest_main.lib"
              "C:\Program Files\GTest\lib\gtest_maind.lib" 

4. Set up environment variable GTEST_ROOT as "/usr/local/GTest" (Linux) or
"C:\Program Files\GTest" (Windows)
