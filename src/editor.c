#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/queue.h"

#include "../include/libFS2025.h"


#define MAX_LINES 5000

// Data to track editor state
char* filename = NULL; // Name of open file
char exit_editor; // Set when graceful exit requested
struct termios orig_termios; // Holds terminal state and config
char *lines[MAX_LINES]; // Holds each line of data in buffer
int line_count = 0; // Number of lines in buffer
int cx = 0, cy = 0; // cursor x and y position


// Saves currently open file
// Returns non-zero on success
// Handles file opening and closing automatically
char saveFile(char* content) {
    // Get virtual file descriptor
    int descriptor = fileOpen(filename);
    
    if(descriptor == -1) // File open error
        return 0;

    // Update file data
    if(fileWrite(descriptor, content) == LIBFS_ERR)
        return 0;
    
    fileClose(descriptor); // Close file

    return 1;
}


// Function to ensure cursor stays withing file bounds
// Moves cuser into valid position if buffer bounds exceeded
void clamp_cursor() {
    // Ensure cursor is in valid vertical position
    if(cy < 0) cy = 0; // Handle cursor before first line
    if(cy >= line_count) cy = line_count - 1; // Handle cursor after last line

    int len = strlen(lines[cy]); // Get length of current line
    
    // Ensure cursor within start and end of line
    if(cx < 0) cx = 0; // Handle cursor before line start
    if(cx > len) cx = len; // Handle cursor after line end
}


// Exit program with error message
// 's' argument displayed as error
void die(const char *s) {
    perror(s); // Display error
    exit(1); // Exit program with error code
}


// Restores terminal for regular use
// Exits program if restore fails
void disableRawMode() {
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor

    // Attempt to restore original terminal settings
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr"); // Exit program if restoration fails
}


// Sets terminal to raw mode
// Configures terminal to set up file editor
// Disables signal generation from keyboard until disabled
void enableRawMode() {
    // Get terminal's current settings
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr"); // Exit on fialure

    atexit(disableRawMode); // Ensure raw mode disable on exit

    // Struct to hold new terminal settings for during editing
    struct termios raw = orig_termios;

    // Modify terminal config flags for raw mode
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_oflag &= ~(OPOST);

    // Flush input before applying confing changes
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr"); // Exit on failure
}


// Inserts char 'c' into buffer at location ('x', 'y')
void insert_char(int y, int x, char c) {
    char *line = lines[y]; // Get current line
    int len = strlen(line); // Get length of current line

    if(x > len) x = len; // clamp cursor location

    // Add new char at selected location
    line = realloc(line, len + 2); // Increase length of line in memory
    memmove(&line[x+1], &line[x], len - x + 1); // Shift line data if needed
    line[x] = c; // Add new char
    lines[y] = line; // Update pointer to new line
}


// Deletes char at given location
void delete_char(int y, int x) {
    char *line = lines[y]; // Get line being deleted from
    int len = strlen(line); // Get length of line
    if(x == 0 || len == 0) return; // Validate cursor within line bounds

    memmove(&line[x-1], &line[x], len - x + 1); // Shift line data
}


// Get text from editor
// Returns full buffer as char*
// Appends '\0' to returned data
// Caller must free returned data
char* get_full_text() {
    size_t total = 0; // Total number of bytes in buffer

    // Calculate total size
    for (int i = 0; i < line_count; i++)
        total += strlen(lines[i]) + 1; // +1 for '\n'

    // Allocate space for buffer data
    char *buffer = malloc(total + 1);
    buffer[0] = '\0';

    // Copy lines into buffer
    for (int i = 0; i < line_count; i++) {
        strcat(buffer, lines[i]); // Copy line data
        strcat(buffer, "\n"); // Append newline
    }

    return buffer; // Return pointer to start of data
}


// Clear terminal screen and hides cursor
void clear_screen() {
    write(STDOUT_FILENO, "\x1b[?25l", 6);  // hide cursor
    write(STDOUT_FILENO, "\x1b[2J", 4);    // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);     // move cursor home
}
 

// Renders editor screen from scratch
// Prints every line sequentially
// Updates and displays cursor to valid position
// Hides cursor during rendering to prevent flicker
// Causes incorrect rendering if file taller than terminal window
void draw_screen() {
    // Clear screen & move cursor to top-left
    write(STDOUT_FILENO, "\x1b[3J\x1b[H\x1b[2J", 10);

    write(STDOUT_FILENO, "\x1b[?25l", 6); // Hide cursor

    // Draw all lines without scrolling
    for(int i = 0; i < line_count; i++) {
        write(STDOUT_FILENO, lines[i], strlen(lines[i]));
        write(STDOUT_FILENO, "\r\n", 2);
    }

    // Draw the cursor to (cx, cy)
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy+1, cx+1);
    write(STDOUT_FILENO, buf, strlen(buf));

    write(STDOUT_FILENO, "\x1b[?25h", 6); // Redraw cursor
}


// Reads single key press from stdin when in raw mode
// Keys read immediately as line buffering is disabled
// Returns navigation code if arrrow key
// Otherwise returns normal character
int read_key() {
    char c; // Stores byte froms tdin
    while (read(STDIN_FILENO, &c, 1) == 0); // Read single byte

    if(c == '\x1b') { // If escape sequence of ESC key
        char seq[2]; // Stores bytes if escape sequence

        // Treat as ESC key if next 2 bytes cannot be read
        if (read(STDIN_FILENO, &seq[0], 1) == 0) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) == 0) return '\x1b';

        if(seq[0] == '[') { // Read escape sequences for arrow keys
            switch(seq[1]) { // Determine which arrow key
                case 'A': return 'U'; // Up arrow
                case 'B': return 'D'; // Down arrow
                case 'C': return 'R'; // Right arrow
                case 'D': return 'L'; // Left arrow
            }
        }

        return '\x1b'; // Treat unkown sequences as ESC key
    }

    return c; // Return normal character
}


// Handle ctrl + x key presses from user
// Saves file to virtual file system and closes editor
void handle_ctrl_x() {
    char *text = get_full_text(); // Get text from buffer
    saveFile(text); // Save to file system

    free(text); // Free text buffer memory
    exit_editor = 1; // Signal editor exit
}


// Processes a single key press
// Handles navigation, control keys, and normal characters to write
void process_key(int k) {
    switch (k) { // Handle all key presses
        // Handle navigation key presses
        case 'L': if (cx > 0) cx--; break;
        case 'R': cx++; break;
        case 'U': if (cy > 0) cy--; break;
        case 'D': if (cy < line_count - 1) cy++; break;
        
        case '\r': {  // Handle enter key for newline
            char *line = lines[cy]; // Get line being split

            // Split current line into left + right parts
            char *left = strndup(line, cx); // Text before newline
            char *right = strdup(line + cx); // Text after newline

            // Replace current line with left part
            lines[cy] = left;

            // Shift lines to make room for new line
            for (int i = line_count; i > cy + 1; i--)
                lines[i] = lines[i - 1];

            // Insert right part as new line
            lines[cy + 1] = right;
            line_count++;

            // Update cursor position
            cy++;
            cx = 0;

            break;
        }

        case 17: // Handle editor exit without saving
            exit_editor = 1; // ctrl+q to quit without saving

        case 24:  // Handle editor save and quit
            handle_ctrl_x(); // ctrl+x to save and quit
            break;

        case 127: // Handle backspace key to delete char
            if(cx > 0) { // Ensure valid cursor position
                delete_char(cy, cx); // Delete character
                cx--; // Update cursor position
            }
            break;

        default: // Insert character if non-special key
            if(k >= 32 && k <= 126) { // Ensure valid char
                insert_char(cy, cx, k); // write char to text buffer
                cx++; // Update cursor position
            }
            break;
    }

    clamp_cursor(); // Ensure curser remains in valid position
}


// Initialize and run file editor
// If filename invalid editor will load but not save
// All files will load as empty and overwrite old data if saved
// Returns zero on success
int editFile(char* editFilename) {
    if(!editFilename) // Valdate input
        return -1;

    // Set editor state
    exit_editor = 0;
    filename = editFilename;
    
    // Reset editor text state
    for(int i = 0; i < line_count; i++) {
        free(lines[i]);
        lines[i] = NULL;
    }

    // Reset cursor and line counts
    line_count = 0;
    cx = 0;
    cy = 0;
    
    enableRawMode();

    // Clear the screen BEFORE printing a newline
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);

    // Move below the shell prompt
    write(STDOUT_FILENO, "\n", 1);

    // Load one empty line
    lines[0] = strdup("");
    line_count = 1;

    // Run editor until ext
    while (!exit_editor) {
        draw_screen(); // Render screen
        int key = read_key(); // Read next key press
        process_key(key); // Process next key press
    }

    // Ensure graceful exit 
    disableRawMode();
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7); // clear screen on exit
    
    return 0;  
}
