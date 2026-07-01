#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *data;
  int capacity;
  int len;
  int head;
  int tail;
} charbuf;

typedef struct flag_def {
  const char *name;
  int requires_value;
  const char *help;
} flag_def;

typedef struct parsed_flag {
  const char *name;
  const char *value;
} parsed_flag;

typedef struct parsed_args {
  int argc;
  char **argv;
  const char *help;
  parsed_flag flags[16]; // supports up to 16 flags
  int flag_count;
  char *pos_args[16]; // supports up to 16 positional arguments
  int pos_args_count;
} parsed_args;

typedef struct command_def {
  const char *command;
  const char *help;
  const flag_def *flags;
  const int flag_count;
  // TODO positional_count
  int (*handler)(parsed_args *pargs);
} command_def;

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

int safe_str_to_int(const char *str, int *out) {
  char *endptr;
  errno = 0;

  long val = strtol(str, &endptr, 10);

  // Check for conversion errors
  if (endptr == str) {
    return 1; // no digits found
  }
  if (*endptr != '\0') {
    return 1; // trailing garbage (e.g. "123abc")
  }
  if (errno == ERANGE) {
    return 1; // overflow/underflow
  }
  if (val > INT_MAX || val < INT_MIN) {
    return 1; // out of int range
  }

  *out = (int)val;

  return 0;
}

int handle_wrap(parsed_args *pargs) {
  if (pargs->pos_args_count == 0) {
    fprintf(stderr, "Fatal: no input file provided\n");
    return 1;
  }

  char *in_file = pargs->pos_args[0];

  FILE *ifp = NULL;
  const char *len_err_str = NULL;

  ifp = fopen(in_file, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Fatal: could not open %s\n", in_file);
    return 1;
  }

  int len = 50;

  for (int i = 0; i < pargs->flag_count; i++) {
    if (strcmp(pargs->flags[i].name, "len") == 0) {
      if (safe_str_to_int(pargs->flags[i].value, &len) != 0) {
        fprintf(stderr, "Fatal: failed to convert --len arg %s to integer.\n",
                pargs->flags[i].value);
      }
    }
  }

  wrap_lines(ifp, stdout, len);

  fclose(ifp);

  return 0;
}

int handle_upper(parsed_args *pargs) {
  if (pargs->pos_args_count == 0) {
    fprintf(stderr, "Fatal: no input file provided\n");
    return 1;
  }

  char *in_file = pargs->pos_args[0];

  FILE *ifp = NULL;

  ifp = fopen(in_file, "r");

  if (ifp == NULL) {
    fprintf(stderr, "Fatal: could not open %s\n", in_file);
    return 1;
  }

  stream_to_upper(ifp, stdout);

  fclose(ifp);
  return 0;
}

int parse_args(parsed_args *args, command_def *def, int argc, char **argv) {
  args->argc = argc;
  args->argv = argv;
  args->help = def->help;
  args->flag_count = 0;
  args->pos_args_count = 0;

  for (int i = 2; i < argc; i++) {
    if (strncmp(argv[i], "--", 2) == 0) {
      const char *flag_name = argv[i] + 2; // skip "--"

      if (strcmp(flag_name, "help") == 0) {
        fprintf(stdout, "%s", def->help);
        exit(0);
      }

      for (int f = 0; f < def->flag_count; f++) {
        if (strcmp(def->flags[f].name, flag_name) == 0) {
          args->flags[args->flag_count].name = flag_name;

          if (def->flags[f].requires_value) {
            if (i + 1 > argc) {
              fprintf(stderr, "Fatal: %s requires value.", flag_name);
              exit(1);
            }

            args->flags[args->flag_count].value = argv[i + 1];
            i++;
          } else {
            args->flags[args->flag_count].value = NULL;
          }

          args->flag_count++;
        }
      }
    } else {
      args->pos_args[args->pos_args_count] = argv[i];

      args->pos_args_count++;
    }
  }

  return 0;
}

int excmd(struct command_def *table, int table_len, const char *help, int argc,
          char **argv) {
  if (argc < 2) {
    fprintf(stdout, "%s\n", help);
    return 0;
  }

  if (strcmp(argv[1], "--help") == 0) {
    fprintf(stdout, "%s\n", help);
    return 0;
  }

  for (int i = 0; i < table_len; i++) {
    if (strcmp(argv[1], table[i].command) == 0) {
      parsed_args args;
      int res = parse_args(&args, &table[i], argc, argv);

      if (res == 0) {
        res = table[i].handler(&args);
      }

      return res;
    }
  }

  fprintf(stderr, "Fatal: Invalid command %s passed.\n", argv[1]);

  return 1;
}

int main(int argc, char **argv) {
  const char *help =
      "Stream based text manipulation tools for fun 🧚‍♂️.\n\n\
USAGE\n\
  lines <command> [flags]\n\n\
COMMANDS\n\
  wrap:     Wrap lines.\n\
  upper:    Uppercase input\n\n\
FLAGS\n\
  --help    Show help for command\n\
";

  const char *wrap_help = "Wrap lines.\n\n\
USAGE\n\
  lines wrap <input-file> [flags]\n\n\
FLAGS\n\
  --help    Show help for command\n\
  --len     Line wrap length\n\
";

  const char *upper_help = "Uppercase input.\n\n\
USAGE\n\
  lines upper <input-file> [flags]\n\n\
FLAGS\n\
  --help    Show help for command\n\
";

  const flag_def wrap_flags[] = {
      {.name = "help", .requires_value = 0, .help = "TODO"},
      {.name = "len", .requires_value = 1, .help = "TODO"},
  };

  const flag_def upper_flags[] = {
      {.name = "help", .requires_value = 0, .help = "TODO"},
  };

  struct command_def command_table[] = {
      {.command = "wrap",
       .handler = handle_wrap,
       .flags = wrap_flags,
       .flag_count = sizeof(wrap_flags) / sizeof(wrap_flags[0]),
       .help = wrap_help},
      {.command = "upper",
       .handler = handle_upper,
       .flags = upper_flags,
       .flag_count = sizeof(upper_flags) / sizeof(upper_flags[0]),
       .help = upper_help},
  };

  int table_len = sizeof(command_table) / sizeof(command_table[0]);

  return excmd(command_table, table_len, help, argc, argv);
}
