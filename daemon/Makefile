CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic

bmp280d:
	$(CC) $(CFLAGS) -o build/$@ daemon.c iio_reader.c ring_buffer.c

clean:
	rm -f bmp280d

.PHONY: clean
