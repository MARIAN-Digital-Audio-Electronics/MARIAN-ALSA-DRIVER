#!/bin/bash

stop_audio_services() {
    if command -v pulseaudio &>/dev/null; then
        echo "PulseAudio detected, stopping PulseAudio..."
        systemctl --user stop pulseaudio.socket
        systemctl --user stop pulseaudio.service
    elif command -v pipewire &>/dev/null; then
        echo "PipeWire (with PulseAudio compatibility layer) detected, stopping PipeWire..."
        systemctl --user stop wireplumber
        systemctl --user stop pipewire-pulse.service
        systemctl --user stop pipewire-pulse.socket
        systemctl --user stop pipewire.service pipewire.socket
    fi
}

start_audio_services() {
    if command -v pulseaudio &>/dev/null; then
        echo "Starting PulseAudio..."
        systemctl --user start pulseaudio.service
        systemctl --user start pulseaudio.socket
    elif command -v pipewire &>/dev/null; then
        echo "Starting PipeWire (with PulseAudio compatibility layer)..."
        systemctl --user start pipewire.socket pipewire.service
        systemctl --user start pipewire-pulse.socket
        systemctl --user start pipewire-pulse.service
        systemctl --user start wireplumber
    fi
}

make clean
clear
if make -j4; then
        stop_audio_services
        echo "Removing the old kernel module..."
        sudo rmmod snd-marian || true
        echo "Inserting the new kernel module..."
        sudo insmod marian/snd-marian.ko
        start_audio_services
else
        echo "Build failed"
fi
