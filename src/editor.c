

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/queue.h"

#include "../include/libFS2025.h"


#define MAX_LINES 5000

char* filename = NULL;
char exit_editor;

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


struct termios orig_termios;
char *lines[MAX_LINES];
int line_count = 0;
int cx = 0, cy = 0; // cursor x and y

void clamp_cursor() {
    if (cy < 0) cy = 0;
    if (cy >= line_count) cy = line_count - 1;

    int len = strlen(lines[cy]);
    if (cx < 0) cx = 0;
    if (cx > len) cx = len;  // do not allow moving past line end
}


void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}


void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
        die("tcgetattr");

    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_oflag &= ~(OPOST);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}



void insert_char(int y, int x, char c) {
    char *line = lines[y];
    int len = strlen(line);

    if(x > len) x = len;  // clamp

    line = realloc(line, len + 2);
    memmove(&line[x+1], &line[x], len - x + 1);
    line[x] = c;

    lines[y] = line;
}


void delete_char(int y, int x) {
    char *line = lines[y];
    int len = strlen(line);
    if (x == 0 || len == 0) return;

    memmove(&line[x-1], &line[x], len - x + 1);
}

// Get text from editor
char* get_full_text() {
    size_t total = 0;

    // Calculate total size
    for (int i = 0; i < line_count; i++) {
        total += strlen(lines[i]) + 1; // +1 for '\n'
    }

    char *buffer = malloc(total + 1);
    buffer[0] = '\0';

    // Copy lines into buffer
    for (int i = 0; i < line_count; i++) {
        strcat(buffer, lines[i]);
        strcat(buffer, "\n");
    }

    return buffer;
}

// Clear screen
void clear_screen() {
    write(STDOUT_FILENO, "\x1b[?25l", 6);  // hide cursor
    write(STDOUT_FILENO, "\x1b[2J", 4);    // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);     // move cursor home
}
 

void draw_screen() {
    // Clear screen & move cursor to top-left
    //write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    write(STDOUT_FILENO, "\x1b[3J\x1b[H\x1b[2J", 10);

    write(STDOUT_FILENO, "\x1b[?25l", 6); // Hide cursor


    // Draw all lines WITHOUT scrolling
    for (int i = 0; i < line_count; i++) {
        write(STDOUT_FILENO, lines[i], strlen(lines[i]));
        write(STDOUT_FILENO, "\r\n", 2);
    }

    // Draw the cursor
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy+1, cx+1);
    write(STDOUT_FILENO, buf, strlen(buf));

    write(STDOUT_FILENO, "\x1b[?25h", 6);
}





int read_key() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 0);

    if (c == '\x1b') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 0) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) == 0) return '\x1b';

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'U';
                case 'B': return 'D';
                case 'C': return 'R';
                case 'D': return 'L';
            }
        }
        return '\x1b';
    }

    return c;
}


void handle_ctrl_x() {
    disableRawMode(); 
    printf("ctrl+x pressed\n"); // DEBUG *******
    fflush(stdout);

    char *text = get_full_text();
    saveFile(text);

    //disableRawMode();  // temporarily restore terminal
    printf("\nCollected text (%zu bytes):\n", strlen(text));
    printf("---- BEGIN ----\n%s---- END ----\n", text);

    free(text);
    exit_editor = 1;
}


void process_key(int k) {
    switch (k) {
        case 'q': exit(0); // quit

        case 'L': if (cx > 0) cx--; break;
        case 'R': cx++; break;
        case 'U': if (cy > 0) cy--; break;
        case 'D': if (cy < line_count - 1) cy++; break;
        case '\r': {  // ENTER
            char *line = lines[cy];
            int len = strlen(line);

            // Split current line into left + right parts
            char *left = strndup(line, cx);
            char *right = strdup(line + cx);

            // Replace current line with left part
            lines[cy] = left;

            // Make room for new line
            for (int i = line_count; i > cy + 1; i--) {
                lines[i] = lines[i - 1];
            }

            // Insert right part as new line
            lines[cy + 1] = right;
            line_count++;

            cy++;
            cx = 0;
            break;
        }

        case 24:  // CTRL-X
            handle_ctrl_x();
            break;

        case 127: // Backspace
            if (cx > 0) {
                delete_char(cy, cx);
                cx--;
            }
            break;

        default:
            if (k >= 32 && k <= 126) {
                insert_char(cy, cx, k);
                cx++;
            }
            break;
    }

    clamp_cursor();
}


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

    while (!exit_editor) {
        draw_screen();
        int key = read_key();
        if (key == 'q') break;
        process_key(key);
    }

    disableRawMode();
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7); // clear screen on exit
    return 0;
}
