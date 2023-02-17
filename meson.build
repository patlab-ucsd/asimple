project('redboard_adc_experiment', 'c',
  default_options: ['b_lto=false', 'b_staticpic=false'],
  license: ['Apache-2.0'],
  version: '0.1.0')

lib = dependency('rba_atp')

c_args = [
  '-DAM_CUSTOM_BDADDR', '-DAM_PACKAGE_BGA', '-DWSF_TRACE_ENABLED',
  '-DWSF_TRACE_ENABLED', '-DWSF_TRACE_ENABLED', '-DAM_PART_APOLLO3',
  '-DPART_APOLLO3', '-mthumb', '-mcpu=cortex-m4', '-mfpu=fpv4-sp-d16',
  '-mfloat-abi=hard', '-std=c17', '-ffunction-sections', '-Wall', '-Wextra',
  '-pedantic',
]

link_args = [
  '--specs=nosys.specs', '--specs=nano.specs', '-mthumb',
  '-mcpu=cortex-m4', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard', '-nostartfiles',
  '-static', '-Wl,--gc-sections', '-fno-exceptions'
]

sources = files([
  'src/main.c',
  'src/uart.c',
  'src/adc.c',
])

includes = include_directories([
  'include',
])

exe = executable(meson.project_name(),
  sources,
  dependencies: [lib],
  include_directories: includes,
  c_args: c_args,
  link_args: link_args + ['-T' + meson.source_root() / 'linker.ld']
)

objcopy = find_program('objcopy')

bin = custom_target(meson.project_name() + '.bin',
  input : exe,
  output : meson.project_name() + '.bin',
  command : [objcopy, '-O', 'binary',
    '@INPUT@', '@OUTPUT@',
  ],
  build_by_default: true
)

run_target('flash',
  command : ['python3', meson.source_root() / 'svl.py',
    get_option('tty'), '-f',  meson.project_name() + '.bin',
    '-b', '921600', '-v'],
  depends : bin,
)
