#include <stdio.h>
#include <string.h>
#include <csetjmp>
#include "Particle.h"

extern "C" {
#include "microrl/microrl.h"
}
#include "microrl_host.hpp"
#include "zforth_host.hpp"

// create microrl object and pointer on it
microrl_t rl;
microrl_t *prl = &rl;
static void (*executor) (const char *) = NULL;

//*****************************************************************************
// print to stream callback
void mrl_print(const char * str) {
	Serial.printf("\033[32m");
	Serial.write(str);
	Serial.printf("\033[0m");
}

static char phantom = 0;
// get_char from stream
char mrl_get_char(jmp_buf *no_input) {
	if(phantom) {
		char tmp = phantom;
		phantom = 0;
		return tmp;
	}
	if(Serial.available()) {
		char input = (char) Serial.read();
		if(input == '\r' || input == '\n') {
			input = '\r';
			phantom = '\n';
		}
		return input;
	} else {
		longjmp(*no_input, 1);
	}
}

// execute callback
int mrl_execute(int argc, const char * const * argv) {
	if (executor) {
		int len = 0;
		int i;
		for(i = 0; i < argc; i++) {
			len += strlen(argv[i]) + !!i;
		}
		char * const line = (char * const) malloc(len);
		char *head = line;
		for(i = 0; i < argc; i++) {
			if(i != 0) {
				*head = ' ';
				head++;
			}
			strcpy(head, argv[i]);
			head += strlen(argv[i]);
		}
		(*executor)(line);
		free(line);
		zforth_line++;
		Serial.printf("\r\n");
	}
	return 0;
}

// completion callback
char ** mrl_complet(int argc, const char * const * argv) {
	char **list = (char **) malloc(sizeof (char *));
	list[0] = NULL;
	return list;
}

// ctrl+c callback
void mrl_sigint(void) {
	Serial.printf("\033[33m");
	Serial.write("SIGINT\r\n");
	Serial.printf("\033[0m");
}

// init platform
void mrl_init(void (*execute_word) (const char *)) {
	executor = execute_word;

	// call init with ptr to microrl instance and print callback
	microrl_init(prl, mrl_print);

	// set callback for execute
	microrl_set_execute_callback(prl, mrl_execute);

	#ifdef _USE_COMPLETE
	// set callback for completion
	microrl_set_complete_callback(prl, mrl_complet);
	#endif
	#ifdef _USE_CTLR_C
	// set callback for Ctrl+C
	microrl_set_sigint_callback(prl, mrl_sigint);
	#endif
}

void mrl_process_input(jmp_buf *no_input) {
	microrl_insert_char(prl, mrl_get_char(no_input));
}
