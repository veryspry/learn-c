CC = gcc
CFLAGS = -Wall -Wextra

CUNIT = $(shell brew --prefix cunit)
CUNIT_CPPFLAGS = -I$(CUNIT)/include
CUNIT_LDFLAGS = -L$(CUNIT)/lib
CUNIT_LDLIBS = -lcunit

OUT = out

.PHONY: all test clean
all: $(OUT)/lines $(OUT)/window $(OUT)/cpu-monitor $(OUT)/avl $(OUT)/avl-tests

$(OUT):
	mkdir -p $@

$(OUT)/lines: lines/lines.c fairy-dust/fairy-dust.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT)/window: window/window.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT)/cpu-monitor: cpu-monitor/main.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT)/avl: avl/main.c avl/avl.c | $(OUT)
	$(CC) $(CFLAGS) -o $@ $^

$(OUT)/avl-tests: avl/test.c avl/avl.c | $(OUT)
	$(CC) $(CFLAGS) $(CUNIT_CPPFLAGS) $(CUNIT_LDFLAGS) -o $@ $^ $(CUNIT_LDLIBS)

test: $(OUT)/avl-tests
	./$(OUT)/avl-tests

clean:
	rm -rf $(OUT)
