
Debian
====================
This directory contains files used to package npwd/npw-qt
for Debian-based Linux systems. If you compile npwd/npw-qt yourself, there are some useful files here.

## npw: URI support ##


npw-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install npw-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your npw-qt binary to `/usr/bin`
and the `../../share/pixmaps/npw128.png` to `/usr/share/pixmaps`

npw-qt.protocol (KDE)

