#include <stdio.h>
#include <string.h>
#include "./fairy-dust.h"

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
        return 0;
      }

      for (int f = 0; f < def->flag_count; f++) {
        if (strcmp(def->flags[f].name, flag_name) == 0) {
          args->flags[args->flag_count].name = flag_name;

          if (def->flags[f].requires_value) {
            if (i + 1 > argc) {
              fprintf(stderr, "Fatal: %s requires value.", flag_name);
              return 1;
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

typedef struct {
  char *desc;
} help_def;

void print_help() {

  // ------------------
  // MAIN HELP
  // ------------------

  // <DESCRIPTION>
  //
  // USAGE
  // <TOOL_NAME>
  //   <TOOL_NAME> <command> [flags]
  //
  // COMMANDS
  //   <CMD_1>     <DESCRIPTION>
  //   <CMD_2>     <DESCRIPTION>
  //   <CMD_2>     <DESCRIPTION>
  //
  // FLAGS
  //   <FLAG_1>    <DESCRIPTION>
  //   <FLAG_2>    <DESCRIPTION>
  //   <FLAG_3>    <DESCRIPTION>

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

  fprintf(stdout, "%s", help);

  return 1;
}
