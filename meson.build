project('redboard_spi_experiment', 'c',
  default_options: ['b_lto=false', 'b_staticpic=false'],
  license: ['Apache-2.0'],
  version: '0.1.0')

rba_lib = dependency('rba_atp')

c_args = [
  '-DAM_CUSTOM_BDADDR', '-DAM_PACKAGE_BGA', '-DWSF_TRACE_ENABLED',
  '-DWSF_TRACE_ENABLED', '-DWSF_TRACE_ENABLED', '-DAM_PART_APOLLO3',
  '-DPART_APOLLO3', '-mthumb', '-mcpu=cortex-m4', '-mfpu=fpv4-sp-d16',
  '-mfloat-abi=hard', '-std=c17', '-ffunction-sections', '-Wall', '-Wextra',
  '-pedantic', '-fno-delete-null-pointer-checks',
]

link_args = [
  '--specs=nosys.specs', '--specs=nano.specs', '-mthumb',
  '-mcpu=cortex-m4', '-mfpu=fpv4-sp-d16', '-mfloat-abi=hard', '-nostartfiles',
  '-static', '-Wl,--gc-sections', '-fno-exceptions',
  '-fno-delete-null-pointer-checks',
]

sources = files([
  'src/main.c',
])

lib_sources = files([
  'src/uart.c',
  'src/adc.c',
  'src/spi.c',
  'src/lora.c',
])

includes = include_directories([
  'include/rba_simple',
])

lib = library('rba_simple',
  lib_sources,
  dependencies: [rba_lib],
  include_directories: includes,
  c_args: c_args,
  link_args: link_args,
  install: true,
)

install_subdir('include', install_dir: 'include/')

exe = executable(meson.project_name(),
  sources,
  dependencies: [rba_lib],
  link_with: lib,
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

pkg = import('pkgconfig')

pkg.generate(lib, subdirs: ['rba_simple'])
