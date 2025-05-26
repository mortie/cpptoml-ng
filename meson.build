project(
  'cpptoml', 'cpp',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
  ],
)

cpptoml_lib = library(
  'cpptoml',
  'src/misc.cc',
  'src/parser.cc',
  'src/writer.cc',
  include_directories: 'include',
)

cpptoml_dep = declare_dependency(
  include_directories: 'include',
  link_with: cpptoml_lib,
)

subdir('examples')
