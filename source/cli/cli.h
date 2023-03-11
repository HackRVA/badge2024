//
// Created by Samuel Jones on 11/9/21.
//

#ifndef badge_c_CLI_H
#define badge_c_CLI_H

#include <stddef.h>

typedef struct CLI_COMMAND {
    const char * name;
    const char * help;
    int (*process)(char*);
    struct CLI_COMMAND *subcommands;
} CLI_COMMAND;

void cli_run(const CLI_COMMAND *cmd);
int cli_get_line(const char *prompt, char* line, size_t len);
char* cli_get_token(char** line);
int cli_process_line(const CLI_COMMAND *parent, const CLI_COMMAND *cmd, char* line);

#endif //badge_c_CLI_H
