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
```
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
```
# Feature 5 Report: Alphabetical Sorting

## Implementation Summary

Feature 5 adds alphabetical sorting using the standard C library function
`qsort()`.

All visible directory entries are first stored in a dynamically allocated
array. After reading is complete, the array is sorted before any display
function is called. The sorted array is then passed to the default column,
horizontal column, or long-listing display function.

The comparison function receives two `const void *` arguments, converts them
to string pointers, and compares the filenames using `strcmp()`.

Sorting is applied to all display modes:

- Default down-then-across output
- Horizontal `-x` output
- Long listing `-l` output

## Report Question 1

All directory entries must be stored before sorting because `qsort()` operates
on an existing collection of elements. A directory stream provides entries
one at a time, so the program must first copy the filenames into memory.

The disadvantage is that memory usage grows with the number of directory
entries. A directory containing millions of files could require a large
amount of memory. However, storing all entries is necessary for sorting the
complete directory listing.

## Report Question 2

The comparison function tells `qsort()` how to order the filenames. It receives
pointers to two array elements, converts them to string pointers, and calls
`strcmp()` to compare the names alphabetically.

The arguments are declared as `const void *` because `qsort()` is a generic
function that can sort arrays containing different data types. The `const`
keyword prevents the comparison function from modifying the array elements.

```
# Feature 6 Report: Colorized Output

## ANSI Escape Codes

ANSI escape codes are special character sequences beginning with the escape
character. They instruct the terminal to change text formatting or color.

For example, green text can be printed using:

```c
printf("\033[32m%s\033[0m", filename);

## Recursive Listing Questions

### What is a base case?

A base case is the condition that stops a recursive function from calling
itself again. Every recursive function must eventually reach its base case;
otherwise, it will continue calling itself until the program runs out of
stack memory.

In a recursive `ls` implementation, recursion stops when the program has
finished reading the current directory and there are no more subdirectories
that need to be visited. The function also stops recursing when it encounters
a path that cannot be opened or when the entry is not a directory.

Symbolic links should not be followed blindly because they can create cycles
that may cause infinite recursion.

### Why is constructing the full path necessary?

Before making a recursive call, the program must construct the complete path
of the subdirectory:

```text
parent_dir/subdir
