#include "kernel/types.h"
#include "user/user.h"

#define MAX_BUF_SIZE 32

int main(int argc, char ** argv) {

    int pipe_fds[2];
    int nr_read = 0;
    int nr_write = 0;
    char buf[MAX_BUF_SIZE];

    pipe(pipe_fds);

    if (fork() == 0) {
        // child

        nr_read = read(pipe_fds[0], buf, MAX_BUF_SIZE);
        if (nr_read > 0)
            printf("%d: received ping\n", getpid());

        nr_write = write(pipe_fds[1], "ping-pong", MAX_BUF_SIZE);
        if (nr_write < 0)
            exit(1);

    } else {
        // parent

        nr_write = write(pipe_fds[1], "ping-pong", MAX_BUF_SIZE);
        if (nr_write < 0)
            exit(1);

        nr_read = read(pipe_fds[0], buf, MAX_BUF_SIZE);
        if (nr_read > 0)
            printf("%d: received pong\n", getpid());

    }

    exit(0);
}