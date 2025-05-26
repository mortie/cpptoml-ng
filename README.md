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
