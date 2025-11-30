#include <stdio.h>
#include "../include/Alex_libFS2025.h"
#include "../include/Alex_editor.h"


#define INPUT_BUF_SIZE 64
#define FILE_DATA_BUF_SIZE 2048

#define EDITOR_USE_MSG "The file editor allows you to write text to file.\nQuit Without Saving:\t'ctrl+q'\nSave and Quit:\t'ctrl+x'\n\nPress enter key to enter the editor or any other key to return to menu\n"


// Reads input and assigns to buffer argument
// Strips trailing newling
// returns non-zero if read successful
char *get_input(char *buffer, size_t size) {
    if(fgets(buffer, size, stdin) == NULL)
        return NULL;  // EOF or error

    // Remove trailing newline, if present
    buffer[strcspn(buffer, "\n")] = '\0';

    return buffer;
}


// Function to display the menu
void displayMenu() {
    printf("\n--- Menu ---\n");
    printf("1. Create a file\n");
    printf("2. Write to a file\n");
    printf("3. Read from a file\n");
    printf("4. List files\n");
    printf("5. Delete a file\n");
    printf("6. Exit\n");
    printf("Enter your choice: ");
}


// Creates files by name
// Name must be unique
void handleCreate() {
    // Prompt user for input filename
    char input[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to create: ");

    if(!get_input(input, INPUT_BUF_SIZE)) // Check for read failure
        return;

    fileCreate(input);
}


// Enters text editor and writes data to file
// If file name not valid you can enter editor but save will fail
// Overwrites any existing file content
// Automates file opening and closing for user
void handleWrite() {
    // Prompt user for input filename
    char file_name[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to write data to: ");

    if(!get_input(file_name, INPUT_BUF_SIZE)) // Check for read failure
        return;

    // Display editor instructions
    printf(EDITOR_USE_MSG);
    int key = getchar();

    // Enter editor if use presses enter key
    if(key == '\n')
        editFile(file_name);
}


// Displays content of file to output
// file name read from input
// Automates file opening and closing for user
void handleRead() {
    // Prompt user to enter filename to read
    char file_name[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to read: ");

    if(!get_input(file_name, INPUT_BUF_SIZE)) // Get filename to read
        return;

    int fd = fileOpen(file_name); // Open file

    if(fd == LIBFS_ERR) // Error opening file
        return;
    
    char file_data[FILE_DATA_BUF_SIZE]; // Data from file

    // Read file data
    int file_size = fileRead(fd, file_data, FILE_DATA_BUF_SIZE);

    if(file_size == LIBFS_ERR) // Check for file read error
        return;

    file_data[file_size] = '\0';

    if(file_size > 0) // Print file content
        printf("File Content:\n\n%s", file_data);

    fileClose(fd);
}


// Prints names and sizes of all saved files
// File size in Bytes
void handleList() {
    // Get array of files
    size_t num_files = 0;
    FileEntry** files = fileList(&num_files);
    
    if(num_files) // Print file table header
        printf("\n\nFile Name\t\tSize (B)\n");
    else // No files to display
        printf("No existing files");

    // Print name and size for all files
    for(size_t i = 0; i < num_files; i++)
        printf("%s\t\t%d\n", files[i]->filename, files[i]->size);
}


// Deletes file from file system
// File name is read from input
// Fails if name invalid
void handleDelete() {
    // Prompt user to enter filename to delete
    char file_name[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to delete: ");

    if(!get_input(file_name, INPUT_BUF_SIZE)) // Get filename to read
        return;

    fileDelete(file_name); // Delete file
}


// Run file manager and editor program
// Enters menu-driven TUI
// Allows users to create, delete, edit, and read files
int main() {
    int choice; // Stores user selection
    int files_loaded = libFSLoad(); // Load file(s) from previous sessions

    // Display intro messages
    printf("\n\nWelcome to xfile file-editor and file-system simulator!");
    printf("\n%d files have been loaded from previous sessions", files_loaded);

    while(1) {
        displayMenu(); // Display menu
        scanf("%d", &choice); // Get user choice
        getchar(); // Get trailing newline

        switch(choice) {
            case 1: // Handle file creation
                handleCreate();
                break;
            case 2: // Handle file writing
                handleWrite();
                break;
            case 3: // Display file content
                handleRead();
                break;
            case 4: // List file names and sizes
                handleList();
                break;
            case 5: // Handle file deletion
                handleDelete();
                break;
            case 6: // Exit program
                exit(0);
                break;
        }
    }

    return 0;
}