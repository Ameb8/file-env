#include <stdio.h>
#include "../include/libFS2025.h"
#include "../include/editor.h"


#define INPUT_BUF_SIZE 64
#define FILE_DATA_BUF_SIZE 2048

#define EDITOR_USE_MSG "The file editor allows you to write text to file.\nQuit Without Saving:\t'ctrl+q'\nSave and Quit:\t'ctrl+x'\n\nPress enter key to enter the editor or any other key to return to menu\n"

char *get_input(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) == NULL) {
        return NULL;  // EOF or error
    }

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


void handleView() {
    size_t num_files = 0;
    FileEntry** files = fileList(&num_files);
    
    if(num_files)
        printf("\n\nFile Name\t\tSize (B)\n");
    else
        printf("No existing files");

    for(int i = 0; i < num_files; i++)
        printf("%s\t\t%d\n", files[i]->filename, files[i]->size);
}

void handleCreate() {
    char input[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to create: ");

    if(!get_input(input, INPUT_BUF_SIZE))
        return;

    fileCreate(input);
}

void handleOpen() {
}

void handleWrite() {
    char file_name[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to write data to: ");

    if(!get_input(file_name, INPUT_BUF_SIZE))
        return;

    // Display editor instructions
    printf(EDITOR_USE_MSG);
    int key = getchar();

    // Enter editor if use presses enter key
    if(key == '\n')
        editFile(file_name);
}

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
    size_t file_size = fileRead(fd, file_data, FILE_DATA_BUF_SIZE);

    if(file_size == LIBFS_ERR) // Check for file read error
        return;

    file_data[file_size] = '\0';

    if(file_size > 0) // Print file content
        printf("File Content:\n\n%s", file_data);

    fileClose(fd);
}


void handleList() {
    size_t num_files = 0;
    FileEntry** files = fileList(&num_files);
    
    if(num_files)
        printf("\n\nFile Name\t\tSize (B)\n");
    else
        printf("No existing files");

    for(int i = 0; i < num_files; i++)
        printf("%s\t\t%d\n", files[i]->filename, files[i]->size);
}


void handleClose() {
    
}

void handleDelete() {
    // Prompt user to enter filename to delete
    char file_name[INPUT_BUF_SIZE];
    printf("Enter the name of the file you would like to delete: ");

    if(!get_input(file_name, INPUT_BUF_SIZE)) // Get filename to read
        return;

    fileDelete(file_name); // Delete file
}


void handleExit() {

}


int main() {
    int choice; // Stores user selection
    int files_loaded = libFSLoad(); // Load file(s) from previous sessions

    // Display intro messages
    printf("\n\nWelcome to xfile file-editor and file-system simulator!");
    printf("\n%d files have been loaded from previous sessions", files_loaded);

    while(1) {
        displayMenu();
        scanf("%d", &choice);  // Get user choice
        getchar();

        switch(choice) {
            case 1:
                handleCreate();
                break;
            case 2:
                handleWrite();
                break;
            case 3:
                handleRead();
                break;
            case 4:
                handleList();
                break;
            case 5:
                handleDelete();
                break;
            case 6:
                exit(0);
                break;
        }
    }
}