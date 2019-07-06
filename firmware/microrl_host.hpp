#ifndef MICRORL_HOST_HPP
#define MICRORL_HOST_HPP

#include <csetjmp>

// init platform
void mrl_init (void (*execute_word) (const char *));

// print to stream callback
void mrl_print (const char * str);

// get_char from stream
char mrl_get_char (void);

// execute callback
int mrl_execute (int argc, const char * const * argv);

// completion callback
char ** mrl_complet (int argc, const char * const * argv);

// ctrl+c callback
void mrl_sigint (void);

void mrl_process_input(jmp_buf *no_input);

#endif
