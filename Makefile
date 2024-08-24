# Define the compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 --debug -g
LDFLAGS = -lm

# Define the source files and output executable
SRCS = main.c rs_render.c rs_input.c rs_grid.c rs_player.c rs_tween.c rs_graphics.c rs_types.c rs_terrain.c rs_geometry.c rs_math.c
OBJS = $(SRCS:.c=.o)
TARGET = rs

# SDL2 flags (assuming SDL2 is installed on your system)
SDL2_CFLAGS = $(shell sdl2-config --cflags)
SDL2_LDFLAGS = $(shell sdl2-config --libs)

# Rule to build the object files
%.o: %.c
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c $< -o $@

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(SDL2_LDFLAGS) -o $(TARGET)

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets to avoid conflicts with files
.PHONY: clean

