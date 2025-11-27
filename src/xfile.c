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
    printf("2. Open a file\n");
    printf("3. Write to a file\n");
    printf("4. Read from a file\n");
    printf("5. List files\n");
    printf("6. Close a file\n");
    printf("7. Delete a file\n");
    printf("8. Exit\n");
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

}


void handleExit() {

}


int main() {
    int choice;

    while (1) {
        displayMenu();
        scanf("%d", &choice);  // Get user choice
        getchar();

        switch(choice) {
            case 1:
                handleCreate();
                break;
            case 2:
                handleOpen();
                break;
            case 3:
                handleWrite();
                break;
            case 4:
                handleRead();
                break;
            case 5:
                handleList();
                break;
            case 6:
                handleClose();
                break;
            case 7:
                handleDelete();
                break;
            case 8:
                exit(0);
                break;
        }
    }
}