#include <stdio.h>
#include <ctype.h>
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

void wrap_lines(FILE *ifp, FILE *ofp, int max_len) {
  int c;
  char buf[1024];
  int curr_len = 0;
  int last_ws = -1;

  fseek(ifp, 0, SEEK_SET);

  while((c = fgetc(ifp)) != EOF) {
    if ((c == ' ' || c == '\t') && curr_len >= max_len) {
      last_ws = curr_len;

      // flush buffer up to whitespace before max_len
      for (int i = 0; i < curr_len; i++) {
        fputc(buf[i], ofp);
      }

      fputc('\n', ofp);

      // reset items that have not been flushed, ignoring leading whitespace
      // as this will be the start of a newline.
      int new_curr_len = 0;
      for (int i = last_ws + 1; i < curr_len; i++) {
        new_curr_len = i - last_ws - 1;
        buf[new_curr_len] = buf[i];
      }

      curr_len = new_curr_len;

      memset(&buf[curr_len], 0, sizeof(buf) - curr_len);

      last_ws = -1;

      continue;
    }

    if (c == '\n') {
      fputc(c, ofp);
      continue;
    }


    if ((c == ' ' || c == '\t')) {
      last_ws = curr_len;
    }

    buf[curr_len] = c;

    curr_len++;
  }

  for (int i = 0; i < curr_len; i++) {
    fputc(buf[i], ofp);
  }
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

  printf("dumb truncate:\n\n");
  wrap_lines_hard(ifp, ofp, 30);
  printf("\n\nsmart truncate:\n\n");
  wrap_lines(ifp, ofp, 30);

  fclose(ifp);
  fclose(ofp);
}
