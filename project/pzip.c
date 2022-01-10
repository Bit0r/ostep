#define _DEFAULT_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/uio.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct {
    // 输入文件名
    char **files;
    // 输出文件指针数组
    FILE *fps[];
} in_out_t;

int idx = 0;
mtx_t mtx;

int pzip(in_out_t *in_out);
void zip(int fd, FILE *fp);
void iovec_init(struct iovec iov[], int *cnt, char *c);
void mergeout(int nfiles, FILE *fps[]);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    int nprocs = get_nprocs(), nthrds = nprocs < argc - 1 ? nprocs : argc - 1;
    in_out_t *in_out = malloc(argc * sizeof(void *));
    thrd_t thrds[nthrds];

    in_out->files = argv + 1;
    mtx_init(&mtx, mtx_plain);
    for (int i = 0; i < argc - 1; i++) {
        // 创建临时文件保存压缩结果
        in_out->fps[i] = tmpfile();
    }

    for (int i = 0; i < nthrds; i++) {
        // 开 nthrds 个线程进行压缩
        thrd_create(&thrds[i], pzip, in_out);
    }

    for (int i = 0; i < nthrds; i++) {
        // 等待所有线程完成
        thrd_join(thrds[i], NULL);
    }

    mergeout(argc - 1, in_out->fps);
    return 0;
}

/**
 * 合并输出已经压缩过的临时文件
 */
void mergeout(int nfiles, FILE *fps[]) {
    char c1, c2;
    int fd_cur, fd_next, cnt1, cnt2;
    size_t size;
    struct stat st;
    struct iovec iov[2];

    for (int i = 0; i < nfiles - 1; i++) {

        // 获取当前的文件描述符，同时读取最后5个字节
        fd_cur = fileno(fps[i]);
        iovec_init(iov, &cnt1, &c1);
        fstat(fd_cur, &st);
        size = st.st_size;
        preadv(fd_cur, iov, 2, size - sizeof(int) - sizeof(char));

        // 获取下一个文件描述符，同时读取开头5个字节
        fd_next = fileno(fps[i + 1]);
        iovec_init(iov, &cnt2, &c2);
        preadv(fd_next, iov, 2, 0);

        // 设置应该写入的文件大小
        if (c1 == c2) {
            // 如果前后两个文件可以合并，就进行合并同时缩小输出大小
            cnt2 += cnt1;
            pwrite(fd_next, &cnt2, sizeof(int), 0);
            size -= sizeof(int) + sizeof(char);
        }

        if (size) {
            // 将文件发送到 stdout
            lseek(fd_cur, 0, SEEK_SET);
            sendfile(STDOUT_FILENO, fd_cur, NULL, size);
        }
    }

    // 将最后一个文件输出
    fd_cur = fileno(fps[nfiles - 1]);
    fstat(fd_cur, &st);
    if (st.st_size) {
        lseek(fd_cur, 0, SEEK_SET);
        sendfile(STDOUT_FILENO, fd_cur, NULL, st.st_size);
    }
}

void iovec_init(struct iovec iov[], int *cnt, char *c) {
    iov[0].iov_base = cnt;
    iov[0].iov_len = sizeof(int);
    iov[1].iov_base = c;
    iov[1].iov_len = sizeof(char);
}

int pzip(in_out_t *in_out) {
    char **files = in_out->files;
    int fd;
    FILE *fp, **fps = in_out->fps;

    while (1) {
        mtx_lock(&mtx);
        if (files[idx] == NULL) {
            // 如果所有文件都已经被压缩，则退出
            mtx_unlock(&mtx);
            return 0;
        }
        // 获取一个尚未被压缩的文件，进行压缩操作
        fd = open(files[idx], O_RDONLY);
        fp = fps[idx];
        idx++;
        mtx_unlock(&mtx);

        zip(fd, fp);
        close(fd);
    }
}

/**
 * 将文件描述符 fd 中的内容压缩到文件指针 fp 中
 */
void zip(int fd, FILE *fp) {
    struct stat st;
    fstat(fd, &st);
    char old, *content = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    int cnt = 0;

    old = *content;
    for (off_t i = 0; i < st.st_size; i++) {
        if (content[i] != old) {
            fwrite(&cnt, sizeof(int), 1, fp);
            putc(old, fp);
            cnt = 0;
            old = content[i];
        }
        cnt++;
    }
    fwrite(&cnt, sizeof(int), 1, fp);
    putc(content[st.st_size - 1], fp);
    munmap(content, st.st_size);
    fflush(fp);
}
