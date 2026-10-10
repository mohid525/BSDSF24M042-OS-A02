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
