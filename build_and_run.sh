#!/bin/bash
echo "Compilation..."
gcc -o programme main.c game_state.c img_button.c main_menu.c options_menu.c utils.c player.c level.c platform.c minimap.c save_menu.c gameplay_state.c `sdl2-config --cflags --libs` -lSDL2_image -lSDL2_mixer -lm -Wall

if [ $? -eq 0 ]; then
    echo "OK! Lancement..."
    export SDL_RENDER_DRIVER=software
    export SDL_VIDEODRIVER=x11
    ./programme
else
    echo "Erreur de compilation"
fi
