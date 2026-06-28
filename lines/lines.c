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
  int capacity;
  int len;
  int head;
  int tail;
} charbuf;

charbuf *charbuf_new(int initial_cap) {
  charbuf *buf;
  buf = malloc(sizeof(charbuf));

  if (buf == NULL) {
    fprintf(stderr, "Fatal: failed to allocate charbuf\n");
    exit(1);
  }

  char *data = malloc(initial_cap);
  if (data == NULL) {
    fprintf(stderr, "Fatal: failed to allocate %i bytes\n", initial_cap);
    exit(1);
  }

  buf->data = data;
  buf->capacity = initial_cap;
  buf->len = 0;
  buf->head = 0;
  buf->tail = 0;

  return buf;
}

void charbuf_free(charbuf *buf) {
  free(buf->data);
  free(buf);
}

int charbuf_push(charbuf *buf, char c) {
  if (buf->len >= buf->capacity - 1) {
    return -1;
  }

  buf->data[buf->tail] = c;

  buf->tail++;
  buf->len++;

  if (buf->tail >= buf->capacity) {
    buf->tail = 0;
  }

  return 0;
}

void charbuf_flush_line_chunk(charbuf *buf, FILE *ofp, int from, int to) {
  int count = (to - from + buf->capacity) % buf->capacity;

  for (int n = 0; n < count; n++) {
    int i = (from + n) % buf->capacity;

    fputc(buf->data[i], ofp);
  }
}

void charbuf_flush_line(charbuf *buf, FILE *ofp, int from, int to) {
  charbuf_flush_line_chunk(buf, ofp, from, to);

  fputc('\n', ofp);
}

void wrap_lines(FILE *ifp, FILE *ofp, int max_len) {
  int c;

  charbuf *buf = charbuf_new(1024);

  int push_res = 0;

  int last_ws = -1;

  fseek(ifp, 0, SEEK_SET);

  while ((c = fgetc(ifp)) != EOF) {

    if (buf->len >= buf->capacity - 2) {
      // if the line buffer is full, flush a partial line and reset the buffer.
      charbuf_flush_line_chunk(buf, ofp, buf->head, buf->tail);
      buf->len = 0;
      buf->head = 0;
      buf->tail = 0;
      continue;
    }

    if ((c == ' ' || c == '\t') && buf->len >= max_len) {
      // flush buffer up to whitespace before max_len
      // or whole buffer if this is -1 as there isn't another whitespace in the
      // buffer
      if (last_ws == -1) {
        charbuf_flush_line(buf, ofp, buf->head, buf->tail);

        buf->head = buf->tail;
        buf->len = 0;
      } else {
        charbuf_flush_line(buf, ofp, buf->head, last_ws);

        // skip ahead because the whitespace gets consumed by the wrap
        buf->head = last_ws + 1;

        buf->len = (buf->tail - buf->head + buf->capacity) % buf->capacity;

        last_ws = buf->tail;
      }

      continue;
    }

    if (c == '\n') {
      charbuf_flush_line(buf, ofp, buf->head, buf->tail);

      buf->head = buf->tail;
      buf->len = 0;
      last_ws = -1;
      continue;
    }

    if ((c == ' ' || c == '\t')) {
      last_ws = buf->tail;
    }

    push_res = charbuf_push(buf, c);

    if (push_res == -1) {
      fprintf(stderr, "Fatal: buffer overflowed\n");
      exit(1);
    }
  }

  // flush remaining contents
  charbuf_flush_line(buf, ofp, buf->head, buf->tail);

  charbuf_free(buf);
}

void print_help() {
  fprintf(stdout,
          "Stream based text manipulation tools for fun 🧚‍♂️.\n\n\
USAGE\n\
  lines <command> [flags]\n\n\
COMMANDS\n\
  wrap:     Wrap lines.\n\
  upper:    Uppercase input\n\n\
FLAGS\n\
  --help    Show help for command\n\
  \n");
}

void print_help_wrap() {
  fprintf(stdout, "Wrap lines.\n\n\
USAGE\n\
  lines wrap <input-file> [flags]\n\n\
FLAGS\n\
  --help    Show help for command\n\
  --len     Line wrap length\n\
");
}

void print_help_upper() {
  fprintf(stdout, "Uppercase input.\n\n\
USAGE\n\
  lines upper <input-file> [flags]\n\n\
FLAGS\n\
  --help    Show help for command\n\
");
}

typedef struct wrap_args {
  char *in_file;
  int len;
} wrap_args;

void handle_wrap(int argc, char **argv) {
  wrap_args *args = malloc(sizeof(wrap_args));

  if (args == NULL) {
    fprintf(stderr, "Fatal: could not allocate %lu bytes.", sizeof(wrap_args));
    exit(1);
  }

  args->in_file = NULL;
  args->len = 50;

  FILE *ifp = NULL;
  const char *len_err_str = NULL;

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      print_help_wrap();
      exit(0);
    }

    if (strcmp(argv[i], "--len") == 0) {
      args->len = strtonum(argv[i + 1], 1, 10000, &len_err_str);

      if (len_err_str != NULL) {
        fprintf(stderr, "Fatal: [%s] Coud not parse --len flag.", len_err_str);
        exit(1);
      }

      i++;
      continue;
    }

    args->in_file = argv[i];
  }

  if (args->in_file == NULL) {
    fprintf(stderr, "Fatal: no input file provided\n");
    exit(1);
  }

  ifp = fopen(args->in_file, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Fatal: could not open %s\n", args->in_file);
    exit(1);
  }

  wrap_lines(ifp, stdout, args->len);

  fclose(ifp);
}

typedef struct upper_args {
  char *in_file;
} upper_args;

void handle_upper(int argc, char **argv) {
  upper_args *args = malloc(sizeof(upper_args));

  if (args == NULL) {
    fprintf(stderr, "Fatal: could not allocate %lu bytes.", sizeof(upper_args));
  }

  args->in_file = NULL;

  FILE *ifp = NULL;

  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      print_help_upper();
      exit(0);
    }

    args->in_file = argv[i];
  }

  if (args->in_file == NULL) {
    fprintf(stderr, "Fatal: no input file provided\n");
    exit(1);
  }

  ifp = fopen(args->in_file, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Fatal: could not open %s\n", args->in_file);
    exit(1);
  }

  stream_to_upper(ifp, stdout);

  fclose(ifp);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_help();
    return 0;
  }

  if (strcmp(argv[1], "wrap") == 0) {
    handle_wrap(argc, argv);
  } else if (strcmp(argv[1], "upper") == 0) {
    handle_upper(argc, argv);
  } else {
    print_help();
  }
  return 0;
}
