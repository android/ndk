#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/sched.h>
#include <stdlib.h>

int v, fd;

int child_proc()
{
    v = 42;
    close(fd);
    exit(0);
}

#define STACK_SIZE 1024

int main(int argc, char *argv[])
{
    void **child_stack;
    char ch;

    v = 9;
    fd = open(argv[0], O_RDONLY);
    if (read(fd, &ch, 1) < 1) {
        printf("Can't read file");
        exit(1);
    }
    child_stack = (void **) malloc(STACK_SIZE * sizeof(void *));
    printf("v = %d\n", v);

    clone(child_proc, child_stack + STACK_SIZE, CLONE_VM|CLONE_FILES, NULL);
    sleep(1);

    printf("v = %d\n", v);
    if (read(fd, &ch, 1) < 1) {
        printf("Can't read file because it's closed by child.\n");
        return 0;
    } else {
        printf("We shouldn't be able to read from file which is closed by child.\n");
        return 0;
    }
}

