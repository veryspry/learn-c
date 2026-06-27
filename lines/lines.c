#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stream_to_upper(FILE *ifp, FILE *ofp) {
  int c;

  while ((c = fgetc(ifp)) != EOF) {
    fprintf(ofp, "%c", toupper(c));
  }
}

void wrap_lines_hard(FILE *ifp, FILE *ofp, int max_len) {
  int c;
  int curr_len = 0;

  fseek(ifp, 0, SEEK_SET);

  while ((c = fgetc(ifp)) != EOF) {
    if (curr_len >= max_len) {
      fputc('\n', ofp);
      curr_len = 0;
    }

    fprintf(ofp, "%c", c);

    curr_len++;
  }
}

void flush_line(FILE *ofp, char *buf, int from, int to) {
  for (int i = from; i < to; i++) {
    fputc(buf[i], ofp);
  }

  fputc('\n', ofp);
}

typedef struct {
  char *data;
  int len;
  int capacity;
  int head;
  int tail;
} charbuf;

charbuf charbuf_new(int initial_cap) {
  charbuf buf;

  void *data = malloc(initial_cap);
  if (data == NULL) {
    fprintf(stderr, "Fatal: failed to allocate %i bytes", initial_cap);
    abort();
  }

  buf.data = data;
  buf.len = 0;
  buf.capacity = initial_cap;
  buf.head = 0;
  buf.tail = 0;

  return buf;
}

void charbuf_free(charbuf *buf) {
  free(buf->data);

  buf->data = NULL;
  buf->len = 0;
  buf->capacity = 0;
  buf->head = 0;
  buf->tail = 0;
}

void charbuf_push(charbuf *buf, char c) {
  buf->data[buf->tail] = c;

  buf->tail++;
  buf->len++;

  if (buf->tail >= buf->capacity) {
    buf->tail = 0;
  }
}

void charbuf_flush_line(charbuf *buf, FILE *ofp, int from, int to) {

  int count = (to - from + buf->capacity) % buf->capacity;

  for (int n = 0; n < count; n++) {
    int i = (from + n) % buf->capacity;

    fputc(buf->data[i], ofp);
  }

  fputc('\n', ofp);
}

void wrap_lines(FILE *ifp, FILE *ofp, int max_len) {
  int c;

  charbuf buf = charbuf_new(1024);

  int last_ws = -1;

  fseek(ifp, 0, SEEK_SET);

  while ((c = fgetc(ifp)) != EOF) {
    if ((c == ' ' || c == '\t') && buf.len >= max_len) {
      // flush buffer up to whitespace before max_len
      // or whole buffer if this is -1 as there isn't another whitespace in the
      // buffer
      if (last_ws == -1) {
        charbuf_flush_line(&buf, ofp, buf.head, buf.tail);

        buf.head = buf.tail;
        buf.len = 0;
      } else {
        charbuf_flush_line(&buf, ofp, buf.head, last_ws);

        // skip ahead because the whitespace gets consumed by the wrap
        buf.head = last_ws + 1;

        // TODO fix this in case of buffer wrap.
        buf.len = buf.tail - buf.head;

        last_ws = buf.tail;
      }

      continue;
    }

    if (c == '\n') {
      charbuf_flush_line(&buf, ofp, buf.head, buf.tail);

      buf.head = buf.tail;
      buf.len = 0;
      last_ws = -1;
      continue;
    }

    if ((c == ' ' || c == '\t')) {
      last_ws = buf.tail;
    }

    charbuf_push(&buf, c);
  }

  // flush remaining contents
  charbuf_flush_line(&buf, ofp, buf.head, buf.tail);

  charbuf_free(&buf);
}

int main(int argc, char **argv) {

  if (argc < 2) {
    fprintf(
        stderr,
        "Not enough arguments. Usage: lines [INPUT_FILE] ([OUTPUT_FILE])\n");
    return 1;
  }

  char *in_filename = argv[1];
  char *out_filename = NULL;

  FILE *ifp;
  FILE *ofp = stdout;

  if (argc > 2) {
    char *out_filename = argv[2];
    ofp = fopen(out_filename, "w");
  }

  ifp = fopen(in_filename, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Could not open file\n");
    return 1;
  }

  // stream_to_upper(ifp, ofp);

  // printf("dumb truncate:\n\n");
  // wrap_lines_hard(ifp, ofp, 30);
  
  printf("\n\nsmart truncate:\n\n");
  wrap_lines(ifp, ofp, 30);

  fclose(ifp);
  fclose(ofp);
}
