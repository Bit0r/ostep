#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

char **paths;

FILE *getcmdfile(int argc, char const *argv[]);
void eval(char *cmdline);
void run(int argc, char *argv[]);
char **getargv(char *cmdline, int *argc);
char *getfilepath(char *filename);
void setpaths(char *pathv[]);
void redirect(char *file, int *saved_stdout, int *saved_stderr);
void restore(int saved_stdout, int saved_stderr);

inline static void write_error() {
    write(STDERR_FILENO, "An error has occurred\n", 22);
}

int main(int argc, char const *argv[]) {
    char *cmdline = NULL, *subcmd, *chr;
    size_t n = 0;
    FILE *cmdfile = getcmdfile(argc, argv);

    paths = calloc(2, sizeof(char *));
    paths[0] = strdup("/bin");

    while (true) {
        if (cmdfile == stdin) {
            printf("wish> ");
        }

        getline(&cmdline, &n, cmdfile);

        if (feof(cmdfile)) {
            exit(EXIT_SUCCESS);
        }

        for (subcmd = cmdline; (chr = strchr(subcmd, '&')); subcmd = chr + 1) {
            *chr = '\0';
            eval(subcmd);
        }
        eval(subcmd);

        while (wait(NULL) != -1) {
        }
    }

    return 0;
}

FILE *getcmdfile(int argc, char const *argv[]) {
    FILE *cmdfile;

    if (argc == 1) {
        return stdin;
    } else if (argc == 2 && (cmdfile = fopen(argv[1], "r"))) {
        return cmdfile;
    } else {
        write_error();
        exit(EXIT_FAILURE);
    }
}

void eval(char *cmdline) {
    char *chr, *save_ptr, *file, **argv;
    int saved_stdout = -1, saved_stderr = -1, argc;

    chr = strchr(cmdline, '>');
    if (chr) {
        *chr = '\0';
    }

    argv = getargv(cmdline, &argc);

    if (chr) {
        file = strtok_r(chr + 1, " \t\n", &save_ptr);
        if (argc && file && !strtok_r(NULL, " \t\n", &save_ptr)) {
            redirect(file, &saved_stdout, &saved_stderr);
            run(argc, argv);
        } else {
            write_error();
        }
    } else if (argc) {
        run(argc, argv);
    }

    free(argv);
    if (saved_stdout != -1) {
        restore(saved_stdout, saved_stderr);
    }
}

void run(int argc, char *argv[]) {
    if (!strcmp(argv[0], "cd")) {
        if (argc != 2 || chdir(argv[1])) {
            write_error();
        }
    } else if (!strcmp(argv[0], "exit")) {
        if (argc > 1) {
            write_error();
        } else {
            exit(EXIT_SUCCESS);
        }
    } else if (!strcmp(argv[0], "path")) {
        setpaths(argv + 1);
    } else {
        char *filepath = getfilepath(argv[0]);
        if (filepath) {
            if (fork()) {
                free(filepath);
            } else {
                execv(filepath, argv);
            }
        } else {
            write_error();
        }
    }
}

char **getargv(char *cmdline, int *argc) {
    char *save_ptr, *arg, **argv;
    size_t len, cap = 1024;

    argv = calloc(cap, sizeof(char *));

    // 循环提取参数，并将参数填入 argv 数组
    arg = strtok_r(cmdline, " \t\n", &save_ptr);
    for (len = 0; arg; arg = strtok_r(NULL, " \t\n", &save_ptr)) {
        // 长度达到容量上限时进行扩容
        if (len + 1 == cap) {
            cap *= 2;
            argv = realloc(argv, cap * sizeof(char *));
        }

        // 将参数填入数组
        argv[len] = arg;
        len++;
    }
    argv[len] = NULL;
    *argc = len;

    return argv;
}

char *getfilepath(char *filename) {
    char *filepath;
    size_t cap = 4096, len;
    filepath = calloc(cap, sizeof(char));

    for (size_t i = 0; paths[i]; i++) {
        len = strlen(paths[i]) + strlen(filename) + 2;
        if (len > cap) {
            cap = cap * 2 > len ? cap * 2 : len;
            filepath = realloc(filepath, cap * sizeof(char));
        }

        sprintf(filepath, "%s/%s", paths[i], filename);

        if (access(filepath, X_OK) == 0) {
            return filepath;
        }
    }

    free(filepath);
    return NULL;
};

void setpaths(char *pathv[]) {
    size_t len = 0;
    for (len = 0; pathv[len]; len++) {
    }

    for (size_t i = 0; paths[i]; i++) {
        free(paths[i]);
    }

    paths = realloc(paths, (len + 1) * sizeof(char *));
    paths[len] = NULL;
    for (size_t i = 0; i < len; i++) {
        paths[i] = strdup(pathv[i]);
    }
}

void redirect(char *file, int *saved_stdout, int *saved_stderr) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
    *saved_stdout = dup(STDOUT_FILENO);
    *saved_stderr = dup(STDERR_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
}

void restore(int saved_stdout, int saved_stderr) {
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);
}
