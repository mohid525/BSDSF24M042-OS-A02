# Operating Systems Assignment Report

## Feature 1: Project Setup and Initial Build

### Summary

The project repository was created as a public GitHub repository with the
required naming convention. The starter C source file was placed in the
`src/` directory.

The project structure includes:

```text
src/
bin/
obj/
man/
Makefile
README.md
REPORT.md
```

The program was compiled successfully using `make`, and the executable was
created in the `bin/` directory.

The initial project was committed and pushed to the `main` branch.

---

## Feature 2: Long Listing Format

### Summary

Feature 2 adds the `-l` option. It displays detailed information about each
file, including permissions, link count, owner, group, file size, modification
time, and filename.

The implementation uses:

- `lstat()`
- `getpwuid()`
- `getgrgid()`
- `strftime()`
- Permission-bit macros

### Question 1: What is the difference between `stat()` and `lstat()`?

Both `stat()` and `lstat()` retrieve file metadata and store it in a
`struct stat`.

The important difference is that `stat()` follows symbolic links and returns
metadata about the link target. In contrast, `lstat()` returns metadata about
the symbolic link itself.

In the context of `ls`, `lstat()` is more appropriate when the program needs
to identify symbolic links and display them correctly. It prevents the
program from treating a symbolic link as if it were the target file.

### Question 2: How are file types and permissions extracted from `st_mode`?

The `st_mode` field contains both the file type and permission bits. Bitwise
operators and predefined macros can be used to examine these bits.

To check whether a file is a directory:

```c
if ((file_stat.st_mode & S_IFMT) == S_IFDIR)
    printf("Directory");
```

`S_IFMT` is a mask that extracts the file-type bits, while `S_IFDIR`
represents a directory.

Permission bits can be checked using macros such as:

```c
if (file_stat.st_mode & S_IRUSR)
    printf("The owner has read permission");
```

Examples of permission macros include:

- `S_IRUSR`: owner read permission
- `S_IWUSR`: owner write permission
- `S_IXUSR`: owner execute permission
- `S_IRGRP`: group read permission
- `S_IWGRP`: group write permission
- `S_IXGRP`: group execute permission
- `S_IROTH`: other-user read permission
- `S_IWOTH`: other-user write permission
- `S_IXOTH`: other-user execute permission

The bitwise AND operator `&` checks whether a particular bit is set. A
nonzero result means that the corresponding permission or file-type flag is
present.

---

## Feature 3: Column Display

### Summary

Feature 3 changes the default output from one filename per line to a
multiple-column display.

The program reads all visible directory entries into a dynamically allocated
array. It records the longest filename, obtains the terminal width using
`ioctl()`, calculates the number of columns and rows, and prints the entries
in down-then-across order.

The program uses an 80-column fallback if the terminal size cannot be
detected.

### Question 1: What is the general logic for down-then-across printing?

The program first reads all filenames into an array. It then determines the
longest filename and obtains the terminal width.

Using the filename width and terminal width, it calculates the number of
columns:

```text
columns = terminal_width / column_width
```

It then calculates the number of rows:

```text
rows = (number_of_files + columns - 1) / columns
```

To print down each column and then across, the array index is calculated as:

```c
index = row + column * rows;
```

A simple loop is insufficient because it would print filenames sequentially.
It would not know how to arrange them into rows and columns or how to align
the columns based on the terminal width.

### Question 2: What is the purpose of `ioctl()`?

The `ioctl()` system call is used with the `TIOCGWINSZ` request to obtain the
current terminal dimensions.

The program uses the terminal width to calculate how many filename columns can
fit on the screen.

If the program only used a fixed fallback such as 80 columns, it would not
adapt to different terminal sizes. On a wide terminal, it might display fewer
columns than possible. On a narrow terminal, filenames could extend beyond
the screen or the output could become poorly aligned.

---

## Feature 4: Horizontal Column Display

### Summary

Feature 4 adds the `-x` option for horizontal, row-major output.

The program supports three display modes:

```c
#define MODE_DEFAULT 0
#define MODE_LONG 1
#define MODE_HORIZONTAL 2
```

The modes are:

- Default: down-then-across column display
- `-l`: long listing format
- `-x`: horizontal, across-then-wrapping display

The `-l` and `-x` options are mutually exclusive.

### Question 1: Which printing method is more complex?

The down-then-across method is more complex because it requires more
pre-calculation.

It must know:

- The total number of filenames
- The longest filename
- The terminal width
- The number of columns
- The number of rows

It also requires an index calculation such as:

```c
index = row + column * rows;
```

The horizontal method is simpler. It processes filenames from left to right
and keeps track of the current horizontal position. When the next filename
would exceed the terminal width, it starts a new line.

### Question 2: How are the different display modes managed?

The program stores the selected display mode in an integer variable.

The `getopt()` function checks for `-l` and `-x`. Depending on the option, the
program sets the mode to `MODE_LONG` or `MODE_HORIZONTAL`. If no option is
provided, the mode remains `MODE_DEFAULT`.

After reading and sorting the filenames, the program selects the appropriate
function:

```c
if (display_mode == MODE_LONG)
    print_long_format(...);
else if (display_mode == MODE_HORIZONTAL)
    print_horizontal(...);
else
    print_columns(...);
```

---

## Feature 5: Alphabetical Sorting

### Summary

Feature 5 adds alphabetical sorting using the standard C library function
`qsort()`.

All filenames are first stored in a dynamically allocated array. After the
directory has been completely read, the array is sorted using a comparison
function based on `strcmp()`.

The sorted filenames are then passed to the selected display function.

Sorting applies to:

- Default column output
- Horizontal `-x` output
- Long `-l` output

### Question 1: Why must all entries be stored before sorting?

A directory stream provides entries one at a time. However, `qsort()` sorts
an existing collection of elements. Therefore, the program must first copy
all filenames into a dynamically allocated array.

The disadvantage is increased memory usage. A directory containing millions of
files could require a large amount of memory to store all filenames and
pointers.

Despite this limitation, storing all entries is necessary to sort the
complete directory listing before displaying it.

### Question 2: What is the purpose and signature of the comparison function?

The comparison function tells `qsort()` how two filenames should be ordered.

Its signature is:

```c
int compare_names(const void *first, const void *second);
```

The function converts the generic pointers into string pointers and compares
the strings using `strcmp()`:

```c
int compare_names(const void *first, const void *second)
{
    const char *name_first = *(const char * const *)first;
    const char *name_second = *(const char * const *)second;

    return strcmp(name_first, name_second);
}
```

The parameters are `const void *` because `qsort()` is a generic function that
can sort arrays of different data types. The `const` qualifier prevents the
comparison function from modifying the array elements.

---

## Feature 6: Colorized Output

### Summary

Feature 6 adds colorized output based on file type and permissions.

The program uses `lstat()` to inspect each file before printing its name.

The color rules are:

- Directories: blue
- Executable files: green
- Archive files: red
- Symbolic links: cyan
- Special files: reverse video
- Normal files: default terminal color

ANSI reset codes are printed after colored filenames so that later output is
not affected.

### Question 1: How do ANSI escape codes produce color?

ANSI escape codes are special character sequences understood by standard Linux
terminals. They begin with the escape character and control text formatting.

For example, green text can be printed using:

```c
printf("\033[32m%s\033[0m", filename);
```

The sequence `\033[32m` changes the text color to green. The sequence
`\033[0m` resets the terminal formatting to its default state.

The program stores these sequences in macros such as:

```c
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"
```

### Question 2: Which permission bits identify executable files?

The `st_mode` field contains execute permission bits for the owner, group,
and other users.

The relevant macros are:

- `S_IXUSR`: owner execute permission
- `S_IXGRP`: group execute permission
- `S_IXOTH`: other-user execute permission

The program checks them using:

```c
if (file_stat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
{
    /* The file is executable */
}
```

If any of these bits is set, the file is treated as executable and printed in
green.

---

## Feature 7: Recursive Listing

### Summary

Feature 7 adds the `-R` option for recursive directory listing.

When `-R` is used, the program:

1. Prints the current directory header.
2. Reads and sorts the directory entries.
3. Displays the entries using the selected display mode.
4. Constructs a full path for each entry.
5. Uses `lstat()` to retrieve metadata.
6. Checks whether the entry is a directory.
7. Recursively calls `do_ls()` for each subdirectory.

The recursive option works with other display modes:

```text
-R
-lR
-xR
```

Hidden files remain excluded because entries beginning with `.` are skipped.

### Question 1: What is a base case?

A base case is the condition that stops a recursive function from calling
itself again. Every recursive function must eventually reach a base case;
otherwise, it will continue calling itself until the program runs out of
stack memory.

In recursive `ls`, recursion stops when the current directory has been
processed and there are no more subdirectories to visit.

Recursion also stops when:

- The path cannot be opened
- The entry is not a directory
- The directory contains no subdirectories

Symbolic links should not be followed blindly because they can create cycles.
The program uses `lstat()` to identify symbolic links and does not recurse
through them.

### Question 2: Why is constructing the full path necessary?

Before making a recursive call, the program must construct the complete path
of the subdirectory:

```text
parent_dir/subdir
```

For example:

```c
do_ls("parent_dir/subdir", display_mode, recursive);
```

The full path tells the operating system exactly which directory to open.

If the program called:

```c
do_ls("subdir", display_mode, recursive);
```

the operating system would search for `subdir` relative to the program's
current working directory instead of relative to `parent_dir`.

This could cause the program to:

- Report that the directory does not exist
- Open the wrong directory
- Repeatedly visit an unintended path
- Fail to process nested directories correctly

Constructing the full path preserves the correct location as recursion moves
deeper into the directory tree:

```text
parent_dir/subdir/nested_dir
```
