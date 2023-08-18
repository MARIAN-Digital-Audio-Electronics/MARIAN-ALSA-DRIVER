#!/bin/bash

make clean
clear
if make; then
	sudo rmmod snd-marian
	sudo insmod marian/snd-marian.ko
else
	echo "Build failed"
fi