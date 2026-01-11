#ifndef COMMAND_H
#define COMMAND_H

typedef void (*command_handler_t)(int argc, char** argv);

struct command {
    const char* name;
    const char* description;
    command_handler_t handler;
};

void command_init();
void command_register(const char* name, const char* description, command_handler_t handler);
void command_run(char* input);

#endif
