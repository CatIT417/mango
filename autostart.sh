#! /bin/bash

set +e

systemctl --user unmask xdg-desktop-portal-hyprland
systemctl --user mask xdg-desktop-portal-gnome

dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP=sway

/usr/lib/xdg-desktop-portal-hyprland &

wl-clip-persist --clipboard regular &
wl-paste --type text --watch cliphist store & 


