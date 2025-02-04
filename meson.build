project('plume', ['c', 'cpp'],
  version : '0.1'
)

subdir('protocols')

fs = import('fs')

if not fs.exists('config.h')
  run_command('cp', 'config.def.h', 'config.h', check: true)
endif

cc = meson.get_compiler('c')
libm = cc.find_library('m')
xcb = dependency('xcb', required : get_option('xwayland'))
xlibs = dependency('xcb-icccm', required : get_option('xwayland'))
wayland_server_dep = dependency('wayland-server')
wlroots_dep = dependency('wlroots', version : ['>=0.17.0', '<0.18.0'])
xkbcommon_dep = dependency('xkbcommon')
libinput_dep = dependency('libinput')
libwayland_client_dep = dependency('wayland-client')

c_args = [
  '-g',
  '-Wno-unused-function',
  '-DWLR_USE_UNSTABLE',
  '-D_POSIX_C_SOURCE=200809L',
  '-DVERSION="@0@"'.format(meson.project_version())
]

if xcb.found() and xlibs.found()
  c_args += '-DXWAYLAND'
  c_args += '-DIM'
 endif

executable('plume',
  'main.c',
  'util.c',
  'wlr_foreign_toplevel_management_v1.c',
  'wlr_foreign_toplevel_management_v1.h',
  wayland_sources,
  dependencies : [
    libm,
    xcb,
    xlibs,
    wayland_server_dep,
    wlroots_dep,
    xkbcommon_dep,
    libinput_dep,
    libwayland_client_dep,
  ],
  install : true,
  c_args : c_args
)

prefix = get_option('prefix')
desktop_install_dir = join_paths(prefix, 'share/wayland-sessions')
install_data('plume.desktop', install_dir : desktop_install_dir)
