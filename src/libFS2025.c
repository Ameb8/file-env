#include <unistd.h>

#include "../include/queue.h"

#include "libFS2025.h"


// File store directory config
#define LIBFS_BASE_DIR ".fsdata/" // Path from project root to where files are saved


// Invalid file provided by user
#define ERR_MSG_FNE "Error: File '%s' does not exists.\n" 
#define ERR_MSG_FAE "Error: File '%s' already exists.\n"
#define ERR_MSG_IDXNE "Error: File descriptor '%d' does not point to valid file.\n"

// Invalid file open status for requested action
#define ERR_MSG_FC "Error: File '%s' is closed.\n"
#define ERR_MSG_FO "Error: File '%s' is already open.\n"

// System errors when working with files
#define ERR_MSG_CND "Error: File '%s' could not be deleted.\n"
#define ERR_MSG_CNC "Error: Unable to create file '%s'.\n"

// Success messages when output succeeds
#define SCS_MSG_FW "Data written to file '%s' successfully.\n"


// Global variables to track state
FileEntry file_table[MAX_FILES] = {0}; // File table to track files where index serves as virtual file descriptor
int file_count = 0; // Number of files in the system
int file_end = 0; // Tracks highest used virtual descriptor for file storage 
Queue free_mem = { NULL, 0 }; // Holds open indices in table less than file count


static void buildFullPath(char *out, const char *filename) {
    snprintf(out, MAX_FILENAME + 20, "%s%s", LIBFS_BASE_DIR, filename);
}


// Returns index of file if in memory
int findFile(const char* filename) {
    for (int i = 0; i < file_count; i++) {
        if(file_table[i].exists && strcmp(file_table[i].filename, filename) == 0)
            return i;
    }

    return LIBFS_ERR;
}


// Create a new file
int fileCreate(const char *filename) {
    if(findFile(filename) != LIBFS_ERR) {
        printf(ERR_MSG_FAE, filename);
        return LIBFS_ERR;
    }

    int mem_idx = file_count; // Default mem_idx to next open spot

    if(queueSize(&free_mem)) // Set fragmented mem idx if exists
        mem_idx = dequeue(&free_mem);
    else // Increment end of contiguously stored files
        file_end++;

    // Get full path to local file
    char fullpath[MAX_FILENAME + 20];
    buildFullPath(fullpath, filename);

    // Create the file on the local disk
    FILE *file = fopen(fullpath, "w");
    if (!file) {
        printf(ERR_MSG_CNC, filename);
        return -1;
    }
    fclose(file);

    // Add file to the file table
    strcpy(file_table[mem_idx].filename, filename);
    file_table[mem_idx].size = 0;
    file_table[mem_idx].is_open = 0;  // File defaults as closed
    file_table[mem_idx].exists = 1;
    file_count++;

    printf("File '%s' created successfully.\n", filename);
    return 0;
}

// Open a file
int fileOpen(const char *filename) {
    int open_idx = findFile(filename); // Get file mem location

    // Validate by name that file exists
    if(open_idx == LIBFS_ERR) {
        printf(ERR_MSG_FNE, filename);
        return LIBFS_ERR;
    }

    // Validate file not already open
    if(file_table[open_idx].is_open) {
        printf(ERR_MSG_FO, filename);
        return LIBFS_ERR;
    }

    // File opened successfully 
    file_table[open_idx].is_open = 1; // Set file as open
    return open_idx; // Return virtual file descriptor
}


// Write data to a file
int fileWrite(int file_index, const char *data) {
    // Ensure file index is valid
    if(!file_table[file_index].exists) {
        printf(ERR_MSG_IDXNE, file_index);
        return LIBFS_ERR;
    }
    
    // Ensure file is open
    if(!file_table[file_index].is_open) {
        printf(ERR_MSG_FC, file_table[file_index].filename);
        return LIBFS_ERR;
    }

    int data_size = strlen(data);

    // Get full path to local file
    char fullpath[MAX_FILENAME + 20];
    buildFullPath(fullpath, file_table[file_index].filename);

    // Open local file to write data
    FILE *file = fopen(fullpath, "w");

    if (!file) { // Error opening file
        printf("Error: Unable to open file '%s' for writing.\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    fwrite(data, 1, data_size, file); // Write data
    fclose(file); // Close file

    file_table[file_index].size = data_size; // Update file size metadata
    printf(SCS_MSG_FW, file_table[file_index].filename); 

    return 0;
}

// Read data from a file
int fileRead(int file_index, char *buffer, int buffer_size) {
    if(!file_table[file_index].is_open) { // Check that file is open in LIBFS
        printf("Error: File '%s' is not open.\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    if(file_table[file_index].size < 1) {
        printf("File '%s' is empty, no data to read\n", file_table[file_index].filename);
        return 0;
    }
 
    // Get full path to local file
    char fullpath[MAX_FILENAME + 20];
    buildFullPath(fullpath, file_table[file_index].filename);

    // Attempt to open file
    FILE* file = fopen(fullpath, "r");

    if(!file) { // File failed to open
        printf("Error: Unable to open file '%s' for writing.\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    // Provided buffer too small for data
    if(file_table[file_index].size > buffer_size) {
        return LIBFS_ERR;
    }

    // Read data into buffer
    size_t bytes_read = fread(buffer, 1, file_table[file_index].size, file);
    fclose(file); // Close local file

    if(bytes_read > 0) {
        printf("%zu bytes of data read from file '%s' successfully.\n", bytes_read, file_table[file_index].filename);
    } else { 
        printf("Failed to read data from file %s\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    return bytes_read;
}


// Close a file
int fileClose(int file_index) {
    // Ensure file index is valid
    if(!file_table[file_index].exists) {
        printf(ERR_MSG_IDXNE, file_index);
        return LIBFS_ERR;
    }

    // Ensure file is open
    if(!file_table[file_index].is_open) {
        printf(ERR_MSG_FC, file_table[file_index].filename);
        return LIBFS_ERR;
    }

    file_table[file_index].is_open = 0; // Mark file as closed

    return 0;
}

// Delete a file
int fileDelete(const char *filename) {
    // Search for file by name in memory
    int delete_idx = findFile(filename); 

    // File does not exist
    if(delete_idx == LIBFS_ERR) {
        printf(ERR_MSG_FNE, filename);
        return LIBFS_ERR;
    }

    // Get full path to local file
    char fullpath[MAX_FILENAME + 20];
    buildFullPath(fullpath, filename);

    // Delete file from system
    if(unlink(fullpath)) {
        printf(ERR_MSG_CND, file_table[delete_idx].filename);
        return LIBFS_ERR;
    }

    // Mark file as DNE
    file_table[delete_idx].exists = 0;

    if(delete_idx == file_end - 1) // Remove file from end of memory
        file_end--;
    else // Removing file will fragment memory, store open position
        enqueue(&free_mem, delete_idx);
    
    file_count--; // Decrement total file count
    return 0;
}

FileEntry** fileList(size_t* num_files) {
    FileEntry** files = malloc(file_count * sizeof(FileEntry*));
    *num_files=0;

    for(int i = 0; i < file_end; i++) {
        if(file_table[i].exists)
            files[(*num_files)++] = &file_table[i];
    }

    return files;
}