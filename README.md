

https://github.com/user-attachments/assets/da80c6d1-b9a5-44c4-b738-6421365e6aa5



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
- workspace switch animaition
- window close animaition
- custom mov/open/close/tag animaition sppeed
- fade in / fade out


# install 
# wlroots(0.17)
```
git clone -b 0.17.4 https://gitlab.freedesktop.org/wlroots/wlroots.git
cd wlroots
meson build -Dprefix=/usr
sudo ninja -C build install

git clone https://github.com/DreamMaoMao/maomaowm.git
cd maomaowm
meson build -Dprefix=/usr
sudo ninja -C build install

// set your autostart cmd ih this
touch ~/.config/maomao/autostart.sh

```
# dependcy

## tools
```
yay -S wofi foot

```

# thanks for some refer repo 
https://github.com/dqrk0jeste/owl - for window animation implementation code
https://github.com/djpohly/dwl - for base dwl code

