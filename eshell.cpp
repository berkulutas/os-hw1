#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "parser.h"

#define STDIN 0
#define STDOUT 1
#define PIPEREAD 0
#define PIPEWRITE 1

void execute_cmd(command* cmd) {
    // quit
    if (strcmp(cmd->args[0], "quit") == 0) {
        exit(0);
    }

    if (fork()) { // parent
        wait(nullptr);
    }
    else { // child
        execvp(cmd->args[0], cmd->args);
        exit(0);
    }
}


void execute(single_input* input) {
    // cmd
    if ( input->type == INPUT_TYPE_COMMAND ) {
        execute_cmd(&input->data.cmd);
    }
    // pline
    else if ( input->type == INPUT_TYPE_PIPELINE ) {
        
    }
    // subshell
    else if ( input->type == INPUT_TYPE_SUBSHELL ) {

    }
}

void pipeline_execute(single_input* input, int num_cs) {
    // create pipes
    int pipes[num_cs - 1][2];
    for (int i = 0; i < num_cs - 1; i++) {
        pipe(pipes[i]);
    }
    // create children
    for (int i = 0; i < num_cs; i++) {
        if (fork()) { // parent
            if (i != 0) {
                close(pipes[i-1][PIPEREAD]);
            }
            if (i != num_cs - 1) {
                close(pipes[i][PIPEWRITE]);
            }
        }
        else { // child
            if (i != 0) {
                dup2(pipes[i-1][PIPEREAD], STDIN);
            }
            if (i != num_cs - 1) {
                dup2(pipes[i][PIPEWRITE], STDOUT);
            }
            // close all pipes
            for (int j = 0; j < num_cs - 1; j++) {
                close(pipes[j][PIPEREAD]);
                close(pipes[j][PIPEWRITE]);
            }
            execute(input + i);
            exit(0);    
        }
    }
    // wait for all children
    for (int i = 0; i < num_cs; i++) {
        wait(nullptr);
    }
}



void process_parsed(parsed_input *parsed) {
    single_input* inputs = parsed->inputs;
    switch (parsed->separator)
    {
    case SEPARATOR_NONE: {
        execute(parsed->inputs);
        break;
    }
    
    case SEPARATOR_PIPE:{
        pipeline_execute(parsed->inputs, parsed->num_inputs);
        break;
    }

    case SEPARATOR_SEQ:{
        std::cout << "Sequential: " << std::endl;
        break;
    }
    
    case SEPARATOR_PARA:{
        std::cout << "Parallel: " << std::endl;
        break;
    }

    default:
        break;
    }
}

int main() {
    while (1) {
        std::cout << "/> ";
        std::string input;
        std::getline(std::cin, input);

        parsed_input parsed;
        parse_line((char *)input.c_str(), &parsed);
        process_parsed(&parsed);
        free_parsed_input(&parsed);
    
    }

}