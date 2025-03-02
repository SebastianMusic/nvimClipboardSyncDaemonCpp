project(
  'nvimClipboardSync',
  'cpp',
  version: '0.1.0',
  default_options: ['cpp_std=c++17'],
)

libuv_dep = dependency('libuv', fallback: 'libuv')
spdlog_dep = dependency('spdlog', fallback: 'spdlog')
rapidjson_dep = declare_dependency(include_directories: 'subprojects/rapidjson')

executable(
  'nvimClipboardSync',
  'src/main.cpp',
  dependencies: [libuv_dep, spdlog_dep, rapidjson_dep],
)
