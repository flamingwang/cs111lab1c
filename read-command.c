#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdbool.h>  // for bool types
#include <stdio.h>
#include <ctype.h>    // for isalnum()
#include <string.h>   // for strcpy()


int lin_num = 1;//global for line numbers

// Returns true if character is valid based on the spec
bool is_valid_char(char character) {
  if(isalnum(character))
    return true;

  switch(character)
  {
    case '!':
    case '%':
    case '+':
    case ',':
    case '-':
    case '.':
    case '/':
    case ':':
    case '@':
    case '^':
    case '_':
      return true;
    default:
      return false;
  }
}

// Returns precedence of operator
int get_precedence(int type){
  if(type == PIPE_COMMAND)
    return 3;
  if(type == AND_COMMAND || type == OR_COMMAND)
    return 2;
  if(type == SEQUENCE_COMMAND)
    return 1;
  fprintf(stderr, "unknown precedence detected");
  return -1;
}

// Returns true if the passed in enum/int is an operator
bool is_operator(int type){
  return (type == PIPE_COMMAND || type == AND_COMMAND || type == OR_COMMAND || type == SEQUENCE_COMMAND);
}

//////////////////////////////////////////////////////////////
///////////////////  Token Implementation  ///////////////////
//////////////////////////////////////////////////////////////

struct token {
  token_type type;
  char* words;
  int lin_num;
};

struct token_list {
  token m_token;
  token_list_t m_next;
  token_list_t m_prev;
};

// Add token to linked list
void add_token(token* to_add, token_list_t* head) {

  // empty list
  if((*head) == NULL) {
    (*head) = (token_list_t) checked_malloc(sizeof(token_list));

    // initialize node
    (*head)->m_token.type = to_add->type;
    (*head)->m_token.lin_num = to_add->lin_num;

    if(to_add->type == WORD) {
      int len = strlen(to_add->words);

      //(*head)->m_token.words = checked_malloc((sizeof(char) * len) + 1);
      //changed to below for clarity \/
      (*head)->m_token.words = checked_malloc(sizeof(char) * (len + 1) );
      strcpy((*head)->m_token.words, to_add->words);
      //to_add->words[len] = '\0'; // This line causes the segfault
      (*head)->m_token.words[len] = '\0'; //meant this instead (I think)
    }
    else 
      (*head)->m_token.words = NULL;

    (*head)->m_next = NULL;
    (*head)->m_prev = NULL;
  }
  else // !empty list
  {
    token_list_t p = (*head);

    // seek p to point to the last node
    for(; p->m_next != NULL; p = p->m_next) {}

    // initialize new node
    p->m_next = (token_list_t) checked_malloc(sizeof(token_list));

    token_list_t last_node = p->m_next;

    last_node->m_token.type = to_add->type;
    last_node->m_token.lin_num = to_add->lin_num;

    if(to_add->type == WORD)
    {
      int len = strlen(to_add->words);

      last_node->m_token.words = checked_malloc((sizeof(char) * len) + 1);
      strcpy(last_node->m_token.words, to_add->words);
      last_node->m_token.words[len] = '\0';
    }
    else 
      last_node->m_token.words = NULL;

    last_node->m_next = NULL;
    last_node->m_prev = p;
  }
}

// Print all tokens in list for the sake of debugging
// Prints one out on each line in the following format:
// Type: token_type Line Number: lin_num Words: "Words"
void print_token_list(token_list_t token_list) {

  token_list_t ptr = token_list;

  // buffer to hold name of token
  int max_size_string = 10;
  char* token_type_name = checked_malloc(sizeof(char) * max_size_string);
  int count = 0;

  while(ptr != NULL) {
    switch(ptr->m_token.type) {
      case SEMICOLON:
        strcpy(token_type_name, ";");
        break;
      case OR:
        strcpy(token_type_name, "||");
        break;
      case AND:
        strcpy(token_type_name, "&&");
        break;
      case PIPE:
        strcpy(token_type_name, "|");
        break;  
      case LEFT_PAREN:
        strcpy(token_type_name, "(");
        break;
      case RIGHT_PAREN:
        strcpy(token_type_name, ")");
        break;
      case LEFT_ARROW:
        strcpy(token_type_name, "<");
        break;
      case RIGHT_ARROW:
        strcpy(token_type_name, ">");
        break;
      case WORD:
        strcpy(token_type_name, "Word");
        break;
      case NEWLINE:
        strcpy(token_type_name, "\\n");
        break;
      case UNKNOWN:
        strcpy(token_type_name, "Unknown");
        break;
      default:
        break;
    }

    // Print report one per line
    fprintf(stdout, "Type: %s Line Number: %i Words: %s \n", token_type_name, ptr->m_token.lin_num, ptr->m_token.words);
    
    // Increment Variables
    count++;
    ptr = ptr->m_next;
  }

  fprintf(stdout, "Total number of tokens : %i \n", count);

  free (token_type_name);
  return;
}

// Frees all tokens in a list
void free_token_list(token_list_t head) {

  token_list_t nxt_ptr = head->m_next;
  token_list_t cur_ptr = head;

  while(nxt_ptr != NULL) {
    free(cur_ptr);

    cur_ptr = nxt_ptr;
    nxt_ptr = nxt_ptr->m_next;
  }
  free(cur_ptr);
}

//////////////////////////////////////////////////////////////
///////////////////  Stack Implementation  ///////////////////
//////////////////////////////////////////////////////////////

struct st_node {
  st_node_t m_next;
  st_node_t m_prev;
  command_t m_data;
};

struct stack {
  int m_size;
  st_node_t m_top;
};

// Initial stack constructor
stack2_t init_stack() {
  stack2_t stack = checked_malloc(sizeof(stack));
  stack->m_size = 0;
  stack->m_top = NULL;

  return stack;
}

// Pushes command onto given stack
void push(command_t to_add, stack2_t stack){

  st_node_t top_node;

  // empty stack case
  if(stack->m_size == 0) {
    stack->m_top = checked_malloc(sizeof(st_node));

    top_node = stack->m_top;
    top_node->m_next = NULL;
    top_node->m_prev = NULL;
    top_node->m_data = to_add;

    // increment size
    stack->m_size++;

    return;
  }

  // allocate space for a new top_node
  top_node = stack->m_top;
  top_node->m_next = checked_malloc(sizeof(st_node));

  // For readability
  st_node_t new_top_node = top_node->m_next;

  new_top_node->m_next = NULL;
  new_top_node->m_prev = top_node;
  new_top_node->m_data = to_add;

  // Set Stack top pointer up one node
  stack->m_top = new_top_node;

  // increment size
  stack->m_size++;

  return;
}

// Removes a command from the stack and returns it
command_t pop(stack2_t stack) {

  // Grab command from top node
  st_node_t top_node = stack->m_top;
  command_t to_pop = top_node->m_data;

  // Set stack top pointer back one node
  stack->m_top = top_node->m_prev;
  stack->m_size--;
  // free node
  free(top_node);

  return to_pop;
}

// Accessor Method to look at top of stack
command_t peek(stack2_t stack) {
  return stack->m_top->m_data;
}

// Returns true if the stack is empty
bool isEmpty(stack2_t stack) {
  if(stack->m_top == NULL)
    return true;
  else
    return false;
}

// For debugging purposes
void print_stack(stack2_t stack) {

  int MAX_SIZE = LEFT_PAREN_COMMAND;
  char* command_name = checked_malloc(sizeof(char) * MAX_SIZE);

  int i = 0;
  st_node_t ptr = stack->m_top;

  fprintf(stdout, "TOP OF STACK: \n");
  for (;i < stack->m_size; i++) {

    switch(ptr->m_data->type) {
      case AND_COMMAND:
        strcpy(command_name, "AND_COMMAND");
        break;
      case SEQUENCE_COMMAND:
        strcpy(command_name, "SEQUENCE_COMMAND");
        break;
      case OR_COMMAND:
        strcpy(command_name, "OR_COMMAND");
        break;
      case PIPE_COMMAND:
        strcpy(command_name, "PIPE_COMMAND");
        break;
      case SIMPLE_COMMAND:
        strcpy(command_name, "SIMPLE_COMMAND");
        break;
      case SUBSHELL_COMMAND:
        strcpy(command_name, "SUBSHELL_COMMAND");
        break;
    }

    fprintf(stdout, "Command #%i -> Type : %s \n", i + 1, command_name);

    ptr = ptr->m_prev;
  }

  free(command_name);
}

// Test function 
void test_stack() {

  stack2_t stack = init_stack();
  int i = 0;
  for(;i < 3; i++)
  {
    command_t temp = checked_malloc(sizeof(command));
    temp->type = AND_COMMAND;

    push(temp, stack);
  }

  print_stack(stack);

  fprintf(stdout, "Is it empty? : %s \n", isEmpty(stack) ? "true" : "false");

  free_stack(stack);

}

// Frees allocated memory
void free_stack(stack2_t stack) {

  int i = 0;
  for(; i < stack->m_size; i++) {
    st_node_t temp = stack->m_top;

    stack->m_top = stack->m_top->m_prev;

    free(temp);
  }
}

//////////////////////////////////////////////////////////////
/////////////  Command Stream Implementation  ////////////////
//////////////////////////////////////////////////////////////

struct node {
  command_t m_dataptr;
  node_t m_next;
};

struct command_stream {
  node_t m_head;   // holds head of linked list
  node_t m_curr;   // holds iterater
  int m_size;
};

// Initial Constructor Method
void initialize_stream(command_stream_t m_command_stream){
  m_command_stream->m_head = NULL;
  m_command_stream->m_curr = NULL;
}

// Add command to the given linked list
void add_command(command_t to_add_command, command_stream_t m_command_stream) {
  // empty stream
  if(m_command_stream->m_head == NULL) {
    
    m_command_stream->m_curr = (node_t) checked_malloc(sizeof(node));
    m_command_stream->m_curr->m_dataptr = to_add_command;
    m_command_stream->m_curr->m_next = NULL;
    m_command_stream->m_head = m_command_stream->m_curr;
  }
  else // !empty stream
  {
    m_command_stream->m_curr->m_next = (node_t) checked_malloc(sizeof(node));
    m_command_stream->m_curr = m_command_stream->m_curr->m_next;
    
    m_command_stream->m_curr->m_dataptr = to_add_command;
    m_command_stream->m_curr->m_next = NULL;
  }

  m_command_stream->m_size++;
  return;
}

// Manipulate curr pointer to traverse
void reset_traverse(command_stream_t cStream){
  cStream->m_curr = cStream->m_head;
}

// Returns current command pointed to by m_curr
// Then iterates m_curr forward one
command_t traverse(command_stream_t cStream){
  if(cStream == NULL){
    fprintf(stderr, "NULL Command Stream");
    return NULL;
  }
  if(cStream->m_head == NULL && cStream->m_curr == NULL){
    fprintf(stderr, "Command Stream Empty");
    return NULL;
  }
  if(cStream->m_curr == NULL){
    //fprintf(stderr, "End of Stream");
    return NULL;
  }
  
  // Iterate grab command and iterate m_curr
  else{
    command_t cmd = checked_malloc(sizeof(struct command));
    cmd->type = cStream->m_curr->m_dataptr->type;
    cmd->status = cStream->m_curr->m_dataptr->status;
    cmd->input = cStream->m_curr->m_dataptr->input;
    cmd->output = cStream->m_curr->m_dataptr->output;
    cmd->u = cStream->m_curr->m_dataptr->u;
    cStream->m_curr = cStream->m_curr->m_next;
    //fprintf(stderr, "Traverse output is: %d\n",cmd->type);
    return cmd;
  }
  
  // This should never execute
  fprintf(stderr, "ERROR in stream traversal\n"); 
  return NULL;
}

// Dumps contents for debugging
void  print_stream(command_stream_t cStream){
  if(cStream->m_head == NULL && cStream->m_curr == NULL){
    fprintf(stderr, "Command Stream Empty\n");
  }
  else{
    node_t iter = cStream->m_head;
    while(iter){
      fprintf(stderr,"Type of command is: %d\n",iter->m_dataptr->type);
      iter = iter->m_next;
    }
    fprintf(stderr,"SEPARATE\n");
  }
}

// Frees up allocated memory
void free_stream(command_stream_t m_command_stream) {
  int i = 0;
  node_t cur_ptr = m_command_stream->m_head;
  node_t next_ptr = cur_ptr->m_next;

  while (cur_ptr != NULL) {
    free(cur_ptr->m_dataptr);
    free(cur_ptr);

    cur_ptr = next_ptr;
    next_ptr = cur_ptr->m_next;
  }
}

//////////////////////////////////////////////////////////
///////////////////  Additional Functions  ///////////////
//////////////////////////////////////////////////////////

// Read file into buffer and preprocess the characters:
// Comments removed
char* read_file_into_buffer(int (*get_next_byte) (void *), void *get_next_byte_argument) 
{
  size_t capacity = 1024; // arbitrary size
  size_t iter = 0;
  char* buffer = (char*) checked_malloc(capacity);
  char next_byte;
  
  
  
  // loop through entire file
  bool isFileBeginning = true;
  while ((next_byte = get_next_byte(get_next_byte_argument)) != EOF) {
    
    //Trim newlines on beginning of file but preseve line number
    if(next_byte == '\n' && isFileBeginning){
      lin_num++;
      continue;
    }
    
    // Comment: loop until newline
    if(next_byte == '#') {
      while ((next_byte = get_next_byte(get_next_byte_argument)) != '\n' && next_byte != EOF) 
      {
	if (next_byte == EOF)
	  break;
        // test
        // printf("%c", next_byte);
      }
      lin_num++;
      // get next byte disregard '\n'
      continue;
    }
    
    if(next_byte != '\n' && next_byte != '#')
      isFileBeginning = false;
  
    // store in buffer
    buffer[iter++] = next_byte;

    // if buffer full
    if (capacity == iter)
      buffer = (char*) checked_grow_alloc(buffer, &capacity);
  }

  // null terminate buffer 
  buffer[iter] = '\0';
  
  // test  
  // printf("%s", buffer);

  return buffer;
}

// Converts input buffer into a linked list of tokens 
// This helps to categorize the inputs
token_list_t convert_to_tokens(char* buffer) {

  // list info
  token_list_t m_head = NULL;
  token_list_t p = m_head;

  // token info
  token_type type;
  //int lin_num = 1;//make this a global so it actually gives the correct line number
  
  // iteration variables
  int iter = 0;
  char current;
  char next;

  // empty buffer case
  if(buffer[iter] == '\0')
    return NULL;

  // While we are not at the end of the buffer
  while (buffer[iter] != '\0') {

    current = buffer[iter];
    next = buffer[iter + 1];

    switch(current) {
      case '&':
        // correctly have '&&'
        if(current == next) {
          type = AND;
          iter++; // inc to account for one '&'
        }
        else
          type = UNKNOWN;

        break;

      case '|':
        if(current == next) {
          type = OR;
          iter++;
        }
        else
          type = PIPE;

        break;

      case ';':
        type = SEMICOLON;
        break;

      case '(':
        type = LEFT_PAREN;
        break;

      case ')':
        type = RIGHT_PAREN;
        break;

      case '<':
        type = LEFT_ARROW;
        break;

      case '>':
        type = RIGHT_ARROW;
        break;

      // skip over newlines and increment linum
      case '\n':
        type = NEWLINE;

        // increment lin_num
        lin_num++; 

        break;

      // skip over whitespace
      case ' ':
      case '\t':
        iter++;
        continue;
      default:
        type = UNKNOWN;
        break;
    }

    // Word Case
    int len = 1;
    int word_begin = iter;
    if (is_valid_char(current)) {
      type = WORD;
      // loop through word and keep track of length
      for(; is_valid_char(buffer[iter + len]); len++) {}

      // advance iterator to last character of word
      iter += len - 1;
    }

    // Now add token to linked list
    token* temp_token = (token*) checked_malloc(sizeof(token));
    temp_token->type = type;
    temp_token->lin_num = lin_num;

    // Print error message
    if (type == UNKNOWN) {
      fprintf(stderr, "Error: Line %i: Unknown Token -> %c \n", lin_num, current);
      exit(1);
    }
    else if (type == WORD) {
      // Allocate memory for full string
      temp_token->words = (char*) checked_malloc((sizeof(char) * len) + 1); //+1 for '\0'
      int i = 0;

      // Store characters
      for(; i < len; i++) {
        temp_token->words[i] = buffer[word_begin + i];
      }

      // Null terminate string
      temp_token->words[i] = '\0';
    }
    else 
      temp_token->words = NULL;
  
    add_token(temp_token, &m_head);

    free(temp_token);
    iter++;
  }

  return m_head;
}

// Checks passed in token_list to verify that token ordering is syntactically valid
void check_token_list(token_list_t token_list) {

  // Null pointer case
  if (token_list == NULL){
    fprintf(stderr, "Error: token_list is NULL");
    exit(1);
  }

  token_list_t curr_ptr = token_list;
  token_list_t prev = NULL;  // Init after moving forward one

  token next_token, prev_token, curr_token;


  int paren_count = 0; // For subshell depth
  FILE* f_ptr;  // For IO redirect < >

  // while there are tokens in the list
  while (curr_ptr != NULL) {

    curr_token = curr_ptr->m_token;

    if(curr_ptr->m_next != 0) {
      next_token = curr_ptr->m_next->m_token;
    }
    if(curr_ptr->m_prev != 0) {
      prev_token = curr_ptr->m_prev->m_token; 
    }

    switch (curr_token.type) {
      
      // Cannot be first token or appear consecutively
      case SEMICOLON:
        if (curr_ptr->m_next != NULL) {
          // Consecutive semicolon case
          if (next_token.type == SEMICOLON) {
            fprintf(stderr, "Error: Line %i: Semicolons cannot appear consecutively", curr_token.lin_num);
            exit(1);
          }
          // Semicolons are treated as newlines if not inside a subshell
          if (paren_count == 0) {
            curr_ptr->m_token.type = NEWLINE;
          }
          // Cannot be first token to appear
          if (curr_ptr->m_prev == NULL) {
            fprintf(stderr, "Error: Line %i: Semicolons cannot be the first token to appear", curr_token.lin_num);
            exit(1);
          }
        }
        // End of list
        else 
          prev->m_next = NULL;
        break;

      // Newline Case: Can be before parenthesis or filenames 
      case NEWLINE:
        if (curr_ptr->m_next != NULL) {
          // If next token is a word or paren
          if((next_token.type == WORD) || (next_token.type == LEFT_PAREN) || (next_token.type == RIGHT_PAREN)) {
            // Newline Cannot be before IO Redirection
            if((prev_token.type == LEFT_ARROW) || (prev_token.type == RIGHT_ARROW)) {
              fprintf(stderr, "Error: Line %i: Newline cannot be before IO Redirection < >", curr_token.lin_num);
              exit(1);
            }
            // break out of switch statement to continue iteration
            else if ((prev_token.type == AND) || (prev_token.type == OR) || (prev_token.type == PIPE) || (prev_token.type == SEMICOLON)) {
              break;
            }
            // Commands Split by newline therefore can be seen as a semicolon
            else if ((paren_count > 0) && (prev_token.type != LEFT_PAREN)) 
              curr_ptr->m_token.type = SEMICOLON;
          }
          // Consecutive Newlines
          else if (next_token.type == NEWLINE) {
            prev = curr_ptr;
            curr_ptr = curr_ptr->m_next;
            continue;
          }
          else {
            fprintf(stderr, "Error: Line %i: Newline can only be followed by file names and parenthesis", curr_token.lin_num);
            exit(1);
          }
        }
        // end of list
        else
          prev->m_next = NULL;
        // IO redirection error testcase
        if ((prev_token.type == RIGHT_ARROW) || (prev_token.type == LEFT_ARROW)) {
          fprintf(stderr, "Error: Line %i: Newline cannot follow IO Redirection < >", curr_token.lin_num);
          exit(1);
        }
        break;

      // IO Redirect Case: must always be followed by a word which represents
      // a filename. If looking for input file it must already exist
      case LEFT_ARROW:
      case RIGHT_ARROW:
        if((next_token.type != WORD) || (curr_ptr->m_next == NULL)) {
          fprintf(stderr, "Error: Line %i: IO Redirection must be followed by a word \n", curr_token.lin_num);
          exit(1);
        }
	if(prev == NULL) {
	  fprintf(stderr, "Error: Line %i : IO Redirection must be surrounded by words", curr_token.lin_num);
	  exit(1);
	}
        // Check to see if there is an existing file
        if(curr_token.type == LEFT_ARROW) {
          if((f_ptr = fopen(next_token.words, "r"))) {
            fclose(f_ptr);
          }
          // No existing file for IO redirection
          else  {
            // Print out message but don't end execution so test cases will pass
            // fprintf(stderr, "Error: Line %i: For '<', no file to accept input from found", curr_token.lin_num);
            // exit(1);
          }
        }
        break;
      
      // increase scope
      case LEFT_PAREN:
        paren_count++;
        break;

      // decrease scope
      case RIGHT_PAREN: 
          paren_count--;
        break;

      case OR:
        if(next_token.type == PIPE || prev == NULL) {
	  fprintf(stderr, "Error: Line %i : Or operator must be surrounded by commands", curr_token.lin_num);
	  exit(1);
	}
	if(next_token.type == NEWLINE) {
	  // Now iterate through next tokens until something other than newline seen
	  token_list_t temp = curr_ptr;
	  bool isEOF = true;

	  // loop through list
	  while(temp->m_next != NULL) {
      
	    // if something is not a newline break
	    if(temp->m_next->m_token.type != NEWLINE) {
    		isEOF = false;
    		break;
	    }

	    temp = temp->m_next;
	  }
	  if(isEOF) {
	    fprintf(stderr, "Error: Line %i : End of file reached after operator", curr_token.lin_num);
	    exit(1);
	  }
	}
	break;

      case AND:
	if(prev == NULL) {
	  fprintf(stderr, "Error: Line %i : And operator must be surrounded by commands",curr_token.lin_num);
	  exit(1);
	}
	break;

      case PIPE:
	if(prev == NULL) {
	  fprintf(stderr, "Error: Line %i : Pipe operator must be surrounded by commands", curr_token.lin_num);
	  exit(1);
	}
	break;

      default:
        break;
    }

    // Increment iterating variables
    prev = curr_ptr;
    curr_ptr = curr_ptr->m_next;    
  }

  // not equal amount of parenthesis
  if(paren_count != 0) {
    fprintf(stderr, "Error: Missing an accompanying right parenthesis");
    exit(1);
  }
}

// Initializes a command and returns it
command_t form_basic_command(int type){

  command_t cmd = checked_malloc(sizeof(struct command));
  cmd->type = type;
  cmd->status = -1;
  cmd->input = NULL;
  cmd->output = NULL;

  switch(type){
    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
    case AND_COMMAND:
    case OR_COMMAND:
    {
      cmd->u.command[0] = NULL; // parse this later
      cmd->u.command[1] = NULL; // parse this later
    }
      break;

    default:
      cmd->u.subshell_command = NULL;
      break;
  }
  return cmd;
}

// Creates a simple linked list of commands with no depth
command_stream_t make_basic_stream(token_list_t tList){

  // Allocate memory
  command_stream_t cStream = checked_malloc(sizeof(struct command_stream));
  
  // Instantiate stream
  initialize_stream(cStream);

  while(tList != NULL){
    switch(tList->m_token.type){

      // WORD tokens to commands
      case WORD:
      {
      	command_t cmd = checked_malloc(sizeof(struct command));
      	cmd->type = SIMPLE_COMMAND;
      	cmd->status = -1;
      	cmd->input = NULL;
      	cmd->output = NULL;
      	int num_words = 1;
      	
      	token_list_t wordPtr = tList;
      	
      	while(wordPtr->m_next != NULL && wordPtr->m_next->m_token.type == WORD){
      	  num_words++;
      	  wordPtr = wordPtr->m_next;
      	  if(wordPtr->m_next == NULL){break;}
      	}
            
      	cmd->u.word = (char**)checked_malloc((num_words+1));
            
      	int ii = 0;
      	for(; ii < num_words; ii++){
      	  cmd->u.word[ii] = checked_malloc(sizeof(char) * (strlen(tList->m_token.words)+1) );
      	  strcpy(cmd->u.word[ii], tList->m_token.words);
      	  tList = tList->m_next;
      	 }
      	 cmd->u.word[num_words] = NULL;
      	 add_command(cmd,cStream);
      } 
        break;
        
      case SEMICOLON:
      {
      	command_t cmd = form_basic_command(SEQUENCE_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 
      
        break;

      case PIPE:
      {
      	command_t cmd = form_basic_command(PIPE_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 

        break;

      case AND:
      {
      	command_t cmd = form_basic_command(AND_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 

      break;

      case OR:
      {
        command_t cmd = form_basic_command(OR_COMMAND);
        add_command(cmd,cStream);
        tList = tList->m_next;
      } 

        break;

      case LEFT_ARROW:
      {
      	//technically not a command type but parse this later
      	command_t cmd = form_basic_command(LEFT_ARROW_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 

        break;

      case RIGHT_ARROW:
      {
        //technically not a command type but parse this later
        command_t cmd = form_basic_command(RIGHT_ARROW_COMMAND);
        add_command(cmd,cStream);
        tList = tList->m_next;
      } 
        break;

      case LEFT_PAREN:
      {
      	//assume valid
      	command_t cmd = form_basic_command(LEFT_PAREN_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 
        break;

      case RIGHT_PAREN:
      {
      	//assume valid
      	command_t cmd = form_basic_command(RIGHT_PAREN_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
        } 

        break;

      case NEWLINE:
      {
      	command_t cmd = form_basic_command(NEWLINE_COMMAND);
      	add_command(cmd,cStream);
      	tList = tList->m_next;
      } 
        break;

      default:
  	    fprintf(stderr, "not supposed to end up here!!!");
    }

  }
  return cStream;
}

// Handles newlines cases given a simple command list
// Receives output of make_basic_stream
command_stream_t solve_newlines(command_stream_t nlStream) {
  //print_stream(nlStream);
  command_stream_t cStream = checked_malloc(sizeof(struct command_stream));
  initialize_stream(cStream);
  command_t cmd;
  bool prev_command_was_operator = false;
  
  reset_traverse(nlStream);
  cmd = traverse(nlStream);
  while(cmd != NULL){
   
    // Method:
    // if newline
    // 	 if prev_command_was_operator then ignore
    // 	 if at the end just return;
    //	 if next one also a newline create a new tree
    //	 if next is a regular then semicolon it
    // if not newline
    //	 if operator add operator and set prev command was op to true
    //	 if not operator just add regularly
    if(cmd->type == NEWLINE_COMMAND){ 
      if(prev_command_was_operator){
	      for(;;){
      	  cmd = traverse(nlStream);
      	  if(cmd == NULL){
      	    return cStream;
      	  }
      	  if(cmd->type != NEWLINE_COMMAND)
      	    break;
      	}

      	add_command(cmd,cStream);
      	prev_command_was_operator = false;
      }
      else {

      	cmd = traverse(nlStream);

      	if(cmd == NULL) {
      	  return cStream;
      	} 
      	if(cmd->type == NEWLINE_COMMAND) {//two or more newlines form a separate command tree
	  for(;;){
	    cmd = traverse(nlStream);
	    if(cmd == NULL)
	      return cStream;
	    if(cmd->type != NEWLINE_COMMAND)
	      break;
	  }
      	  add_command(form_basic_command(NEW_TREE_COMMAND) ,cStream);
	  continue;
      	}
      	else {
	  add_command(form_basic_command(SEQUENCE_COMMAND), cStream);
      	  add_command(cmd,cStream);
      	  if(is_operator(cmd->type))
      	    prev_command_was_operator = true;
      	}
      }
    }
    else {

      add_command(cmd,cStream);
      prev_command_was_operator = false;

      if(is_operator(cmd->type))
	      prev_command_was_operator = true;
    }

    cmd = traverse(nlStream);
  }  
  return cStream;
}

// Perhaps a way to deal with subshells
command_stream_t make_advanced_stream(command_stream_t basic_stream) {
  //print_stream(basic_stream);
  //print_stream(basic_stream);

  // Use stack algorithm and precedence to get final command_stream
  command_stream_t cStream = checked_malloc(sizeof(struct command_stream));
  initialize_stream(cStream);
  command_t cmd;
  stack2_t com_stack = init_stack();
  stack2_t op_stack = init_stack();
  
  bool finish_up = false;
  
  reset_traverse(basic_stream);
  cmd = traverse(basic_stream);

  while(cmd != NULL){
    if(cmd->type == NEW_TREE_COMMAND)
      finish_up = true;
    
    if(cmd->type == SIMPLE_COMMAND)
    {
      push(cmd, com_stack);
    }
    if(cmd->type == LEFT_PAREN_COMMAND){
      push(cmd, op_stack);
    }
    
    if(cmd->type == RIGHT_PAREN_COMMAND){
      command_t op = pop(op_stack);
      
      while(op->type != LEFT_PAREN_COMMAND){
	command_t com2 = pop(com_stack);
	command_t com1 = pop(com_stack);
	   
	command_t new_com = checked_malloc(sizeof(struct command));
	new_com->type = op->type;
	   
	new_com->status = -1;
	new_com->input = NULL;
	new_com->output = NULL;
	new_com->u.command[0] = com1;
	new_com->u.command[1] = com2;
	   
	push(new_com, com_stack);
	if(isEmpty(op_stack))
	  break;
	op = pop(op_stack);
      }
      
      command_t sub_com = pop(com_stack);//the sub command of the subshell
	  
      //subshell command creation    
      command_t subshell_com = checked_malloc(sizeof(struct command));
      subshell_com->type = SUBSHELL_COMMAND;
      subshell_com->status = -1;
      subshell_com->input = NULL;
      subshell_com->output = NULL;
      subshell_com->u.subshell_command = sub_com;
      push(subshell_com, com_stack);
      
    }
    
    if(is_operator(cmd->type)){
      if(isEmpty(op_stack)){
	      push(cmd, op_stack);
      }
      else {
	
      	while((peek(op_stack)->type != LEFT_PAREN_COMMAND) && (get_precedence(cmd->type) <= get_precedence(peek(op_stack)->type))) {
      	  
          command_t op = pop(op_stack);
      	  command_t com2 = pop(com_stack);
      	  command_t com1 = pop(com_stack);
      	  
      	  //combine the 3 commands into 1 command
      	  command_t new_com = checked_malloc(sizeof(struct command));
      	  new_com->type = op->type;
      	  
      	  new_com->status = -1;//maybe change later
      	  new_com->input = NULL;
      	  new_com->output = NULL;
      	  new_com->u.command[0] = com1;
      	  new_com->u.command[1] = com2;

      	  push(new_com, com_stack);
      	  if(isEmpty(op_stack))
      	    break;
      	  
      	}
      	push(cmd, op_stack);
      }
    }
    if(cmd->type == LEFT_ARROW_COMMAND || cmd->type == RIGHT_ARROW_COMMAND) {
      //If one of the arrow commands (LEFT_ARROW_COMMAND for left '<' RIGHT_ARROW_COMMAND for right '>')
      if( cmd->type == LEFT_ARROW_COMMAND ) {
      	//Assume correct tokens and next is a word
      	command_t in_com = pop(com_stack);
      	char* in_file = traverse(basic_stream)->u.word[0];
      	in_com->input = in_file;
      	push(in_com,com_stack);
      }
      else{
      	//Assume correct tokens and next is a word
      	command_t out_com = pop(com_stack);
      	char* out_file = traverse(basic_stream)->u.word[0];
      	out_com->output = out_file;
      	push(out_com,com_stack);
      }
    }
    if(finish_up) {
      while(!isEmpty(op_stack)) {
      	command_t op = pop(op_stack);
      	command_t com2 = pop(com_stack);
      	command_t com1 = pop(com_stack);
      	command_t new_com = checked_malloc(sizeof(struct command));
      	
      	new_com->type = op->type;
      	  
      	new_com->status = -1;//maybe change later
      	new_com->input = NULL;
      	new_com->output = NULL;
      	new_com->u.command[0] = com1;
      	new_com->u.command[1] = com2;
      	push(new_com, com_stack);
      }

      finish_up = false;
      add_command(pop(com_stack),cStream);
    }
    cmd = traverse(basic_stream);
  }
  
  while(!isEmpty(op_stack)) {
    command_t op = pop(op_stack);
    command_t com2 = pop(com_stack);
    command_t com1 = pop(com_stack);
    command_t new_com = checked_malloc(sizeof(struct command));
    
    new_com->type = op->type;
    
    new_com->status = -1;//maybe change later
    new_com->input = NULL;
    new_com->output = NULL;
    new_com->u.command[0] = com1;
    new_com->u.command[1] = com2;
    push(new_com, com_stack);
  }
  
  // Reached end of stream
  // Pop the command_t into the command stream
  add_command(pop(com_stack),cStream);
  reset_traverse(cStream);
  //print_stream(cStream);
  return cStream;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{  
  // Read data into buffer and preprocess
  char* buffer = read_file_into_buffer(get_next_byte, get_next_byte_argument);

  // Takes input buffer and creates a linked list of catagorized "tokens"
  token_list_t token_list = convert_to_tokens(buffer);

  // Null list checking
  if(token_list == NULL) {
    fprintf(stderr, "Error: No commands found in file\n");
    exit(1);
    return NULL;
  }
  
  // Check the list of tokens for syntax and ordering
  check_token_list(token_list);

  // Take use linked list of tokens to make a command stream
  return make_advanced_stream(solve_newlines(make_basic_stream(token_list)));
}

command_t
read_command_stream (command_stream_t s)
{
  // Assume initial run is on command stream with m_curr set to m_head
  // Run reset_traverse to set m_curr to m_head
  return traverse(s);
}
