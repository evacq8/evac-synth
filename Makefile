CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

# asound is Linux ALSA
LIBS = -lasound -lm

SRCS =	src/synthesis.c \
		src/alsa_audio_handler.c \
		src/alsa_midi_handler.c \
		src/cli_utils.c \
		src/oscillator.c

TARGET = build

.PHONY: all clean

all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
