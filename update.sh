#!/bin/bash

make clean
clear
if make -j4; then
	systemctl --user stop pulseaudio.socket
	systemctl --user stop pulseaudio.service
	sudo rmmod snd-marian
	sudo insmod marian/snd-marian.ko
	systemctl --user start pulseaudio.service
	systemctl --user start pulseaudio.socket
else
	echo "Build failed"
fi
