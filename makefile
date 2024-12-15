CC = gcc
CFLAGS = -g -pthread
TARGET = main
SOURCES = main.c
OBJECTS = $(SOURCES:.c=.o)

# Default rule to build the program
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJECTS) $(TARGET)

