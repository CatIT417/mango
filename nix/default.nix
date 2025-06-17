{
  lib,
  libX11,
  libinput,
  libxcb,
  libxkbcommon,
  pcre2,
  pixman,
  pkg-config,
  stdenv,
  wayland,
  wayland-protocols,
  wayland-scanner,
  xcbutilwm,
  xwayland,
  enableXWayland ? true,
  meson,
  ninja,
  wlroots,
  mmsg,
}: let
  pname = "maomaowm";
in
  stdenv.mkDerivation {
    inherit pname;
    version = "nightly";

    src = builtins.path {
      path = ../.;
      name = "source";
    };

    nativeBuildInputs = [
      meson
      ninja
      pkg-config
      wayland-scanner
    ];

    buildInputs =
      [
        libinput
        libxcb
        libxkbcommon
        pcre2
        pixman
        wayland
        wayland-protocols
        wlroots
      ]
      ++ lib.optionals enableXWayland [
        libX11
        xcbutilwm
        xwayland
      ];

    passthru = {
      providedSessions = ["maomao"];
      inherit mmsg;
    };

    meta = {
      mainProgram = "maomao";
      description = "A streamlined but feature-rich Wayland compositor";
      homepage = "https://github.com/DreamMaoMao/maomaowm";
      license = lib.licenses.mit;
      maintainers = [];
      platforms = lib.platforms.unix;
    };
  }
