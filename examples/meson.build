examples = [
  'build_toml',
  'conversions',
  'parse',
  'parse_stdin',
]

foreach example: examples
  executable(
    example,
    example + '.cc',
    dependencies: [cpptoml_dep],
  )
endforeach
