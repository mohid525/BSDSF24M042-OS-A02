#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>
#include <sys/ioctl.h>
void do_ls(const char *dir, int long_format);
void print_long_format(const char *dir, const char *name);
void permissions(mode_t mode, char *result);
void print_columns(char **names, size_t count, size_t max_length);
int main(int argc, char *argv[])
{
    int option;
    int long_format = 0;

    while ((option = getopt(argc, argv, "l")) != -1)
    {
        if (option == 'l')
            long_format = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [directory ...]\n", argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_format);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], long_format);

            if (i < argc - 1)
                putchar('\n');
        }
    }

    return 0;
}

void do_ls(const char *dir, int long_format)
{
    struct stat dir_stat;

    if (lstat(dir, &dir_stat) == -1)
    {
        perror(dir);
        return;
    }

    if (!S_ISDIR(dir_stat.st_mode))
    {
        if (long_format)
            print_long_format(".", dir);
        else
            printf("%s\n", dir);

        return;
    }

    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t max_length = 0;

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
        {
            print_long_format(dir, entry->d_name);
            continue;
        }

        if (count == capacity)
        {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **temporary = realloc(names,
                                       new_capacity * sizeof(char *));

            if (temporary == NULL)
            {
                perror("realloc");

                for (size_t i = 0; i < count; i++)
                    free(names[i]);

                free(names);
                closedir(dp);
                return;
            }

            names = temporary;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL)
        {
            perror("strdup");

            for (size_t i = 0; i < count; i++)
                free(names[i]);

            free(names);
            closedir(dp);
            return;
        }

        size_t length = strlen(names[count]);

        if (length > max_length)
            max_length = length;

        count++;
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);

    if (!long_format)
        print_columns(names, count, max_length);

    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
} 
void print_columns(char **names, size_t count, size_t max_length)
{
    if (count == 0)
        return;

    struct winsize terminal_size;
    size_t terminal_width = 80;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal_size) == 0 &&
        terminal_size.ws_col > 0)
    {
        terminal_width = terminal_size.ws_col;
    }

    size_t spacing = 2;
    size_t column_width = max_length + spacing;
    size_t columns = terminal_width / column_width;

    if (columns == 0)
        columns = 1;

    if (columns > count)
        columns = count;

    size_t rows = (count + columns - 1) / columns;

    for (size_t row = 0; row < rows; row++)
    {
        for (size_t column = 0; column < columns; column++)
        {
            size_t index = row + column * rows;

            if (index >= count)
                continue;

            if (column == columns - 1 || index + rows >= count)
                printf("%s", names[index]);
            else
                printf("%-*s", (int)column_width, names[index]);
        }

        putchar('\n');
    }
}
void print_long_format(const char *dir, const char *name)
{
    char path[PATH_MAX];

    if (strcmp(dir, ".") == 0)
        snprintf(path, sizeof(path), "./%s", name);
    else
        snprintf(path, sizeof(path), "%s/%s", dir, name);

    struct stat file_stat;

    if (lstat(path, &file_stat) == -1)
    {
        perror(path);
        return;
    }

    char mode[11];
    permissions(file_stat.st_mode, mode);

    struct passwd *owner = getpwuid(file_stat.st_uid);
    struct group *group = getgrgid(file_stat.st_gid);

    char date[32];
    struct tm *time_info = localtime(&file_stat.st_mtime);
    strftime(date, sizeof(date), "%b %e %H:%M", time_info);

    printf("%s %2lu %-8s %-8s %8lld %s %s",
           mode,
           (unsigned long)file_stat.st_nlink,
           owner ? owner->pw_name : "unknown",
           group ? group->gr_name : "unknown",
           (long long)file_stat.st_size,
           date,
           name);

    if (S_ISLNK(file_stat.st_mode))
    {
        char target[PATH_MAX];
        ssize_t length = readlink(path, target, sizeof(target) - 1);

        if (length != -1)
        {
            target[length] = '\0';
            printf(" -> %s", target);
        }
    }

    putchar('\n');
}

void permissions(mode_t mode, char *result)
{
    result[0] = S_ISDIR(mode) ? 'd' :
                S_ISLNK(mode) ? 'l' :
                S_ISCHR(mode) ? 'c' :
                S_ISBLK(mode) ? 'b' :
                S_ISFIFO(mode) ? 'p' :
                S_ISSOCK(mode) ? 's' : '-';

    result[1] = mode & S_IRUSR ? 'r' : '-';
    result[2] = mode & S_IWUSR ? 'w' : '-';
    result[3] = mode & S_IXUSR ? 'x' : '-';

    result[4] = mode & S_IRGRP ? 'r' : '-';
    result[5] = mode & S_IWGRP ? 'w' : '-';
    result[6] = mode & S_IXGRP ? 'x' : '-';

    result[7] = mode & S_IROTH ? 'r' : '-';
    result[8] = mode & S_IWOTH ? 'w' : '-';
    result[9] = mode & S_IXOTH ? 'x' : '-';

    result[10] = '\0';
}
