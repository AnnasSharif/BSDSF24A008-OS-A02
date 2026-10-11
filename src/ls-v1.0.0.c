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
#else
#include <pwd.h>
#include <grp.h>
#endif

extern int errno;

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

    printf("%s %2ld %-8s %-8s %8lld %s %s\n",
           modestr,
           (long)info.st_nlink,
           user,
           group,
           (long long)info.st_size,
           formatted_time,
           filename);
}

void do_ls(char *dirname, int l_flag)
{
    DIR *dp = opendir(dirname);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dirname);
        return;
    }

    struct dirent *entry;
    char path[1024];

    if (l_flag)
    {
        long long total_blocks = 0;
        while ((entry = readdir(dp)) != NULL)
        {
            if (entry->d_name[0] == '.')
                continue;

            if (strcmp(dirname, ".") == 0)
                snprintf(path, sizeof(path), "%s", entry->d_name);
            else
                snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

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
        rewinddir(dp);
    }

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (l_flag)
        {
            if (strcmp(dirname, ".") == 0)
                snprintf(path, sizeof(path), "%s", entry->d_name);
            else
                snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

            show_file_info(path, entry->d_name);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
}

int main(int argc, char *argv[])
{
    int l_flag = 0;
    int opt;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        if (opt == 'l')
        {
            l_flag = 1;
        }
        else
        {
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", l_flag);
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
            do_ls(argv[i], l_flag);
            if (multiple && i < argc - 1)
            {
                puts("");
            }
        }
    }

    return 0;
}


