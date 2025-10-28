# GNU ede Makefile for Linux and Termux

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -ldl
TARGET = ede
SRC = ede.c
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Termux detection
ifneq (,$(wildcard /data/data/com.termux))
    PREFIX = $(HOME)/../usr
endif

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Building $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo "Build complete!"

clean:
	@echo "Cleaning..."
	rm -f $(TARGET) *.o *.emod

install: $(TARGET)
	@echo "Installing to $(BINDIR)..."
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Done!"

uninstall:
	rm -f $(BINDIR)/$(TARGET)
