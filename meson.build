project('maomao', ['c', 'cpp'],
  version : '0.1.5'
)

subdir('protocols')

# 获取 sysconfdir 并动态去掉前缀
prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')

# 如果 sysconfdir 以 prefix 开头，去掉 prefix
if sysconfdir.startswith(prefix)
  sysconfdir = sysconfdir.substring(prefix.length())
endif

# 打印调试信息，确认 sysconfdir 的值
message('prefix: ' + prefix)
message('sysconfdir: ' + sysconfdir)

cc = meson.get_compiler('c')
libm = cc.find_library('m')
xcb = dependency('xcb', required : get_option('xwayland'))
xlibs = dependency('xcb-icccm', required : get_option('xwayland'))
wayland_server_dep = dependency('wayland-server')
wlroots_dep = dependency('wlroots', version : ['>=0.17.0', '<0.18.0'])
xkbcommon_dep = dependency('xkbcommon')
libinput_dep = dependency('libinput')
libwayland_client_dep = dependency('wayland-client')

# 获取 Git Commit Hash
git = find_program('git', required : false)
if git.found()
  commit_hash = run_command(git, 'rev-parse', '--short', 'HEAD', check : true).stdout().strip()
else
  commit_hash = 'unknown'
endif

# 将 Commit Hash 添加到版本信息中
version_with_hash = '@0@(@1@)'.format(meson.project_version(), commit_hash)

# 定义编译参数
c_args = [
  '-g',
  '-Wno-unused-function',
  '-DWLR_USE_UNSTABLE',
  '-D_POSIX_C_SOURCE=200809L',
  '-DVERSION="@0@"'.format(version_with_hash),  # 版本信息包含 Commit Hash
  '-DSYSCONFDIR="@0@"'.format(sysconfdir),     # 添加 sysconfdir
]

if xcb.found() and xlibs.found()
  c_args += '-DXWAYLAND'
  c_args += '-DIM'
endif

executable('maomao',
  'maomao.c',
  'util.c',
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

desktop_install_dir = join_paths(prefix, 'share/wayland-sessions')
install_data('maomao.desktop', install_dir : desktop_install_dir)

# 确保 sysconfdir 是绝对路径
if not sysconfdir.startswith('/')
  sysconfdir = '/' + sysconfdir
endif

# 安装 config.conf
install_data('config.conf', install_dir : join_paths(sysconfdir, 'maomao'))