//#include "Particle.h"
//#include <cstdint>
//#include <cstring>
//#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cmath>

extern "C" {
#include "zforth/zforth.h"
}
#include "zforth_host.hpp"
#include "zforth/forth.hpp"
#include "microrl_host.hpp"

zf_result do_eval(const char *src, int line, const char *buf) {
	const char *msg = NULL;

	zf_result rv = zf_eval(buf);

	switch(rv) {
		case ZF_OK: break;
		case ZF_ABORT_INTERNAL_ERROR: msg = "internal error"; break;
		case ZF_ABORT_OUTSIDE_MEM: msg = "outside memory"; break;
		case ZF_ABORT_DSTACK_OVERRUN: msg = "dstack overrun"; break;
		case ZF_ABORT_DSTACK_UNDERRUN: msg = "dstack underrun"; break;
		case ZF_ABORT_RSTACK_OVERRUN: msg = "rstack overrun"; break;
		case ZF_ABORT_RSTACK_UNDERRUN: msg = "rstack underrun"; break;
		case ZF_ABORT_NOT_A_WORD: msg = "not a word"; break;
		case ZF_ABORT_COMPILE_ONLY_WORD: msg = "compile-only word"; break;
		case ZF_ABORT_INVALID_SIZE: msg = "invalid size"; break;
		case ZF_ABORT_DIVISION_BY_ZERO: msg = "division by zero"; break;
		default: msg = "unknown error";
	}

	if(msg) {
		Serial.printf("\033[31m");
		if(src) Serial.printf("%s:%d: ", src, line);
		Serial.printf("%s\033[0m", msg);
	}

	return rv;
}

void include(const char *fname) {
	/*char buf[256];
	FILE *f = fopen(fname, "rb");
	int line = 1;
	if(f) {
		while(fgets(buf, sizeof(buf), f)) {
			do_eval(fname, line++, buf);
		}
		fclose(f);
	} else {
		fprintf(stderr, "error opening file '%s': %s\n", fname, strerror(errno));
	}*/
	Serial.printf("error opening file '%s': NOT_IMPLEMNTED\r\n", fname);
}

static void save(const char *fname) {
	/*size_t len;
	void *p = zf_dump(&len);
	FILE *f = fopen(fname, "wb");
	if(f) {
		fwrite(p, 1, len, f);
		fclose(f);
	}*/
	Serial.printf("error writing file '%s': NOT_IMPLEMNTED\r\n", fname);
}

/*static void load(const char *fname) {
	size_t len;
	void *p = zf_dump(&len);
	FILE *f = fopen(fname, "rb");
	if(f) {
		fread(p, 1, len, f);
		fclose(f);
	} else {
		perror("read");
	}
	Serial.printf("error reading file '%s': NOT_IMPLEMNTED\r\n", fname);
}*/

#define TRACE 0

static jmp_buf quit;
static jmp_buf no_input;
int zforth_line = 1;

void word_eval(const char* word) {
	do_eval("stdin", zforth_line, word);
}

void forth(void) {
	/* Initialize zforth */

	zf_init(TRACE);
	zf_bootstrap();
	//zf_eval(": . 1 sys ;");
	zf_eval(CORE_FORTH);

	mrl_init(&word_eval);

	/* Main loop: read words and eval */

	for(;!setjmp(quit);) {
		setjmp(no_input);
		if (mode_flag != FORTH) {
			break;
		}
		mrl_process_input(&no_input);
	}
	mode_flag = IDLE;
}

zf_input_state zf_host_sys(zf_syscall_id id, const char *input) {
	switch((int)id) {
		/* The core system callbacks */

		case ZF_SYSCALL_EMIT:
			Serial.write((char)zf_pop());
			Serial.flush();
			break;

		case ZF_SYSCALL_PRINT:
			Serial.printf(ZF_CELL_FMT " ", zf_pop());
			break;

		case ZF_SYSCALL_TELL: {
			zf_cell len = zf_pop();
			uint8_t *buf = (uint8_t *)zf_dump(NULL) + (int)zf_pop();
			Serial.write(buf, len);
			Serial.flush(); }
			break;


		/* Application specific callbacks */

		case ZF_SYSCALL_USER + 0:
			printf("\r\n");
			//mode_flag = IDLE;
			longjmp(quit, 1);
			break;

		case ZF_SYSCALL_USER + 1:
			zf_push(sin(zf_pop()));
			break;

		case ZF_SYSCALL_USER + 2:
			if(input == NULL) {
				return ZF_INPUT_PASS_WORD;
			}
			include(input);
			break;

		case ZF_SYSCALL_USER + 3:
			save("zforth.save");
			break;

		default:
			printf("unhandled syscall %d\r\n", id);
			break;
	}

	return ZF_INPUT_INTERPRET;
}


void zf_host_trace(const char *fmt, va_list va) {
	Serial.printf("\033[1;30m");
	int size = 1 + vsnprintf(NULL, 0, fmt, va);
	char *buf = (char *) malloc(size);
	vsnprintf(buf, size, fmt, va);
	int lines = 0;
	char *head = buf;
	while(*head) {
		if(*head == '\n') lines++;
		head++;
	}
	buf = (char *) realloc(buf, size + lines);
	char *rhead = &buf[size - 1];
	char *whead = &buf[size + lines - 1];
	while(rhead != whead) {
		*whead = *rhead;
		if(*rhead == '\n') {
			whead--;
			*whead = '\r';
		}
		rhead--;
		whead--;
	}
	Serial.printf("%s", buf);
	free(buf);
	Serial.printf("\033[0m");
}

zf_cell zf_host_parse_num(const char *buf) {
	char *end;
	zf_cell v = strtof(buf, &end);
	if(*end != '\0') {
		zf_abort(ZF_ABORT_NOT_A_WORD);
	}
	return v;
}
