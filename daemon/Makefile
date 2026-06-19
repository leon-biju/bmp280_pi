CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic

PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
UNITDIR = /etc/systemd/system

bmp280d:
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/$@ *.c

install: bmp280d
	install -D -m 0755 build/bmp280d $(DESTDIR)$(BINDIR)/bmp280d
	install -D -m 0644 bmp280d.service $(DESTDIR)$(UNITDIR)/bmp280d.service
	@echo "Run: systemctl daemon-reload && systemctl enable --now bmp280d"

uninstall:
	systemctl disable --now bmp280d 2>/dev/null || true
	rm -f $(DESTDIR)$(BINDIR)/bmp280d
	rm -f $(DESTDIR)$(UNITDIR)/bmp280d.service
	systemctl daemon-reload 2>/dev/null || true

clean:
	rm -f build/bmp280d

.PHONY: install uninstall clean
