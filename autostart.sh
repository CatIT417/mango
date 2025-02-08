#! /bin/bash

set +e

dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP=sway

wl-clip-persist --clipboard regular &
wl-paste --type text --watch cliphist store & 
wlsunset -T 3501 -t 3500 &
swaybg -i ~/.config/maomao/wallpaper/wallpaper.png &
waybar -c ~/.config/maomao/waybar/config -s ~/.config/maomao/waybar/style.css &
echo "Xft.dpi: 140" | xrdb -merge # change x11 dpi

