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
void evalsub(char *cmdline);
void redirect(char *file, int *saved_stdout, int *saved_stderr);
void restore(int saved_stdout, int saved_stderr);
void run(int argc, char *argv[]);
char **getargv(char *cmdline, int *argc);
char *getfilepath(char *filename);
void setpaths(char *pathv[]);

inline static void prt_err() {
    write(STDERR_FILENO, "An error has occurred\n", 22);
}

int main(int argc, char const *argv[]) {
    char *cmdline = NULL;
    size_t n = 0;

    // 获得输入文件
    FILE *cmdfile = getcmdfile(argc, argv);

    // 初始化 paths
    paths = calloc(2, sizeof(char *));
    paths[0] = strdup("/bin");

    // read-eval-print-loop
    while (true) {
        if (cmdfile == stdin) {
            printf("wish> ");
        }

        getline(&cmdline, &n, cmdfile);

        if (feof(cmdfile)) {
            exit(EXIT_SUCCESS);
        }

        eval(cmdline);
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
        prt_err();
        exit(EXIT_FAILURE);
    }
}

void eval(char *cmdline) {
    char *save_ptr;

    // 将并发命令拆分为子命令并求值
    for (cmdline = strtok_r(cmdline, "&", &save_ptr); cmdline;
         cmdline = strtok_r(NULL, "&", &save_ptr)) {
        evalsub(cmdline);
    }

    // 等待所有子进程
    while (wait(NULL) != -1) {
    }
}

void evalsub(char *cmdline) {
    char *chr, *save_ptr, *file, **argv;
    int save_stdout = -1, save_stderr = -1, argc;

    // 检测是否需要输出重定向并分割
    chr = strchr(cmdline, '>');
    if (chr) {
        *chr = '\0';
    }

    // 获取参数向量
    argv = getargv(cmdline, &argc);

    if (chr) {
        // 如果需要重定向，则获取输出文件
        file = strtok_r(chr + 1, " \t\n", &save_ptr);
        if (argc && file && !strtok_r(NULL, " \t\n", &save_ptr)) {
            // 如果有参数且输出文件有一个，则重定向并运行
            redirect(file, &save_stdout, &save_stderr);
            run(argc, argv);
        } else {
            // 否则打印错误
            prt_err();
        }
    } else if (argc) {
        // 如果不需要重定向且有参数，则直接运行
        run(argc, argv);
    }

    // 释放参数向量
    free(argv);

    if (save_stdout != -1) {
        // 如果输出已经被重定向，则恢复输出
        restore(save_stdout, save_stderr);
    }
}

void run(int argc, char *argv[]) {
    if (!strcmp(argv[0], "cd")) {
        if (argc != 2 || chdir(argv[1])) {
            prt_err();
        }
    } else if (!strcmp(argv[0], "exit")) {
        if (argc > 1) {
            prt_err();
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
            prt_err();
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
    // 获取新 paths 的长度
    for (len = 0; pathv[len]; len++) {
    }

    // 释放旧 paths 指向的串
    for (size_t i = 0; paths[i]; i++) {
        free(paths[i]);
    }

    // 修改 paths 的大小
    paths = realloc(paths, (len + 1) * sizeof(char *));

    //填充新 paths
    paths[len] = NULL;
    for (size_t i = 0; i < len; i++) {
        paths[i] = strdup(pathv[i]);
    }
}

void redirect(char *file, int *save_stdout, int *save_stderr) {
    int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC);
    *save_stdout = dup(STDOUT_FILENO);
    *save_stderr = dup(STDERR_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
}

void restore(int save_stdout, int save_stderr) {
    dup2(save_stdout, STDOUT_FILENO);
    dup2(save_stderr, STDERR_FILENO);
    close(save_stdout);
    close(save_stderr);
}
