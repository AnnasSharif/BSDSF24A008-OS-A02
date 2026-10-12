#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
struct passwd { char *pw_name; };
struct group { char *gr_name; };
static inline struct passwd *getpwuid(int u) { (void)u; static struct passwd p = {"user"}; return &p; }
static inline struct group *getgrgid(int g) { (void)g; static struct group gr = {"group"}; return &gr; }
#define lstat stat
#ifndef S_ISLNK
#define S_ISLNK(m) 0
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) 0
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) 0
#endif
#ifndef S_ISUID
#define S_ISUID 04000
#endif
#ifndef S_ISGID
#define S_ISGID 02000
#endif
#ifndef S_ISVTX
#define S_ISVTX 01000
#endif
#ifndef S_IRGRP
#define S_IRGRP 00040
#endif
#ifndef S_IWGRP
#define S_IWGRP 00020
#endif
#ifndef S_IXGRP
#define S_IXGRP 00010
#endif
#ifndef S_IROTH
#define S_IROTH 00004
#endif
#ifndef S_IWOTH
#define S_IWOTH 00002
#endif
#ifndef S_IXOTH
#define S_IXOTH 00001
#endif
static inline int get_terminal_width(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
    {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
}
#else
#include <pwd.h>
#include <grp.h>
#include <sys/ioctl.h>
static inline int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
    {
        return w.ws_col;
    }
    return 80;
}
#endif

extern int errno;

#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_PINK "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

void print_colored_name(const char *name, mode_t mode)
{
    const char *color = NULL;

    if (S_ISLNK(mode))
    {
        color = COLOR_PINK;
    }
    else if (S_ISDIR(mode))
    {
        color = COLOR_BLUE;
    }
    else if (S_ISCHR(mode) || S_ISBLK(mode) || S_ISSOCK(mode) || S_ISFIFO(mode))
    {
        color = COLOR_REVERSE;
    }
    else if (strstr(name, ".tar") != NULL || strstr(name, ".gz") != NULL || strstr(name, ".zip") != NULL)
    {
        color = COLOR_RED;
    }
    else if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
    {
        color = COLOR_GREEN;
    }

    if (color != NULL)
    {
        printf("%s%s%s", color, name, COLOR_RESET);
    }
    else
    {
        printf("%s", name);
    }
}

int compare_names(const void *a, const void *b)
{
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;
    return strcmp(name1, name2);
}

void get_permissions(mode_t mode, char *str)
{
    strcpy(str, "----------");

    if (S_ISDIR(mode)) str[0] = 'd';
    else if (S_ISCHR(mode)) str[0] = 'c';
    else if (S_ISBLK(mode)) str[0] = 'b';
    else if (S_ISLNK(mode)) str[0] = 'l';
    else if (S_ISFIFO(mode)) str[0] = 'p';
    else if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = (mode & S_ISUID) ? 's' : 'x';
    else if (mode & S_ISUID) str[3] = 'S';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = (mode & S_ISGID) ? 's' : 'x';
    else if (mode & S_ISGID) str[6] = 'S';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = (mode & S_ISVTX) ? 't' : 'x';
    else if (mode & S_ISVTX) str[9] = 'T';
}

void show_file_info(char *path, char *filename)
{
    struct stat info;
    if (lstat(path, &info) == -1)
    {
        perror(path);
        return;
    }

    char modestr[11];
    get_permissions(info.st_mode, modestr);

    struct passwd *p = getpwuid(info.st_uid);
    char *user = p ? p->pw_name : "unknown";

    struct group *g = getgrgid(info.st_gid);
    char *group = g ? g->gr_name : "unknown";

    char *time_str = ctime(&info.st_mtime);
    char formatted_time[20];
    if (time_str)
    {
        strncpy(formatted_time, time_str + 4, 12);
        formatted_time[12] = '\0';
    }
    else
    {
        strcpy(formatted_time, "??? ?? ??:??");
    }

    printf("%s %2ld %-8s %-8s %8lld %s ",
           modestr,
           (long)info.st_nlink,
           user,
           group,
           (long long)info.st_size,
           formatted_time);
    print_colored_name(filename, info.st_mode);
    printf("\n");
}

void print_columns(const char *dirname, char **files, int count, int max_len)
{
    if (count == 0)
    {
        return;
    }

    int term_width = get_terminal_width();
    int col_width = max_len + 2;
    int num_cols = term_width / col_width;

    if (num_cols < 1)
    {
        num_cols = 1;
    }

    int num_rows = (count + num_cols - 1) / num_cols;

    for (int r = 0; r < num_rows; r++)
    {
        for (int c = 0; c < num_cols; c++)
        {
            int index = r + c * num_rows;
            if (index < count)
            {
                char path[1024];
                if (strcmp(dirname, ".") == 0)
                    snprintf(path, sizeof(path), "%s", files[index]);
                else
                    snprintf(path, sizeof(path), "%s/%s", dirname, files[index]);

                struct stat st;
                mode_t mode = 0;
                if (lstat(path, &st) == 0)
                {
                    mode = st.st_mode;
                }

                print_colored_name(files[index], mode);

                if (c < num_cols - 1 && index + num_rows < count)
                {
                    int padding = col_width - (int)strlen(files[index]);
                    for (int p = 0; p < padding; p++)
                    {
                        putchar(' ');
                    }
                }
            }
        }
        printf("\n");
    }
}

void print_horizontal(const char *dirname, char **files, int count, int max_len)
{
    if (count == 0)
    {
        return;
    }

    int term_width = get_terminal_width();
    int col_width = max_len + 2;
    int current_pos = 0;

    for (int i = 0; i < count; i++)
    {
        if (current_pos + col_width > term_width && current_pos > 0)
        {
            printf("\n");
            current_pos = 0;
        }

        char path[1024];
        if (strcmp(dirname, ".") == 0)
            snprintf(path, sizeof(path), "%s", files[i]);
        else
            snprintf(path, sizeof(path), "%s/%s", dirname, files[i]);

        struct stat st;
        mode_t mode = 0;
        if (lstat(path, &st) == 0)
        {
            mode = st.st_mode;
        }

        print_colored_name(files[i], mode);
        int padding = col_width - (int)strlen(files[i]);
        for (int p = 0; p < padding; p++)
        {
            putchar(' ');
        }
        current_pos += col_width;
    }
    printf("\n");
}

void do_ls(char *dirname, int mode)
{
    DIR *dp = opendir(dirname);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dirname);
        return;
    }

    struct dirent *entry;
    char **files = NULL;
    int count = 0;
    int capacity = 0;
    int max_len = 0;

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count >= capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            files = realloc(files, capacity * sizeof(char *));
        }

        files[count] = malloc(strlen(entry->d_name) + 1);
        strcpy(files[count], entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len)
        {
            max_len = len;
        }
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    if (count > 0)
    {
        qsort(files, count, sizeof(char *), compare_names);
    }

    if (mode == 1)
    {
        long long total_blocks = 0;
        char path[1024];

        for (int i = 0; i < count; i++)
        {
            if (strcmp(dirname, ".") == 0)
                snprintf(path, sizeof(path), "%s", files[i]);
            else
                snprintf(path, sizeof(path), "%s/%s", dirname, files[i]);

            struct stat st;
            if (lstat(path, &st) == 0)
            {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
                total_blocks += st.st_blocks / 2;
#else
                total_blocks += (st.st_size + 1023) / 1024;
#endif
            }
        }
        printf("total %lld\n", total_blocks);

        for (int i = 0; i < count; i++)
        {
            if (strcmp(dirname, ".") == 0)
                snprintf(path, sizeof(path), "%s", files[i]);
            else
                snprintf(path, sizeof(path), "%s/%s", dirname, files[i]);

            show_file_info(path, files[i]);
        }
    }
    else if (mode == 2)
    {
        print_horizontal(dirname, files, count, max_len);
    }
    else
    {
        print_columns(dirname, files, count, max_len);
    }

    for (int i = 0; i < count; i++)
    {
        free(files[i]);
    }
    free(files);
}

int main(int argc, char *argv[])
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode))
    {
        SetConsoleMode(hOut, dwMode | 0x0004);
    }
#endif

    int mode = 0;
    int opt;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l')
        {
            mode = 1;
        }
        else if (opt == 'x')
        {
            mode = 2;
        }
        else
        {
            fprintf(stderr, "Usage: %s [-l] [-x] [directory...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        int multiple = (argc - optind > 1);
        for (int i = optind; i < argc; i++)
        {
            if (multiple)
            {
                printf("%s:\n", argv[i]);
            }
            do_ls(argv[i], mode);
            if (multiple && i < argc - 1)
            {
                puts("");
            }
        }
    }

    return 0;
}
