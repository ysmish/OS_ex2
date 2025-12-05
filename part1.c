#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int fdout;
    ssize_t charsw;  //how manny chars were written
    
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <parent_message> <child1_message> <child2_message> <count>\n", argv[0]);
        return 1;
    }
    const char *parent_message = argv[1];
    const char *child1_message = argv[2];
    const char *child2_message = argv[3];
    int times_to_write = atoi(argv[4]);

    fdout = open("output.txt", O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fdout < 0) {
        perror("after create");
        exit(-1);
    }

    pid_t pid1, pid2;

    if ((pid1 = fork()) < 0) { //create child1
        perror("fork error");
        return 1;
    }

    if (pid1 == 0) {
        sleep(1);//child1
        for (int i = 0; i < times_to_write; i++) {
            charsw = write(fdout, child1_message, strlen(child1_message));
            if (charsw < 0) {
                perror("write error in child1");
                exit(-1);
            }
        }
        close(fdout);
        exit(0);
    }

    //parent continues and creates the second child 
    if ((pid2 = fork()) < 0) {
        perror("fork error");
        return 1;
    }

    if (pid2 == 0) {
        //child2
        sleep(3);
        for (int i = 0; i < times_to_write; i++) {
            charsw = write(fdout, child2_message, strlen(child2_message));
            if (charsw < 0) {
                perror("write error in child2");
                exit(-1);
            }
        }
        close(fdout);
        exit(0);
    }

    //wait for both children
    if (waitpid(pid1, NULL, 0) != pid1)
        perror("waitpid error for child1");

        
    if (waitpid(pid2, NULL, 0) != pid2)
        perror("waitpid error for child2");

    for (int i = 0; i < times_to_write; i++) {
        charsw = write(fdout, parent_message, strlen(parent_message));
        if (charsw < 0) {
            perror("write error in parent");
            exit(-1);
        }
    }

    close(fdout);
    return 0;
}