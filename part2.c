#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define LOCK_FILE "lockfile.lock"

int LOCK_FILE_fd = -1;   //file descriptor for the lock file

void lock(void);
void unlock(void);
void writemessage(const char *, int);

int main(int argc, char *argv[]) {
    if (argc <= 4) {
        fprintf(stderr, "Usage: %s <message1> <message2> ... <count>", argv[0]);
        return 1;
    }
    int times_to_write = atoi(argv[argc - 1]);
    int num_of_children = argc - 2;
    pid_t *pids = malloc(num_of_children * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < num_of_children; i++) {//fork all children
        pid_t pid;
        if ((pid = fork()) < 0) {
            perror("fork error");
            free(pids);
            return 1;
        }
        if (pid == 0) {//for nth child process
            setbuf(stdout, NULL);
            lock();
            writemessage(argv[i + 1], times_to_write);
            unlock();
            exit(0);
        }
        pids[i] = pid; //save child's pid and continue loop
    }
    //parent waits for children
    for (int i = 0; i < num_of_children; i++) {
        if (waitpid(pids[i], NULL, 0) != pids[i])
            perror("waitpid error for child");
    }

    free(pids);
    return 0;
}

void lock(void) {
    int fd;
    while ((fd = open(LOCK_FILE, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1) {
        if (errno != EEXIST) {
            perror("open lockfile");
            exit(EXIT_FAILURE);
        }
        usleep(100);//wait and try again if lock exists
    }
    LOCK_FILE_fd = fd; //save acquired fd
}

void unlock(void) {
    if (LOCK_FILE_fd >= 0) {
        close(LOCK_FILE_fd);
        LOCK_FILE_fd = -1;
    }
    unlink(LOCK_FILE);
}

void writemessage(const char *message, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", message);
        usleep((rand() % 100) * 1000); // Random delay between 0 and 99 milliseconds
    }
}
