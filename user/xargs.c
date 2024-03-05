#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define BUF_SIZE 512

char *args[MAXARG];

int main(int argc, char *argv[]) {

	if (argc > MAXARG) {
		printf("xargs: too many args\n");
		exit(1);
	}

	// copy xargs's args
	int index = 0;
	for (int i = 1; i < argc; i++) {
		args[index] = argv[i];
		index++;
	}

	char buf[BUF_SIZE];
	char *p = buf;

	// read each byte into buf, from stdin
	while (read(0, p, 1) == 1) {
		if (*p == '\n') {
			*p = 0; // set '\n' to 0
			if (fork() == 0) {
				args[index] = buf; // append args read from stdin into args[]
				exec(argv[1], args);

			} else {
				// parent
				wait(0);
				// Continue reading from stdin, resetting buf and reset the pointer
				memset(buf, 0, sizeof(buf));
				p = buf;
			}
		} else {
			p++;
		}
	}

	exit(0);
}