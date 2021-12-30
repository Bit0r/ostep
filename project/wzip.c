#include <stdio.h>

int main(int argc, char const *argv[]) {
    if (argc == 1) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }

    char old, chr;
    int cnt = 0, i = 1;
    FILE *file = fopen(argv[1], "r");

    // 初始化 old，使得 old 和 chr 为同一字符
    old = getc(file);
    ungetc(old, file);
    while (1) {
        // 从文件中读取一个字符
        chr = getc(file);

        if (chr == EOF) {
            if (i == argc - 1) {
                // 如果读到 EOF 且已经为最后一个文件，则直接输出并退出
                fwrite(&cnt, 4, 1, stdout);
                putchar(old);
                break;
            } else {
                // 如果还有文件，则继续读取下一个文件
                i++;
                freopen(argv[i], "r", file);
                continue;
            }
        }

        if (old != chr) {
            // 如果新读出的字符与原字符不同，则 cnt 清零并设置 old
            fwrite(&cnt, 4, 1, stdout);
            putchar(old);
            cnt = 0;
            old = chr;
        }

        // 每读出一个字符 cnt+1
        cnt++;
    }

    fclose(file);
    return 0;
}
