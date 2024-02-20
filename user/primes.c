#include "kernel/types.h"
#include "user/user.h"

#define MAX_NUMBER 35

void filter(int pipe_read) {
    int pipe_fds[2];
    int primes[MAX_NUMBER];
    int nr_read;
    int cnt = 0;

    while (1) {
        nr_read = read(pipe_read, & primes[cnt], sizeof(int));
        if (nr_read <= 0)
            break;

        cnt++;
    }
    close(pipe_read);

    if (cnt == 0)
        return;

    int first = primes[0];
    printf("prime %d\n", first);

    pipe(pipe_fds);
    for (int i = 1; i < cnt; i++) {
        if (primes[i] % first != 0) {
            write(pipe_fds[1], & primes[i], sizeof(int));
        }
    }
    close(pipe_fds[1]);

    if (fork() == 0) {
        // child
        filter(pipe_fds[0]);
    } else {
        // parent
        wait(0);
    }

}

int main(int argc, char * argv[]) {

    int pipe_fds[2];

    pipe(pipe_fds);

    for (int i = 2; i < MAX_NUMBER; i++) {
        write(pipe_fds[1], & i, sizeof(i));
    }
    close(pipe_fds[1]);

    filter(pipe_fds[0]);
    close(pipe_fds[0]);

    exit(0);
}