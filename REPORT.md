# Operating Systems - Programming Assignment 02 Report

**Student Roll No:** BSDSF24A008  
**Course:** Operating Systems  
**Instructor:** Dr. Muhammad Arif Butt  

---

## Feature-2: ls-v1.1.0 – Complete Long Listing Format

### Question 1: What is the crucial difference between the stat() and lstat() system calls? In the context of the ls command, when is it more appropriate to use lstat()?

#### Answer:
The main difference between stat() and lstat() is how they handle symbolic links:

- stat() follows symbolic links. It gives the information about the file the link points to. If the target file does not exist, stat() fails.
- lstat() does not follow symbolic links. It gives information about the symbolic link itself.

In the context of ls, it is more appropriate to use lstat() because ls needs to list the actual files and links present in the directory. If an entry is a symbolic link, ls must show it as a link (with 'l' as the file type). If we used stat() on a broken link, stat() would return an error even though the link exists in the directory.

---

### Question 2: The st_mode field in struct stat is an integer that contains both the file type (e.g., regular file, directory) and the permission bits. Explain how you can use bitwise operators (like &) and predefined macros (like S_IFDIR or S_IRUSR) to extract this information.

#### Answer:
The st_mode field is an integer where different bits represent the file type, special bits, and permissions:

1. File Type:
The file type is stored in the higher bits. We use the bitwise AND operator (&) with the mask S_IFMT to filter out the file type bits, and then compare the result with macros like S_IFDIR (directory) or S_IFREG (regular file). Predefined macros like S_ISDIR(st_mode) do this check directly.

2. Permissions:
Each permission is represented by a single bit. We use the bitwise AND operator (&) with permission macros:
- For user: S_IRUSR (read), S_IWUSR (write), S_IXUSR (execute)
- For group: S_IRGRP (read), S_IWGRP (write), S_IXGRP (execute)
- For others: S_IROTH (read), S_IWOTH (write), S_IXOTH (execute)

If the bitwise AND operation (st_mode & S_IRUSR) returns a non-zero value, the read permission is enabled, so we display 'r'. If it returns zero, the permission is not enabled, so we display '-'.

3. Special Bits:
Special bits like SetUID (S_ISUID), SetGID (S_ISGID), and Sticky bit (S_ISVTX) are also checked using the bitwise AND operator (&) to determine if 's', 'S', 't', or 'T' should be printed instead of 'x'.
