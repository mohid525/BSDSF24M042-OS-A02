#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

#define MODE_DEFAULT 0
#define MODE_LONG 1
#define MODE_HORIZONTAL 2

#define COLOR_BLUE "\033[34m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_CYAN "\033[36m"
#define COLOR_REVERSE "\033[7m"
#define COLOR_RESET "\033[0m"

void do_ls(const char *dir, int display_mode, int recursive);
void print_long_format(const char *dir, const char *name);
void print_long_path(const char *path, const char *display_name);
void print_columns(const char *dir, char **names, size_t count,
                   size_t max_length);
void print_horizontal(const char *dir, char **names, size_t count,
                      size_t max_length);
void print_colored_name(const char *dir, const char *name);
void print_colored_path(const char *path, const char *display_name);
void permissions(mode_t mode, char *result);
void free_names(char **names, size_t count);
int compare_names(const void *first, const void *second);
int ends_with(const char *text, const char *suffix);
void build_path(const char *dir, const char *name,
                char *path, size_t path_size);

int main(int argc, char *argv[])
{
    int option;
    int display_mode = MODE_DEFAULT;
    int recursive = 0;

    while ((option = getopt(argc, argv, "lxR")) != -1)
    {
        if (option == 'l')
        {
            if (display_mode != MODE_DEFAULT)
            {
                fprintf(stderr, "Options -l and -x cannot be used together\n");
                return 1;
            }

            display_mode = MODE_LONG;
        }
        else if (option == 'x')
        {
            if (display_mode != MODE_DEFAULT)
            {
                fprintf(stderr, "Options -l and -x cannot be used together\n");
                return 1;
            }

            display_mode = MODE_HORIZONTAL;
        }
        else if (option == 'R')
        {
            recursive = 1;
        }
        else
        {
            fprintf(stderr, "Usage: %s [-l | -x] [-R] [directory ...]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", display_mode, recursive);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (recursive)
            {
                do_ls(argv[i], display_mode, recursive);
            }
            else
            {
                printf("Directory listing of %s:\n", argv[i]);
                do_ls(argv[i], display_mode, recursive);

                if (i < argc - 1)
                    putchar('\n');
            }
        }
    }

    return 0;
}

void do_ls(const char *dir, int display_mode, int recursive)
{
    struct stat dir_stat;

    if (lstat(dir, &dir_stat) == -1)
    {
        perror(dir);
        return;
    }

    if (!S_ISDIR(dir_stat.st_mode))
    {
        if (display_mode == MODE_LONG)
            print_long_path(dir, dir);
        else
        {
            print_colored_path(dir, dir);
            putchar('\n');
        }

        return;
    }

    if (recursive)
        printf("%s:\n", dir);

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

        if (count == capacity)
        {
            size_t new_capacity = capacity == 0 ? 16 : capacity * 2;

            char **temporary = realloc(
                names,
                new_capacity * sizeof(char *)
            );

            if (temporary == NULL)
            {
                perror("realloc");
                free_names(names, count);
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
            free_names(names, count);
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

    qsort(names, count, sizeof(char *), compare_names);

    if (display_mode == MODE_LONG)
    {
        for (size_t i = 0; i < count; i++)
            print_long_format(dir, names[i]);
    }
    else if (display_mode == MODE_HORIZONTAL)
    {
        print_horizontal(dir, names, count, max_length);
    }
    else
    {
        print_columns(dir, names, count, max_length);
    }

    if (recursive)
    {
        for (size_t i = 0; i < count; i++)
        {
            char full_path[PATH_MAX];
            struct stat child_stat;

            build_path(dir, names[i], full_path, sizeof(full_path));

            if (lstat(full_path, &child_stat) == -1)
            {
                perror(full_path);
                continue;
            }

            if (S_ISDIR(child_stat.st_mode))
            {
                putchar('\n');
                do_ls(full_path, display_mode, recursive);
            }
        }
    }

    free_names(names, count);
}

void build_path(const char *dir, const char *name,
                char *path, size_t path_size)
{
    if (strcmp(dir, ".") == 0)
        snprintf(path, path_size, "./%s", name);
    else if (dir[strlen(dir) - 1] == '/')
        snprintf(path, path_size, "%s%s", dir, name);
    else
        snprintf(path, path_size, "%s/%s", dir, name);
}

void print_columns(const char *dir, char **names, size_t count,
                   size_t max_length)
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

            print_colored_name(dir, names[index]);

            if (column != columns - 1 && index + rows < count)
            {
                int padding = (int)(column_width - strlen(names[index]));
                printf("%*s", padding, "");
            }
        }

        putchar('\n');
    }
}

void print_horizontal(const char *dir, char **names, size_t count,
                      size_t max_length)
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
    size_t current_width = 0;

    for (size_t i = 0; i < count; i++)
    {
        size_t filename_length = strlen(names[i]);

        if (current_width > 0 &&
            current_width + column_width > terminal_width)
        {
            putchar('\n');
            current_width = 0;
        }

        if (current_width > 0)
        {
            printf("%*s", (int)spacing, "");
            current_width += spacing;
        }

        print_colored_name(dir, names[i]);
        current_width += filename_length;
    }

    putchar('\n');
}

void print_colored_name(const char *dir, const char *name)
{
    char path[PATH_MAX];

    build_path(dir, name, path, sizeof(path));
    print_colored_path(path, name);
}

void print_colored_path(const char *path, const char *display_name)
{
    struct stat file_stat;

    if (lstat(path, &file_stat) == -1)
    {
        printf("%s", display_name);
        return;
    }

    const char *color = NULL;

    if (S_ISDIR(file_stat.st_mode))
        color = COLOR_BLUE;
    else if (S_ISLNK(file_stat.st_mode))
        color = COLOR_CYAN;
    else if (S_ISCHR(file_stat.st_mode) ||
             S_ISBLK(file_stat.st_mode) ||
             S_ISFIFO(file_stat.st_mode) ||
             S_ISSOCK(file_stat.st_mode))
        color = COLOR_REVERSE;
    else if (ends_with(display_name, ".tar") ||
             ends_with(display_name, ".gz") ||
             ends_with(display_name, ".zip") ||
             ends_with(display_name, ".bz2") ||
             ends_with(display_name, ".xz"))
        color = COLOR_RED;
    else if (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        color = COLOR_GREEN;

    if (color != NULL)
        printf("%s%s%s", color, display_name, COLOR_RESET);
    else
        printf("%s", display_name);
}

void print_long_format(const char *dir, const char *name)
{
    char path[PATH_MAX];

    build_path(dir, name, path, sizeof(path));
    print_long_path(path, name);
}

void print_long_path(const char *path, const char *display_name)
{
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

    if (time_info != NULL)
        strftime(date, sizeof(date), "%b %e %H:%M", time_info);
    else
        strcpy(date, "unknown");

    printf("%s %2lu %-8s %-8s %8lld %s ",
           mode,
           (unsigned long)file_stat.st_nlink,
           owner ? owner->pw_name : "unknown",
           group ? group->gr_name : "unknown",
           (long long)file_stat.st_size,
           date);

    print_colored_path(path, display_name);

    if (S_ISLNK(file_stat.st_mode))
    {
        char target[PATH_MAX];

        ssize_t length = readlink(
            path,
            target,
            sizeof(target) - 1
        );

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

int compare_names(const void *first, const void *second)
{
    const char *name_first = *(const char * const *)first;
    const char *name_second = *(const char * const *)second;

    return strcmp(name_first, name_second);
}

int ends_with(const char *text, const char *suffix)
{
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);

    if (suffix_length > text_length)
        return 0;

    return strcmp(
        text + text_length - suffix_length,
        suffix
    ) == 0;
}

void free_names(char **names, size_t count)
{
    if (names == NULL)
        return;

    for (size_t i = 0; i < count; i++)
        free(names[i]);

    free(names);
}
