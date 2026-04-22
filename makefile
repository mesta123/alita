# Nom de l'exécutable
TARGET = programme

# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall -g `sdl2-config --cflags`

# Options de linkage
LDFLAGS = `sdl2-config --libs` -lSDL2_image -lSDL2_mixer -lm

# Fichiers source
SRCS = main.c game_state.c img_button.c main_menu.c options_menu.c utils.c \
       player.c level.c platform.c minimap.c save_menu.c gameplay_state.c

# Fichiers objets
OBJS = $(SRCS:.c=.o)

# Règle principale
all: $(TARGET)

# Linkage
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compilation des .c en .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJS) $(TARGET)

# Exécution
run: $(TARGET)
	export SDL_RENDER_DRIVER=software && export SDL_VIDEODRIVER=x11 && ./$(TARGET)

.PHONY: all clean run
