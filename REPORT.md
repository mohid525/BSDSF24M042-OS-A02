# Feature 2 Report: Long Listing Format

## stat() versus lstat()

Both `stat()` and `lstat()` retrieve file metadata. The difference is that
`stat()` follows symbolic links and returns metadata for the link target,
while `lstat()` returns metadata about the symbolic link itself.

`lstat()` is more appropriate for an `ls` implementation because it allows
the program to identify symbolic links and display them correctly.

## st_mode and Bitwise Operators

The `st_mode` field contains both the file type and permission bits.

The bitwise AND operator `&` checks whether a specific permission bit is set.
For example:

```c
if (mode & S_IRUSR)
    printf("The owner has read permission");
```
# Feature 3 Report: Column Display

## Implementation Summary

Feature 3 changes the default output of the `ls` program from one filename
per line to a multiple-column display.

The program reads all visible directory entries into a dynamically allocated
array of strings. While reading the entries, it records the length of the
longest filename. This information is required to calculate the column
width.

The terminal width is obtained using the `ioctl()` system call with the
`TIOCGWINSZ` request. If the terminal width cannot be detected, the program
uses a fallback width of 80 columns.

The number of columns is calculated using the terminal width and the maximum
filename length. The program then calculates the number of rows and prints
the filenames down each column and then across.

The index used for down-then-across printing is:

```c
index = row + column * rows;
# Feature 4 Report: Horizontal Column Display

## Implementation Summary

Feature 4 adds the `-x` command-line option for horizontal column display.

The program now supports three display modes:

- Default mode: down-then-across columns
- `-l`: long listing format
- `-x`: horizontal, row-major column display

The display mode is stored in an integer constant:

```c
#define MODE_DEFAULT 0
#define MODE_LONG 1
#define MODE_HORIZONTAL 2
