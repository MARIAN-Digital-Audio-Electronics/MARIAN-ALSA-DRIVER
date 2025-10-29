#!/bin/bash

make clean
clear
if make -j4; then
	systemctl --user stop wireplumber pipewire pipewire-pulse
	sudo rmmod snd-marian
	sudo insmod marian/snd-marian.ko
	systemctl --user start wireplumber pipewire pipewire-pulse
else
	echo "Build failed"
fi
