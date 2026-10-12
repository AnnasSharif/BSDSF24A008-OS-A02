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

---

## Feature-3: ls-v1.2.0 – Column Display (Down Then Across)

### Question 1: Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the list of filenames insufficient for this task?

#### Answer:
In a terminal, text is output line by line from left to right. Once a newline is printed, the cursor moves down and cannot move back up to fill previous lines.

In "down then across" format, filenames are listed downwards in column 1, then downwards in column 2, and so on. Because printing happens row by row:
1. We calculate the number of columns that can fit based on the terminal width and the longest filename plus spacing.
2. We calculate the number of rows needed by dividing total files by the number of columns.
3. For each row r, we print item r from column 0, item (r + num_rows) from column 1, item (r + 2 * num_rows) from column 2, and so on across the line.

A simple single loop through the filenames is insufficient because it prints files sequentially across the line, rather than down each column.

---

### Question 2: What is the purpose of the ioctl system call in this context? What would be the limitations of your program if you only used a fixed-width fallback (e.g., 80 columns) instead of detecting the terminal size?

#### Answer:
The ioctl system call with the TIOCGWINSZ request is used to get the current width (number of columns) of the terminal window directly from the terminal device driver.

If the program only used a fixed-width fallback of 80 columns:
1. On wider screens, the program would waste screen space by squeezing the files into 80 columns instead of using the available area.
2. On narrower screens (like smaller windows or split terminals), lines longer than the window would wrap around awkwardly, breaking the columns and making the output unreadable.

Using ioctl allows the program to automatically adjust its column layout to any terminal size.

---

## Feature-4: ls-v1.3.0 – Horizontal Column Display (-x Option)

### Question 1: Compare the implementation complexity of the "down then across" (vertical) printing logic versus the "across" (horizontal) printing logic. Which one requires more pre-calculation and why?

#### Answer:
The "down then across" (vertical) printing logic requires significantly more pre-calculation than the "across" (horizontal) logic.

1. "Down then across" requires calculating both the total number of columns and the total number of rows beforehand. Because the terminal can only print row by row, the program must calculate the exact array index for each column in every row (using the formula: index = row + column * num_rows). If this math is slightly off, files will appear in the wrong positions or exceed array bounds.

2. In contrast, the "across" (horizontal) logic is much simpler. It loops through the filenames sequentially from left to right. It only needs to know the column width and terminal width. As it prints each file, it keeps track of the horizontal position. When adding another column would exceed the screen width, it simply prints a newline character and resets the position to zero. No row calculations or non-linear index jumps are needed.

---

### Question 2: Describe the strategy you used in your code to manage the different display modes (-l, -x, and default). How did your program decide which function to call for printing?

#### Answer:
In the program, an integer variable named mode is used in the main function to track the selected display mode:
- Mode 0 represents the default vertical column display ("down then across").
- Mode 1 represents the long listing format (-l).
- Mode 2 represents the horizontal column display (-x).

During argument parsing with getopt(), if -l is encountered, mode is set to 1. If -x is encountered, mode is set to 2.

This mode integer is passed to the do_ls() function:
- If mode is 1, do_ls() gathers file metadata and prints the detailed long format.
- If mode is not 1, do_ls() reads the filenames into a dynamic array and checks the mode: if mode is 2, it calls the horizontal display function print_horizontal(); otherwise, it calls the default vertical display function print_columns().

---

## Feature-5: ls-v1.4.0 – Alphabetical Sort

### Question 1: Why is it necessary to read all directory entries into memory before you can sort them? What are the potential drawbacks of this approach for directories containing millions of files?

#### Answer:
A sorting algorithm needs to compare elements and rearrange them in order. System calls like readdir() only return one entry at a time in whatever order the file system stores them on disk. Because we cannot know which file comes first alphabetically without seeing all entries, we must read all filenames into memory before sorting.

The drawbacks of this approach for directories with millions of files are:
1. High Memory Consumption: Storing millions of filename strings in memory requires significant RAM. If memory runs out, the program can crash.
2. Output Delay: The program cannot show any output until all files are read into memory and sorted. The user must wait before seeing any results.

---

### Question 2: Explain the purpose and signature of the comparison function required by qsort(). How does it work, and why must it take const void * arguments?

#### Answer:
The purpose of the comparison function is to tell qsort() the ordering logic between two elements. Because qsort() is a generic sorting function that can sort any data type, it does not know the specific type of data it is sorting.

The signature is int comparison_function(const void *a, const void *b).
1. It takes const void * arguments because void * is a generic pointer in C that can point to any data type. This allows qsort() to work with arrays of any type. The const keyword ensures that the comparison function will not modify the data.
2. Inside the function, the void pointers are cast to pointers of the actual element type. For an array of strings (char *), the elements are pointers, so the arguments are cast to char **.
3. The function returns a negative number if the first item should come before the second, zero if they are equal, and a positive number if the first item should come after the second. For strings, strcmp() is used to return this comparison result.

---

## Feature-6: ls-v1.5.0 – Colorized Output Based on File Type

### Question 1: How do ANSI escape codes work to produce color in a standard Linux terminal? Show the specific code sequence for printing text in green.

#### Answer:
ANSI escape codes are special sequences of characters that begin with an Escape character (octal \033 or ASCII 27) followed by a bracket '['. When the terminal encounters this sequence in the output stream, it does not display the characters on the screen. Instead, it interprets them as a command to change display formatting such as foreground color, background color, or text style.

To print text in green, the specific escape sequence is \033[0;32m. The '0;' indicates normal text style and '32m' sets the foreground text color to green. After printing the text, the sequence \033[0m is printed to reset the terminal color back to default so following text is unaffected.

---

### Question 2: To color an executable file, you need to check its permission bits. Explain which bits in the st_mode field you need to check to determine if a file is executable by the owner, group, or others.

#### Answer:
In the st_mode field of struct stat, execute permissions are represented by three separate bits:
1. S_IXUSR: The execute bit for the owner (user).
2. S_IXGRP: The execute bit for the group.
3. S_IXOTH: The execute bit for others.

To determine if a file is executable by anyone, we combine these three macros using the bitwise OR operator (S_IXUSR | S_IXGRP | S_IXOTH) and check it against st_mode using the bitwise AND operator (&). If the result is non-zero, at least one execute permission bit is active, meaning the file is executable and should be displayed in green.
