# trekscii - random colorized Star Trek ASCII art for your login shell
#
#   make                 build ./build/trekscii
#   make install         install into $(PREFIX)  (default ~/.local)
#   make uninstall       remove the installed files
#   make clean           remove build artifacts
#
# PREFIX=/usr/local make install   installs system-wide (needs sudo)

CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra -std=c11
PREFIX  ?= $(HOME)/.local

BINDIR   = $(PREFIX)/bin
SHAREDIR = $(PREFIX)/share
ARTDIR   = $(SHAREDIR)/trekscii_art

BUILDDIR = build
BIN      = $(BUILDDIR)/trekscii
SRC      = src/trekscii.c

.PHONY: all clean install uninstall run

all: $(BIN)

$(BIN): $(SRC) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $(SRC)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run: $(BIN)
	TREKSCII_ART_DIR=art $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(ARTDIR)
	install -m 0755 $(BIN) $(DESTDIR)$(BINDIR)/trekscii
	install -m 0644 art/*.txt $(DESTDIR)$(ARTDIR)/
	@echo "Installed trekscii to $(DESTDIR)$(BINDIR)/trekscii"
	@echo "Installed art to      $(DESTDIR)$(ARTDIR)/"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/trekscii
	rm -rf $(DESTDIR)$(ARTDIR)

clean:
	rm -rf $(BUILDDIR)
