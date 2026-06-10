CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic

bmp280d: daemon.c iio_reader.c iio_reader.h
	$(CC) $(CFLAGS) -o build/$@ daemon.c iio_reader.c

clean:
	rm -f bmp280d

.PHONY: clean
