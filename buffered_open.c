#define _GNU_SOURCE 
#include "buffered_open.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h> 
#include <stdio.h> 
#include <fcntl.h>

buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    // 1.handle mode argument for O_CREAT/O_TMPFILE
    mode_t mode = 0;
    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    // 2.allocate buffered_file_t
    buffered_file_t *bf = malloc(sizeof(buffered_file_t));
    if (bf == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    // 3.allocate buffers
    bf->read_buffer = malloc(BUFFER_SIZE);
    bf->write_buffer = malloc(BUFFER_SIZE);
    
    if (bf->read_buffer == NULL || bf->write_buffer == NULL) {
        errno = ENOMEM;
        perror("buffered_open: memory allocation error"); 
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    // 4.initialize fields
    bf->read_buffer_size = BUFFER_SIZE;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;
    bf->preappend = (flags & O_PREAPPEND) ? 1 : 0; 
    bf->flags = flags & ~O_PREAPPEND; // remove O_PREAPPEND before opening
    bf->last_operation = 0;
    bf->file_offset = 0; 

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
    if (bf == NULL || buf == NULL || bf->fd == -1) {
        errno = EBADF;
        perror("buffered_read: invalid buffered_file_t or buffer");
        return -1;
    }
    if (count == 0) return 0;
    // flush the write buffer when switching from writing to reading
    if (bf->last_operation == 2) { // 2 = Write
        if (buffered_flush(bf) == -1) {
            perror("buffered_read: failed to flush write buffer before reading");
            return -1;
        }
        // notice that after flushing, file offset is moved to the end of the written data
    }
    bf->last_operation = 1; //1 = Read

    size_t total_read = 0;
    char *dest = (char *)buf;

    while (total_read < count) {
        size_t in_buffer = bf->read_buffer_size - bf->read_buffer_pos;//calc data in buffer
        if (in_buffer == 0) {//if read buffer is empty
            ssize_t bytes_read = read(bf->fd, bf->read_buffer, BUFFER_SIZE);

            if (bytes_read == 0) {
                return total_read;//EOF reached
            }
            if (bytes_read < 0) {
                perror("buffered_read: underlying read error");
                return total_read > 0 ? (ssize_t)total_read : -1;
            }
            bf->read_buffer_size = bytes_read; //total bytes read into buffer
            bf->read_buffer_pos = 0;          //go to the beginning
            in_buffer = bytes_read; 
        }

        //calculate how many bytes we can copy from the buffer
        size_t bytes_needed = count - total_read;
        size_t to_copy = (bytes_needed < in_buffer) ? bytes_needed : in_buffer;
        //copy from read buf to actual buffer
        memcpy(dest + total_read, bf->read_buffer + bf->read_buffer_pos, to_copy);

        bf->read_buffer_pos += to_copy;
        total_read += to_copy;
        //update offset
        bf->file_offset += to_copy; 
    }

    return (ssize_t)total_read;
}


ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    if (bf == NULL || buf == NULL || bf->fd == -1) {
        perror("buffered_write: invalid buffered_file_t or buffer");
        return -1;
    }
    if (count == 0) return 0;

    //discard any buffered read data if switched from read
    if (bf->last_operation == 1) { // 1 = Read
        bf->read_buffer_pos = 0;
        bf->read_buffer_size = 0;
    }
    bf->last_operation = 2; // 2 = Write

    size_t total_written = 0;
    const char *src = (const char *)buf;
   
    while (total_written < count) { 
        size_t to_copy = count - total_written; //determining how much to copy
        size_t space_left = bf->write_buffer_size - bf->write_buffer_pos;
        if (space_left == 0) {//only flush if buffer full
            if (buffered_flush(bf) == -1) {
                perror("buffered_write: flush error");
                return total_written > 0 ? (ssize_t)total_written : -1;
            }
            space_left = bf->write_buffer_size;
        }

        if (to_copy > space_left) {
            to_copy = space_left;
        }
        //copy data to write buffer
        memcpy(bf->write_buffer + bf->write_buffer_pos, src + total_written, to_copy);
        //update state
        bf->write_buffer_pos += to_copy;
        
        bf->last_operation = 2; //indicate the last operation was write
        total_written += to_copy;
    }

    return (ssize_t)total_written;
}

int buffered_flush(buffered_file_t *bf) {
    // NOTE: This is the simple flush. O_PREAPPEND logic is missing.
    if (bf == NULL || bf->fd == -1) {
        if (bf && bf->write_buffer_pos > 0) {
            perror("buffered_flush: invalid file descriptor or pointer");
        }
        return -1;
    }
    if (bf->write_buffer_pos == 0) {
        return 0; // nothing to flush
    }
    
    // Simple write logic (no O_PREAPPEND handling yet)
    size_t total_written = 0;
    while (total_written < bf->write_buffer_pos) {
        ssize_t written = write(bf->fd, bf->write_buffer + total_written, bf->write_buffer_pos - total_written);
        if (written == -1) {
            perror("buffered_flush: write error"); 
            return -1;
        }
        total_written += written;
    }
    bf->file_offset += total_written;//update file offset
    bf->write_buffer_pos = 0;//clear buffer
    
    return 0;
}

int buffered_close(buffered_file_t *bf) {
    if (bf == NULL) return 0;
    int flush_res = 0;
    int close_res = 0;

    if (bf->write_buffer_pos > 0) { //flush any pending writes
        flush_res = buffered_flush(bf);
    }
    close_res = close(bf->fd);//close fd
    
    // free everything
    free(bf->read_buffer); 
    free(bf->write_buffer);
    free(bf);

    if (flush_res == -1 || close_res == -1) {//return -1 if any error occurred
        perror("buffered_close: error during flush or close");
        return -1;
    }

    return 0;
}