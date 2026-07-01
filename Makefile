CC = gcc
CFLAGS = -Wall -Wextra

out/lines: ./lines/lines.c ./fairy-dust/fairy-dust.c
	mkdir -p out
	$(CC) $(CFLAGS) -o out/lines lines/lines.c fairy-dust/fairy-dust.c

clean:
	rm -rf out
