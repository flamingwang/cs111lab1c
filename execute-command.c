// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <string.h> //for strcmp function


// Graph Implementation
// ===================================================================

typedef struct graph_node* graph_node_t;
struct graph_node {
  command_t cmd; // Command inside of the node
  
  graph_node_t* dependencies; // Array of graph node pointers indicating dependencies
  
  char** read_list; // Read List
  
  char** write_list; // Write List
};

typedef struct command_graph* command_graph_t;
struct command_graph {
  
  int size; // Number of graph nodes in graph
  
  graph_node_t* nodes;// Array of graph node pointers, indicates all nodes belonging to
  //this graph
  
  int num_stages; // Number of execution stages
  
  graph_node_t** staged_commands;
  // Array of arrays of graph node pointers
  // 1. Each array of graph node pointers is the commands that should be executed at
  //    that particular stage.
  // 2. Each array of graph node pointers also has a NULL pointer after its last
  //    command so you know where to end
  // 3. Index of each array of graph node pointers is the stage it should be executed.
  //    For example: staged_commands[0] is an array of graph node pointers indicating 
  //    which commands should be executed at stage 1. staged_commands[1] => stage 2,
  //    ... staged_commands[num_stages-1] => final stage.
};

//void test_graph(){
//  // Test 
//}

// End of Graph Implementation
// ===================================================================

static bool DEBUG = true;

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

void execute_command_nf (command_t c, int time_travel);

void appendRL(char** rl, char** rl2)
{
  int i = 0;
  while (rl[i] != NULL) {
    i++;
  }
  int i2 = 0;
  while (rl2[i2] != NULL) {
    rl[i + i2] = rl2[i2];
    i2++;
  }
}

char** createReadList(command_t c)
{
  char** readList = malloc(50);
  switch(c->type) {
  case PIPE_COMMAND:
  case OR_COMMAND:
  case SEQUENCE_COMMAND:
  case AND_COMMAND:
    appendRL(readList, createReadList(c->u.command[0]));
    appendRL(readList, createReadList(c->u.command[1]));
    break;
  case SUBSHELL_COMMAND:
    readList[0] = c->input;
    appendRL(readList, createReadList(c->u.subshell_command));
    break;
  case SIMPLE_COMMAND: 
    readList[0] = c->input;
    appendRL(readList, c->u.word);
    break;
  }
  return readList;
}

char** createWriteList(command_t c)
{
  char** writeList = malloc(50);
  switch(c->type) {
  case PIPE_COMMAND:
  case OR_COMMAND:
  case SEQUENCE_COMMAND:
  case AND_COMMAND:
    appendRL(writeList, createReadList(c->u.command[0]));
    appendRL(writeList, createReadList(c->u.command[1]));
    break;
  case SUBSHELL_COMMAND:
    writeList[0] = c->output;
    appendRL(writeList, createReadList(c->u.subshell_command));
    break;
  case SIMPLE_COMMAND: 
    writeList[0] = c->output;
    break;
  }
  return writeList;
}


int
command_status (command_t c)
{
  return c->status;
}

//Execute simple command
void execute (command_t c) {
  int child_status;
  int pid = fork();

   
  if (pid == 0) {
   if (c->input != NULL)
      freopen(c->input, "r", stdin);
    if (c->output != NULL)
      freopen(c->output, "w", stdout);    
    //printf("command: %s %s\n", c->u.word[0], c->u.word[1]);  
   
   if(!strcmp(c->u.word[0], "exec"))
     execvp(c->u.word[1], c->u.word+1);
   
    execvp(c->u.word[0], c->u.word);
    fclose(stdin);
    fclose(stdout);
  }
  else {
    int return_pid = waitpid(pid, &child_status, 0);
    c->status = WEXITSTATUS(child_status);
  }
}

void execute_nf (command_t c) {
  if (c->input != NULL)
    freopen(c->input, "r", stdin);
  if (c->output != NULL)
    freopen(c->output, "w", stdout);    
  execvp(c->u.word[0], c->u.word);
  fclose(stdin);
  fclose(stdout);
}

void execute_pipe (command_t c, int time_travel) {
  int child_status;
  int first_pid, second_pid, return_pid;
  int mypipe[2];
  pipe(mypipe);
  first_pid = fork();
  if (first_pid == 0) {
    close(mypipe[0]);
    dup2(mypipe[1], 1);
    close(mypipe[1]);
    execute_command_nf(c->u.command[0], time_travel);
  }
  else {
    second_pid = fork();
    if (second_pid == 0) {
      close(mypipe[1]);
      dup2(mypipe[0], 0);
      close(mypipe[0]);
      execute_command_nf(c->u.command[1], time_travel);
    }
    else {
      close(mypipe[0]);
      close(mypipe[1]);
      return_pid = waitpid(first_pid, &child_status, 0);
      return_pid = waitpid(second_pid, &child_status, 0);
      c->status = WEXITSTATUS(child_status);
    }
  } 
  /*
  //first_pid = fork();
  //if (first_pid == 0) {
  close(mypipe[0]);
  dup2(mypipe[1], 1);
  close(mypipe[1]);
  execute_command_nf(c->u.command[0], time_travel);
  //}
  //else {
  //second_pid = fork();
  //if (second_pid == 0) {
  close(mypipe[1]);
  dup2(mypipe[0], 0);
  close(mypipe[0]);
  execute_command_nf(c->u.command[1], time_travel);
  //}
  //else {
  close(mypipe[0]);
  close(mypipe[1]);
  return_pid = waitpid(first_pid, &child_status, 0);
    return_pid = waitpid(second_pid, &child_status, 0);
    c->status = WEXITSTATUS(child_status);
    }
    }
  */
}

void
execute_command (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  int pid;
  int child_status;
  int return_pid;
  switch(c->type) {
  case PIPE_COMMAND:
    execute_pipe(c, time_travel);
    break;
  case SIMPLE_COMMAND:
    execute(c);
    break;
  case AND_COMMAND:
    execute_command(c->u.command[0], time_travel);
    if (c->u.command[0]->status == 0) {
      execute_command(c->u.command[1], time_travel);
      c->status = c->u.command[1]->status;
    }
    else {
      c->status = c->u.command[0]->status;
    }
    break;
  case OR_COMMAND:
    execute_command(c->u.command[0], time_travel);
    if (c->u.command[0]->status == 0)
      c->status = 0;
    else {
      execute_command(c->u.command[1], time_travel);
      c->status = c->u.command[1]->status;
    }
      
    break;
  case SEQUENCE_COMMAND:
    execute_command(c->u.command[0], time_travel);
    execute_command(c->u.command[1], time_travel);
    c->status = 0;
    break;
  case SUBSHELL_COMMAND:
    pid = fork();
    if (pid == 0) {
      if (c->input != NULL)
	freopen(c->input, "r", stdin);
      if (c->output != NULL)
	freopen(c->output, "w", stdout);
      execute_command(c->u.subshell_command, time_travel);    
      c->status = c->u.subshell_command->status;
      exit(c->status);
    }
    else {
      return_pid = waitpid(pid, &child_status, 0);
      c->status = WEXITSTATUS(child_status);
  
      //exit(0);
    }
    break;
  }
  //error (1, 0, "command execution not yet implemented");
}

void
execute_command_nf (command_t c, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  int pid;
  int child_status;
  int return_pid;
  switch(c->type) {
  case PIPE_COMMAND:
    execute_pipe(c, time_travel);
    break;
  case SIMPLE_COMMAND:
    execute_nf(c);
    break;
  case AND_COMMAND:
    execute_command(c->u.command[0], time_travel);
    if (c->u.command[0]->status == 0) {
      execute_command(c->u.command[1], time_travel);
      c->status = c->u.command[1]->status;
    }
    else {
      c->status = c->u.command[0]->status;
    }
    break;
  case OR_COMMAND:
    execute_command(c->u.command[0], time_travel);
    if (c->u.command[0]->status == 0)
      c->status = 0;
    else {
      execute_command(c->u.command[1], time_travel);
      c->status = c->u.command[1]->status;
    }
      
    break;
  case SEQUENCE_COMMAND:
    execute_command(c->u.command[0], time_travel);
    execute_command(c->u.command[1], time_travel);
    c->status = 0;
    break;
  case SUBSHELL_COMMAND:
    //pid = fork();
    //if (pid == 0) {
      if (c->input != NULL)
	freopen(c->input, "r", stdin);
      if (c->output != NULL)
	freopen(c->output, "w", stdout);
      //TODO: don't know if this should be _nf or not
      execute_command(c->u.subshell_command, time_travel);    
      c->status = c->u.subshell_command->status;
      exit(c->status);
      //}
      //else {
      //return_pid = waitpid(pid, &child_status, 0);
      //exit(0);
      //}
    
    break;
  }
  //error (1, 0, "command execution not yet implemented");
}
