{ inputs, self }:
{
  config,
  lib,
  pkgs,
  ...
}:
let
  cfg = config.programs.maomaowm;
  mmsg = lib.types.submodule {
    options = {
      enable = lib.mkEnableOption "Enable mmsg, the ipc for maomaowm";
      package = lib.mkOption {
        type = lib.types.package;
        default = inputs.mmsg.packages.${pkgs.system}.mmsg;
        description = "The mmsg package to use";
      };
    };
  };
in
{
  options = {
    programs.maomaowm = {
      enable = lib.mkEnableOption "maomaowm, a wayland compositor based on dwl";
      package = lib.mkOption {
        type = lib.types.package;
        default = self.packages.${pkgs.system}.maomaowm;
        description = "The maomaowm package to use";
      };
      mmsg = lib.mkOption {
        type = mmsg;
        default = {
          enable = true;
        };
        description = "Options for mmsg, the ipc for maomaowm";
      };
    };
  };

  config = lib.mkMerge [
    (lib.mkIf cfg.enable {
      environment.systemPackages = [ cfg.package ];

      xdg.portal = {
        enable = lib.mkDefault true;

        wlr.enable = lib.mkDefault true;

        configPackages = [ cfg.package ];
      };

      security.polkit.enable = lib.mkDefault true;

      programs.xwayland.enable = lib.mkDefault true;

      services = {
        displayManager.sessionPackages = [ cfg.package ];

        graphical-desktop.enable = lib.mkDefault true;
      };

    })

    (lib.mkIf cfg.mmsg.enable {
      environment.systemPackages = [ cfg.mmsg.package ];
    })
  ];
}
