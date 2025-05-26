# cpptoml-ng

A fork of [cpptoml](https://github.com/skystrife/cpptoml).

## Features

* Not header-only: Compile the parser and serializer once,
  rather than for each source file you use cpptoml-ng in
* CMake is replaced with meson, and cpptoml-ng works as a meson subproject
* Build issues due to missing includes are fixed
* The custom cpptoml::option is replaced with std::optional
* Deprecated stuff is removed
* The source code is simplified a bit, by e.g removing the ability to use std::map
  as a table representation

## Compile time benchmarks

Here are some benchmarks which show compile times under different circumstances.
Benchmarking system: Fedora Linux 22 running on an AMD Ryzen 7 9800x3D

A source file which parses with `cpptoml::parse_file` and serializes
with `std::cout <<`:

```
Benchmark 1: clang++ -std=c++20 -c -o parse-serialize.cc.o parse-serialize.cc -Icpptoml/include
  Time (mean ± σ):     555.2 ms ±   1.4 ms    [User: 528.9 ms, System: 22.4 ms]
  Range (min … max):   553.1 ms … 558.4 ms    10 runs

Benchmark 2: clang++ -std=c++20 -c -o parse-serialize.cc.o parse-serialize.cc -Icpptoml-ng/include
  Time (mean ± σ):     344.8 ms ±   1.3 ms    [User: 325.6 ms, System: 16.9 ms]
  Range (min … max):   342.9 ms … 346.7 ms    10 runs

Summary
  clang++ -std=c++20 -c -o parse-serialize.cc.o parse-serialize.cc -Icpptoml-ng/include ran
    1.61 ± 0.01 times faster than clang++ -std=c++20 -c -o parse-serialize.cc.o parse-serialize.cc -Icpptoml/include
```

A source file only parses with `cpptoml::parse_file`:

```
Benchmark 1: clang++ -std=c++20 -c -o parse.cc.o parse.cc -Icpptoml/include
  Time (mean ± σ):     536.7 ms ±   3.3 ms    [User: 511.9 ms, System: 21.2 ms]
  Range (min … max):   532.6 ms … 542.1 ms    10 runs

Benchmark 2: clang++ -std=c++20 -c -o parse.cc.o parse.cc -Icpptoml-ng/include
  Time (mean ± σ):     323.6 ms ±   0.7 ms    [User: 306.8 ms, System: 14.7 ms]
  Range (min … max):   322.6 ms … 324.7 ms    10 runs

Summary
  clang++ -std=c++20 -c -o parse.cc.o parse.cc -Icpptoml-ng/include ran
    1.66 ± 0.01 times faster than clang++ -std=c++20 -c -o parse.cc.o parse.cc -Icpptoml/include
```

A source file which only includes the header:

```
Benchmark 1: clang++ -std=c++20 -c -o include-only.cc.o include-only.cc -Icpptoml/include
  Time (mean ± σ):     473.8 ms ±   2.5 ms    [User: 453.1 ms, System: 17.8 ms]
  Range (min … max):   470.3 ms … 477.8 ms    10 runs

Benchmark 2: clang++ -std=c++20 -c -o include-only.cc.o include-only.cc -Icpptoml-ng/include
  Time (mean ± σ):     326.6 ms ±   1.1 ms    [User: 307.3 ms, System: 17.2 ms]
  Range (min … max):   324.7 ms … 327.7 ms    10 runs

Summary
  clang++ -std=c++20 -c -o include-only.cc.o include-only.cc -Icpptoml-ng/include ran
    1.45 ± 0.01 times faster than clang++ -std=c++20 -c -o include-only.cc.o include-only.cc -Icpptoml/include
```
