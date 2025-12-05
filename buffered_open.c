#define _GNU_SOURCE 
#include "buffered_open.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h> 
#include <stdio.h> 
#include <fcntl.h>

// Note: The structure members bf->wb_stored and bf->rb_stored are NOT defined in buffered_open.h
// I have removed references to them.

buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    // 1. Handle mode argument for O_CREAT/O_TMPFILE
    mode_t mode = 0;
    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    // 2. Allocate buffered_file_t
    buffered_file_t *bf = malloc(sizeof(buffered_file_t));
    if (bf == NULL) {
        // errno is set by malloc
        return NULL;
    }

    // 3. Allocate buffers
    bf->read_buffer = malloc(BUFFER_SIZE);
    bf->write_buffer = malloc(BUFFER_SIZE);
    
    if (bf->read_buffer == NULL || bf->write_buffer == NULL) {
        // Use perror correctly (no extra quotes or newlines)
        // Set specific error for memory failure, if needed, before calling perror.
        errno = ENOMEM;
        perror("buffered_open: memory allocation error"); 
        
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    // 4. Initialize members
    bf->read_buffer_size = BUFFER_SIZE;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;
    
    // Handle O_PREAPPEND flag
    bf->preappend = (flags & O_PREAPPEND) ? 1 : 0; 
    bf->flags = flags & ~O_PREAPPEND; // Remove O_PREAPPEND before opening

    // Initialize required state for synchronization
    bf->last_operation = 0;
    bf->file_offset = 0; // Assuming we're tracking this

    // 5. Open file (using the mode argument if needed)
    bf->fd = open(pathname, bf->flags, mode); 
    if (bf->fd == -1) {
        perror("buffered_open: file open error");
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }
    return bf;
}

ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    // Implementation needed here
    return -1;
}

ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    if (bf == NULL || buf == NULL || bf->fd == -1) {
        if (count == 0) return 0;
        errno = EBADF;
        return -1;
    }
    if (count == 0) return 0;

    size_t total_written = 0;
    const char *src = (const char *)buf;
   
    while (total_written < count) { 
        size_t bytes_remaining_in_input = count - total_written;
        size_t space_left_in_buffer = bf->write_buffer_size - bf->write_buffer_pos;

        // --- Flush Check: Only flush when buffer is full ---
        if (space_left_in_buffer == 0) {
            if (buffered_flush(bf) == -1) {
                perror("buffered_write: flush error");
                return total_written > 0 ? (ssize_t)total_written : -1;
            }
            // After successful flush, the entire buffer is free
            space_left_in_buffer = bf->write_buffer_size;
        }

        // --- Determine amount to copy ---
        size_t to_copy = bytes_remaining_in_input;
        if (to_copy > space_left_in_buffer) {
            to_copy = space_left_in_buffer;
        }

        // --- Copy data and update state ---
        memcpy(bf->write_buffer + bf->write_buffer_pos, src + total_written, to_copy);
        
        bf->write_buffer_pos += to_copy;
        
        // Removed: bf->wb_stored += to_write; (Undefined member)
        
        bf->last_operation = 2; // Indicate the last operation was a write
        total_written += to_copy;
    }

    return (ssize_t)total_written;
}

int buffered_flush(buffered_file_t *bf) {
    // Note: O_PREAPPEND logic is currently missing here. This is the simple flush.
    
    if (bf == NULL || bf->fd == -1) {
        perror("buffered_flush: invalid file descriptor or pointer");
        return -1;
    }
    if (bf->write_buffer_pos == 0) {
        return 0; // Nothing to flush
    }
    
    size_t total_written = 0;
    while (total_written < bf->write_buffer_pos) {
        ssize_t written = write(bf->fd, bf->write_buffer + total_written, bf->write_buffer_pos - total_written);
        if (written == -1) {
            // write sets errno
            perror("buffered_flush: write error"); 
            return -1;
        }
        total_written += written;
    }
    
    bf->write_buffer_pos = 0;
    // Removed: bf->wb_stored = 0; (Undefined member)
    
    return 0;
}

int buffered_close(buffered_file_t *bf) {
    // Implementation needed here
    return -1;
}