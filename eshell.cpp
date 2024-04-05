#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "parser.h"

#define STDIN 0
#define STDOUT 1
#define PIPEREAD 0
#define PIPEWRITE 1

void process_parsed(parsed_input *parsed); 

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
        // parse subshell input
        parsed_input reparsed;
        parse_line(input->data.subshell, &reparsed);
        // fork a process to execute the subshell
        if (fork()) { // parent
            wait(nullptr);
        }
        else { // child
            if (reparsed.separator != SEPARATOR_PARA) { // if not parallel exec normally
                process_parsed(&reparsed);
                exit(0);
            }
            else { // parallel execution
                std::cout << "Parallel execution: " << input->data.subshell << "\n";
                std::cout << reparsed.num_inputs << "\n";
                // create N pipes for N commands
                int pipes[reparsed.num_inputs][2];
                for (int i = 0; i < reparsed.num_inputs; i++) {
                    pipe(pipes[i]);
                }
                for (int i = 0; i < reparsed.num_inputs; i++) {
                    if (fork()) { // parent
                        close(pipes[i][PIPEREAD]);
                        close(pipes[i][PIPEWRITE]);
                    }
                    else { // child
                        dup2(pipes[i][PIPEREAD], STDIN);
                        close(pipes[i][PIPEREAD]);
                        close(pipes[i][PIPEWRITE]);
                        process_parsed(&reparsed);
                        exit(0);
                    }
                }
            }
            for (int i = 0; i < reparsed.num_inputs; i++) {
                wait(nullptr);
            }
        }
        free_parsed_input(&reparsed);
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

void pipeline_execute2(pipeline* pline) {
    // create pipes
    int pipes[pline->num_commands - 1][2];
    for (int i = 0; i < pline->num_commands - 1; i++) {
        pipe(pipes[i]);
    }
    // create children
    for (int i = 0; i < pline->num_commands; i++) {
        if (fork()) { // parent
            if (i != 0) {
                close(pipes[i-1][PIPEREAD]);
            }
            if (i != pline->num_commands - 1) {
                close(pipes[i][PIPEWRITE]);
            }
        }
        else { // child
            if (i != 0) {
                dup2(pipes[i-1][PIPEREAD], STDIN);
            }
            if (i != pline->num_commands - 1) {
                dup2(pipes[i][PIPEWRITE], STDOUT);
            }
            // close all pipes
            for (int j = 0; j < pline->num_commands - 1; j++) {
                close(pipes[j][PIPEREAD]);
                close(pipes[j][PIPEWRITE]);
            }
            execute_cmd(&pline->commands[i]);
            exit(0);    
        }
    }
    // wait for all children
    for (int i = 0; i < pline->num_commands; i++) {
        wait(nullptr);
    }
}



void process_parsed(parsed_input *parsed) {
    single_input* inputs = parsed->inputs;
    // pretty_print(parsed);
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
        for (int i = 0; i < parsed->num_inputs; i++) {
            // if input is a pipeline, execute it
            if (inputs[i].type == INPUT_TYPE_PIPELINE) {
                pipeline_execute2(&inputs[i].data.pline);
            }
            // if input is  command fork a process
            else {
                execute(inputs + i);
            }
        }
        break;
    }
    
    case SEPARATOR_PARA:{
        for (int i = 0; i < parsed->num_inputs; i++) {
            if (inputs[i].type == INPUT_TYPE_PIPELINE) {
                pipeline_execute2(&inputs[i].data.pline);
            }
            else {
                if (fork()) { // parent
                    
                }
                else { // child
                    execvp(inputs[i].data.cmd.args[0], inputs[i].data.cmd.args);
                }
            }
        }
        for (int i = 0; i < parsed->num_inputs; i++) {
            wait(nullptr);
        }
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