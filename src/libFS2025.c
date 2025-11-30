#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

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

// Macro to check if file descriptor points to valid file
#define FD_VALID(fd) (fd >= 0 && fd < file_end && file_table[fd].exists)

// Global variables to track state
FileEntry file_table[MAX_FILES] = {0}; // File table to track files where index serves as virtual file descriptor
int file_count = 0; // Number of files in the system
int file_end = 0; // Tracks highest used virtual descriptor for file storage 
Queue free_mem = { NULL, 0 }; // Holds open indices in table less than file count


// Constructs full filepath from filename
// Path relative from project root directory
// Result assigned to out argument
static void buildFullPath(char* out, const char* filename) {
    snprintf(out, MAX_FILENAME + 20, "%s%s", LIBFS_BASE_DIR, filename);
}


// Returns index of file if in memory
// Searches by file name
int findFile(const char* filename) {
    for (int i = 0; i < file_count; i++) { // Check all virtual memory addresses
        if(file_table[i].exists && strcmp(file_table[i].filename, filename) == 0)
            return i; // File found
    }

    return LIBFS_ERR; // File not found
}


// Create a new file
// Defaults as empty and closed
// File saved in LIBFS_BASE_DIR path from project root
int fileCreate(const char *filename) {
    // Check name for uniqueness
    if(findFile(filename) != LIBFS_ERR) {
        // Name already exists
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

    if(!file) { // Failure opening file
        printf(ERR_MSG_CNC, filename);
        return LIBFS_ERR;
    }

    fclose(file); // Close file on host system

    // Add file to the file table
    strcpy(file_table[mem_idx].filename, filename); // Copy filename
    file_table[mem_idx].size = 0; // Set file as empty
    file_table[mem_idx].is_open = 0; // File defaults to closed
    file_table[mem_idx].exists = 1; // FileEntry is valid file 
    file_count++;

    // File created successfully
    printf("File '%s' created successfully.\n", filename);

    return 0;
}


// Open a file in virtual file system
// Does not open file on host system
// fails if file is already open or does not exist
// Returns file descriptor on success
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
// Overwrites existing data
// Fails if file closed or index invalid
// Returns number of bytes written
int fileWrite(int file_index, const char *data) {
    // Ensure file index is valid
    if(!FD_VALID(file_index)) {
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

    return data_size;
}


// Read data from a file
// Fails if file not open or index invalid
// 'buffer_size' or less of file data is written to 'buffer' arg
// Returns number of bytes of file data successfully written to 'buffer'
int fileRead(int file_index, char *buffer, int buffer_size) {
    if(!FD_VALID(file_index) || !file_table[file_index].is_open) { // Check that file is open and index valid in LIBFS
        printf("Error: File '%s' is not open.\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    if(file_table[file_index].size < 1) { // File is empty
        printf("File '%s' is empty, no data to read\n", file_table[file_index].filename);
        return 0;
    }

    if(!buffer || buffer_size < 1) { // Validate buffer for file data
        printf("Error: Buffer for file data is corrupted.\n");
        return LIBFS_ERR;
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
    if(file_table[file_index].size > buffer_size)
        return LIBFS_ERR;

    // Read data into buffer
    size_t bytes_read = fread(buffer, 1, file_table[file_index].size, file);
    fclose(file); // Close local file

    if(bytes_read > 0) { // Display number of bytes read
        printf("%zu bytes of data read from file '%s' successfully.\n", bytes_read, file_table[file_index].filename);
    } else { // No bytes read
        printf("Failed to read data from file %s\n", file_table[file_index].filename);
        return LIBFS_ERR;
    }

    return bytes_read; // Return number of bytes read
}


// Close a file
// Closes file based off file descriptor argument
// Fails if bad descriptor or file not open
// Returns zero on success
int fileClose(int file_index) {
    if(!FD_VALID(file_index)) { // Ensure file index is valid
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


// Delete a file from virtual file system
// Files accessed by name
// Returns zero on success
// May cause memory fragmentation in virtual file system
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


// Returns table with metadata for each file in virtual system
// Does not offer direct access to file content
// Caller must free files array but not individual FileEntrys
// Modification of FileEntry's by user will cause undefined behavior
// Size of returned array written to num_files arg
FileEntry** fileList(size_t* num_files) {
    FileEntry** files = malloc(file_count * sizeof(FileEntry*));
    *num_files=0;

    if(files) { // Only add files if arrray created successfully
        for(int i = 0; i < file_end; i++) {
            if(file_table[i].exists) // Only add valid file metadata
                files[(*num_files)++] = &file_table[i];
        }
    }

    return files;
}


// Loads files created in previous sessions into virtual file system memory
// Loads any normal files at LIBFS_BASE_DIR-defined path
// Manual creation of none-text files in LIBFS_BASE_DIR may cause undefined behavior
// Example: symlinks, directories, executables, etc.
int libFSLoad() {
    DIR *dir = opendir(LIBFS_BASE_DIR); // Attempt to open file-storage directory

    if(!dir) { // Error opening directory
        printf("Error opening FS base directory");
        return LIBFS_ERR;
    }

    // Read entries from directory
    struct dirent *entry;
    int files_read = 0;

    while((entry = readdir(dir)) != NULL) { // Iterate files in directory
        // Skip files '.', '..', and '.gitkeep'
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strcmp(entry->d_name, ".gitkeep") == 0)
            continue;

        // Get fullpath to file
        char fullpath[MAX_FILENAME + 20];
        buildFullPath(fullpath, entry->d_name);

        struct stat st;
        if(stat(fullpath, &st) != 0) // Skip if stat() fails
            continue; 

        if(!S_ISREG(st.st_mode)) // Skip irregular files (dirs, symlinks)
            continue;

        if(file_count >= MAX_FILES) // Stop if no space to load
            break;

        files_read++; // File will be loaded, increment count

        // Determine virtual memory location for file
        int mem_idx;
        if(queueSize(&free_mem)) // Use open slot within used virtual memory
            mem_idx = dequeue(&free_mem);
        else // No fragmentation, store at end of virtual memory
            mem_idx = file_end++;

        // Populate table entry
        strcpy(file_table[mem_idx].filename, entry->d_name);
        file_table[mem_idx].size = st.st_size;
        file_table[mem_idx].is_open = 0;
        file_table[mem_idx].exists = 1;

        file_count++;
    }

    closedir(dir);
    return files_read;
}