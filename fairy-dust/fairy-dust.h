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

int parse_args(parsed_args *args, command_def *def, int argc, char **argv);

int excmd(struct command_def *table, int table_len, const char *help, int argc,
          char **argv);
