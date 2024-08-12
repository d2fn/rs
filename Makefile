# Define the compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 --debug
LDFLAGS = -lm

# Define the source files and output executable
SRCS = main.c rs_render.c rs_input.c rs_map.c rs_player.c
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

