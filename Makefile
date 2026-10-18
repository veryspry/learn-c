CC = gcc
CFLAGS = -Wall -Wextra

out/lines: ./lines/lines.c ./fairy-dust/fairy-dust.c
	mkdir -p out
	$(CC) $(CFLAGS) -o out/lines lines/lines.c fairy-dust/fairy-dust.c

out/window: ./window/window.c
	mkdir -p out
	$(CC) $(CFLAGS) -o out/window window/window.c 

out/cpu-monitor: ./cpu-monitor/main.c
	mkdir -p out
	$(CC) $(CFLAGS) -o out/cpu-monitor cpu-monitor/main.c

clean:
	rm -rf out
