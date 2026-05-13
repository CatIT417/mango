---
title: Nix Module Options
description: NixOS and Home Manager configuration options for mangowm.
---

> **Note:** This document is automatically generated from the Nix module source code.

## NixOS

**System-level options via `programs.mango`.**

### `addLoginEntry`

Whether to add a login entry to the display manager for mango. Only has effect if a display manager is configured (e.g. SDDM, GDM via `services.displayManager`).

**Type:** `boolean`

**Default:** `true`

---

### `enable`

Whether to enable mango, a wayland compositor based on dwl.

**Type:** `boolean`

**Default:** `false`

**Example:** `true`

---

### `package`

The mango package to use

**Type:** `package`

**Default:** `<derivation mango-nightly>`

---

## Home Manager

**Configure mangowm declaratively via `wayland.windowManager.mango`.**

### `autostart_sh`

Shell script to run on mango startup. No shebang needed.

When this option is set, the script will be written to
`~/.config/mango/autostart.sh` and an `exec-once` line
will be automatically added to the config to execute it.

**Type:** `strings concatenated with "\n"`

**Default:** `""`

**Example:** 
```nix
''
  waybar &
  dunst &
''
```

---

### `bottomPrefixes`

List of prefixes for attributes that should appear at the bottom of the config file.
Attributes starting with these prefixes will be sorted to the end.

**Type:** `list of string`

**Default:** `[ ]`

**Example:** 
```nix
[
  "source"
]
```

---

### `enable`

Whether to enable mangowm, a Wayland compositor based on dwl.

**Type:** `boolean`

**Default:** `false`

---

### `extraConfig`

Extra configuration lines to add to `~/.config/mango/config.conf`.
This is useful for advanced configurations that don't fit the structured
settings format, or for options that aren't yet supported by the module.

**Type:** `strings concatenated with "\n"`

**Default:** `""`

**Example:** 
```nix
''
  # Advanced config that doesn't fit structured format
  special_option = 1
''
```

---

### `package`

The mango package to use

**Type:** `package`

**Default:** `<derivation mango-nightly>`

---

### `settings`

Mango configuration written in Nix. Entries with the same key
should be written as lists. Variables and colors names should be
quoted. See <https://mangowm.github.io/docs> for more examples.

> **Note:**
>
> This option uses a structured format that is converted to Mango's
> configuration syntax. Nested attributes are flattened with underscore separators.
> For example: `animation.duration_open = 400` becomes `animation_duration_open = 400`
> 
> Keymodes (submaps) are supported via the special `keymode` attribute. Each keymode
> is a nested attribute set under `keymode` that contains its own bindings.

**Type:** `Mango configuration value`

**Default:** `{ }`

**Example:** 
```nix
{
  # Window effects
  blur = 1;
  blur_optimized = 1;
  blur_params = {
    radius = 5;
    num_passes = 2;
  };
  border_radius = 6;
  focused_opacity = 1.0;

  # Animations - use underscores for multi-part keys
  animations = 1;
  animation_type_open = "slide";
  animation_type_close = "slide";
  animation_duration_open = 400;
  animation_duration_close = 800;

  # Or use nested attrs (will be flattened with underscores)
  animation_curve = {
    open = "0.46,1.0,0.29,1";
    close = "0.08,0.92,0,1";
  };

  # Use lists for duplicate keys like bind and tagrule
  bind = [
    "SUPER,r,reload_config"
    "Alt,space,spawn,rofi -show drun"
    "Alt,Return,spawn,foot"
    "ALT,R,setkeymode,resize"  # Enter resize mode
  ];

  tagrule = [
    "id:1,layout_name:tile"
    "id:2,layout_name:scroller"
  ];

  # Keymodes (submaps) for modal keybindings
  keymode = {
    resize = {
      bind = [
        "NONE,Left,resizewin,-10,0"
        "NONE,Escape,setkeymode,default"
      ];
    };
  };
}
```

---

### `systemd.enable`

Whether to enable `mango-session.target` on
mango startup. This links to
`graphical-session.target`.
Some important environment variables will be imported to systemd
and dbus user environment before reaching the target, including
* `DISPLAY`
* `WAYLAND_DISPLAY`
* `XDG_CURRENT_DESKTOP`
* `XDG_SESSION_TYPE`
* `NIXOS_OZONE_WL`
You can extend this list using the `systemd.variables` option.

**Type:** `boolean`

**Default:** `true`

**Example:** `false`

---

### `systemd.extraCommands`

Extra commands to run after D-Bus activation.

**Type:** `list of string`

**Default:** 
```nix
[
  "systemctl --user reset-failed"
  "systemctl --user start mango-session.target"
]
```

---

### `systemd.variables`

Environment variables imported into the systemd and D-Bus user environment.

**Type:** `list of string`

**Default:** 
```nix
[
  "DISPLAY"
  "WAYLAND_DISPLAY"
  "XDG_CURRENT_DESKTOP"
  "XDG_SESSION_TYPE"
  "NIXOS_OZONE_WL"
  "XCURSOR_THEME"
  "XCURSOR_SIZE"
]
```

**Example:** 
```nix
[
  "--all"
]
```

---

### `systemd.xdgAutostart`

Whether to enable autostart of applications using
`systemd-xdg-autostart-generator(8)`.

**Type:** `boolean`

**Default:** `false`

**Example:** `true`

---

### `topPrefixes`

List of prefixes for attributes that should appear at the top of the config file.
Attributes starting with these prefixes will be sorted to the beginning.

**Type:** `list of string`

**Default:** `[ ]`

**Example:** 
```nix
[
  "source"
]
```

---

