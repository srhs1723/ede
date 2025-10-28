# GNU ede Makefile for Linux and Termux

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS = -lncurses -ldl -lpthread
TARGET = ede
SRC = linux_and_termux_ede.c
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Termux detection
ifneq (,$(wildcard /data/data/com.termux))
    PREFIX = $(HOME)/../usr
endif

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	@echo "Building $(TARGET) for Linux/Termux..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	@echo "Build complete! Run with ./$(TARGET)"

clean:
	@echo "Cleaning..."
	rm -f $(TARGET) *.o *.emod *.so

install: $(TARGET)
	@echo "Installing to $(BINDIR)..."
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Done! Run with: ede"

uninstall:
	rm -f $(BINDIR)/$(TARGET)
