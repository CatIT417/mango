
https://github.com/user-attachments/assets/84eb21fd-1782-4edc-b504-50de35201abe



# feature
- dwl ipc support
- maxmize fullscreen
- swap window by direction
- swith to next not empty-workspace/workspace
- move window to next not empty-workspace/workspace
- swith focus by direction
- wheel/button + mod key bind
- hycov like overview
- numlock state pre-set in config
- portal fix for obs(use hyprland-portal)
- master layout support new_on_top
- foreign-toplevel support(dunst,waybar wlr taskbar)
- acitve on focus/urgent support
- hide empty button in waybar
- support minimize window by click waybar
- support minimize window by keyboard
- sway scratchpad support 
- window pin mode support
- text-input-v2
- window move/open animaition


# install 
# wlroots(0.17)
```
git clone -b 0.17.4 https://gitlab.freedesktop.org/wlroots/wlroots.git
cd wlroots
meson build -Dprefix=/usr
sudo ninja -C build install

git clone https://github.com/DreamMaoMao/mydwl.git
cd mydwl
meson build -Dprefix=/usr
sudo ninja -C build install

sed -i s#/home/user#$HOME#g dwl.desktop
sudo cp dwl.desktop /usr/share/wayland-sessions/
```
# dependcy

## tools
```
yay -S wofi foot

```
# waybar (must use this waybar)
```
git clone https://gitee.com/DreamMaoMao/Waybar.git
cd mywaybar
meson build -Dprefix=/usr
sudo ninja -C build install
```

# thanks for some refer repo 
https://github.com/dqrk0jeste/owl - for window animation implementation code
https://github.com/djpohly/dwl - for base dwl code

