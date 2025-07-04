project('asimple', 'c',
  default_options: [
    'b_lto=true',
    'b_staticpic=false',
    'c_std=c2x',
    'warning_level=3',
  ],
  license: ['Apache-2.0'],
  version: '0.1.0')

boards = [
  { 'name': 'asimple_rba_atp', 'dependency': 'ambiq_rba_atp'},
  { 'name': 'asimple_rba', 'dependency': 'ambiq_rba'},
]

c_args = [
  '-ffunction-sections',
]

link_args = [
  '-Wl,--gc-sections', '-fno-exceptions',
]

lfs_lib = dependency('lfs')

asimple_libs = {}

# Libraries per board
foreach board : boards
  ambiq_lib = dependency(board['dependency'])

  lib_sources = files([
    'src/uart.c',
    'src/adc.c',
    'src/spi.c',
    'src/lora.c',
    'src/gpio.c',
    'src/am1815.c',
    'src/syscalls.c',
    'src/flash.c',
    'src/bmp280.c',
    'src/bme280.c',
    'src/power_control.c',
    'src/pdm.c',
    'src/asimple_littlefs.c',
    'src/syscalls.c',
    'src/syscalls/uart.c',
    'src/syscalls/rtc.c',
    'src/syscalls/littlefs.c',
    'src/systick.c',
    'src/cli.c',
    'src/sd_card.c',
  ])

  includes = include_directories([
    'include/asimple',
  ])

  lib = library(board['name'],
    lib_sources,
    dependencies: [ambiq_lib, lfs_lib],
    include_directories: includes,
    c_args: c_args,
    link_args: link_args,
    install: true,
  )

  asimple_libs += {board['name']: lib}

  install_subdir('include', install_dir: '')

  # Create a pkgconfig file
  pkg = import('pkgconfig')
  pkg.generate(lib, subdirs: ['', 'asimple'])
endforeach

# Executables -- FIXME this is here more as an example for now...
sources = files([
  'src/main.c',
])

# Find libm...
cc = meson.get_compiler('c', native: false)
m_dep = cc.find_library('m', required : false)

ambiq_lib = dependency(boards[0]['dependency'])

exe = executable(meson.project_name(),
  sources,
  dependencies: [ambiq_lib, lfs_lib, m_dep],
  link_with: asimple_libs['asimple_rba_atp'],
  include_directories: includes,
  c_args: c_args,
  link_args: link_args + ['-T' + meson.project_source_root() / 'linker.ld']
)

# Custom flash target, to make it easier to program the RedBoard, requires
# objcopy
objcopy = find_program('objcopy')

bin = custom_target(
  input : exe,
  output : exe.name().split('.')[0] + '.bin',
  command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@', ],
  build_by_default: true
)

run_target('flash',
  command : ['python3', meson.project_source_root() / 'svl.py',
    get_option('tty'), '-f',  bin, '-b', '921600', '-v'],
  depends : bin,
)
