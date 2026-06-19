CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -pthread

PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
UNITDIR = /etc/systemd/system

bmp280d:
	@mkdir -p build
	$(CC) $(CFLAGS) -o build/$@ *.c

install: bmp280d
	install -D -m 0755 build/bmp280d $(DESTDIR)$(BINDIR)/bmp280d
	install -D -m 0644 bmp280d.service $(DESTDIR)$(UNITDIR)/bmp280d.service
	@if [ -z "$(DESTDIR)" ] && command -v systemctl >/dev/null 2>&1; then \
		systemctl daemon-reload; \
		systemctl try-restart bmp280d.service; \
		echo "daemon-reloaded and restarted bmp280d (if it was already running)."; \
		echo "First-time setup: systemctl enable --now bmp280d"; \
	else \
		echo "Run: systemctl daemon-reload && systemctl enable --now bmp280d"; \
	fi

uninstall:
	systemctl disable --now bmp280d 2>/dev/null || true
	rm -f $(DESTDIR)$(BINDIR)/bmp280d
	rm -f $(DESTDIR)$(UNITDIR)/bmp280d.service
	systemctl daemon-reload 2>/dev/null || true

clean:
	rm -f build/bmp280d

.PHONY: install uninstall clean
