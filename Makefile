CC = gcc
CFLAGS = -Wall -O2 -I/opt/homebrew/include
LFLAGS = -L/opt/homebrew/lib
LIBS = -lraylib -lm -lpthread -lcjson

TARGET = game
SRC = main.c tiled_loader.c map_manager.c player.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
