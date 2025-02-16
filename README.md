
# 

Master-Stack Layout

https://github.com/user-attachments/assets/da80c6d1-b9a5-44c4-b738-6421365e6aa5


Scroller Layout

https://github.com/user-attachments/assets/2ff96868-b276-4fa1-b4d7-87bdc36beb3c

# Maomaowm

This project is developed based on `dwl(0.5)` , adding many window functions supported in hyprland, such as window switching by direction, non-empty workspace switching ,etc... It also integrates niri's scroll layout and sway'scratchpad . See below for more features


# some special feature
- hycov like overview
- foreign-toplevel support(dunst,waybar wlr taskbar)
- minimize window to waybar(like hych)
- sway scratchpad support 
- window pin mode support
- text-input-v2/v3 for fcitx5
- window move/open animaition
- workspace switch animaition
- fade in animation
- alt-tab switch window like gnome
- niri like scroller layout

## suggest tools
```
yay -S wofi foot xdg-desktop-portal-wlr swaybg waybar wl-clip-persist cliphist wl-clipboard wlsunset

```

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

# set your autostart app ih this
mkdir -p ~/.config/maomao/

# some suggest config
cp autostart.sh ~/.config/maomao/
cp waybar ~/.config/maomao/ -r 
cp wofi ~/.config/maomao/ -r 
cp wallpaper ~/.config/maomao/ -r


```

# config
the config file is `~/.config/maomao/config.conf`
the autostart file is `~/.config/maomao/autostart.sh`


# thanks for some refer repo 
- https://github.com/dqrk0jeste/owl - for basal window animaition

- https://github.com/djpohly/dwl - for basal dwl feature

- https://github.com/guyuming76/dwl - for text-input

- https://github.com/swaywm/sway - for foreign-toplevel
