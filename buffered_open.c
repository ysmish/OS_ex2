#define _GNU_SOURCE  //for O_TMPFILE not sure if it's really needed
#include "buffered_open.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h> 
#include <stdio.h> 
#include <fcntl.h>
#include <unistd.h>     
#include <sys/types.h>
#include <sys/stat.h>

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
        perror("buffered_open: struct memory allocation error");
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
    bf->read_buffer_size = 0;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;
    bf->preappend = (flags & O_PREAPPEND) ? 1 : 0; 
    
    //remove O_PREAPPEND from flags passed to open, if we are pre-appending
    bf->flags = flags & ~O_PREAPPEND; 
    
    bf->last_operation = 0;
    bf->file_offset = 0; 

    // 5.open file 
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
    
    //flush the write buffer when switching from write
    if (bf->last_operation == 2) { // 2 = Write
        if (buffered_flush(bf) == -1) {
            perror("buffered_read: failed to flush write buffer before reading");
            return -1;
        }
    }
    bf->last_operation = 1; // 1 = Read

    size_t total_read = 0;
    char *dest = (char *)buf;

    while (total_read < count) {
        size_t in_buffer = bf->read_buffer_size - bf->read_buffer_pos;
        
        //refill buffer if empty
        if (in_buffer == 0) {
            ssize_t bytes_read = read(bf->fd, bf->read_buffer, BUFFER_SIZE);

            if (bytes_read == 0) {
                //end of file
                return total_read;
            }
            if (bytes_read < 0) {
                perror("buffered_read: underlying read error");
                return total_read > 0 ? (ssize_t)total_read : -1;
            }
            bf->read_buffer_size = bytes_read;
            bf->read_buffer_pos = 0;
            in_buffer = bytes_read; 
        }
        size_t bytes_needed = count - total_read;
        size_t to_copy = (bytes_needed < in_buffer) ? bytes_needed : in_buffer;     
        memcpy(dest + total_read, bf->read_buffer + bf->read_buffer_pos, to_copy);
        bf->read_buffer_pos += to_copy;
        total_read += to_copy;
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

    //discard any buffered read if switched from read
    if (bf->last_operation == 1) { // 1 = Read
        //align file cursor using lseek
        if (lseek(bf->fd, bf->file_offset, SEEK_SET) == (off_t)-1) {
             perror("buffered_write: lseek error");
             return -1;
        }
        bf->read_buffer_pos = 0;
        bf->read_buffer_size = 0;
    }
    bf->last_operation = 2; // 2 = Write

    size_t total_written = 0;
    const char *src = (const char *)buf;
   
    while (total_written < count) { 
        size_t to_copy = count - total_written;
        size_t space_left = bf->write_buffer_size - bf->write_buffer_pos;
        
        if (space_left == 0) {
            //flush buffer if full
            if (buffered_flush(bf) == -1) {
                perror("buffered_write: flush error");
                return total_written > 0 ? (ssize_t)total_written : -1;
            }
            space_left = bf->write_buffer_size;
        }

        if (to_copy > space_left) {
            to_copy = space_left;
        }
        
        memcpy(bf->write_buffer + bf->write_buffer_pos, src + total_written, to_copy);
        bf->write_buffer_pos += to_copy;
        total_written += to_copy;
    }

    return (ssize_t)total_written;
}

int buffered_flush(buffered_file_t *bf) {
    if (bf == NULL || bf->fd == -1) {
        if (bf->write_buffer_pos > 0) {
            perror("buffered_flush: invalid file descriptor or pointer");
        }
        return -1;
    }
    if (bf->write_buffer_pos == 0) {
        return 0;
    }

    size_t total_written = 0;

    if (bf->preappend) {// --- O_PREAPPEND LOGIC ---
        off_t file_size = lseek(bf->fd, 0, SEEK_END);//get file size
        if (file_size == -1) {
            perror("buffered_flush: lseek end error");
            return -1;
        }
        char *temp_buf = NULL;//alloc temp buf to hold content
        if (file_size > 0) {
            temp_buf = malloc(file_size);
            if (!temp_buf) {
                errno = ENOMEM;
                perror("buffered_flush: memory allocation for preappend");
                return -1;
            }
            if (lseek(bf->fd, 0, SEEK_SET) == -1) {//read content into temp buf
                perror("buffered_flush: lseek set error");
                free(temp_buf);
                return -1;
            }
            ssize_t r = 0;
            size_t total_read_temp = 0;
            while (total_read_temp < file_size) {
                r = read(bf->fd, temp_buf + total_read_temp, file_size - total_read_temp);
                if (r <= 0) {
                    perror("buffered_flush: error reading existing content");
                    free(temp_buf);
                    return -1;
                }
                total_read_temp += r;
            }
        }
        if (lseek(bf->fd, 0, SEEK_SET) == -1) {//move to start to write new data
            perror("buffered_flush: lseek rewind error");
            free(temp_buf);
            return -1;
        }
        while (total_written < bf->write_buffer_pos) {//write buf content
            ssize_t written = write(bf->fd, bf->write_buffer + total_written, bf->write_buffer_pos - total_written);
            if (written == -1) {
                if (errno == EINTR) continue;
                perror("buffered_flush: write error (prepend)");
                free(temp_buf);
                return -1;
            }
            total_written += written;
        }
        if (file_size > 0 && temp_buf) {//append old content back
            size_t written_old = 0;
            while (written_old < file_size) {
                ssize_t w = write(bf->fd, temp_buf + written_old, file_size - written_old);
                if (w == -1) {
                    if (errno == EINTR) continue;
                    perror("buffered_flush: write error (restoring old data)");
                    free(temp_buf);
                    return -1;
                }
                written_old += w;
            }
            free(temp_buf);
        }
        if (lseek(bf->fd, bf->file_offset + total_written, SEEK_SET) == -1) {//restore fd to correct logical position
             perror("buffered_flush: lseek restore error");
             return -1;
        }
    } 
    else {
        while (total_written < bf->write_buffer_pos) {
            ssize_t written = write(bf->fd, bf->write_buffer + total_written, bf->write_buffer_pos - total_written);
            if (written == -1) {
                if (errno == EINTR) continue; 
                perror("buffered_flush: write error"); 
                return -1;
            }
            total_written += written;
        }
    }
    bf->file_offset += total_written;
    bf->write_buffer_pos = 0;//clear buffer
    
    return 0;
}

int buffered_close(buffered_file_t *bf) {
    if (bf == NULL) return 0;
    int flush_res = 0;
    int close_res = 0;

    //flush pending writes
    if (bf->write_buffer_pos > 0) { 
        flush_res = buffered_flush(bf);
    }
    close_res = close(bf->fd);
    if (close_res == -1) {
        perror("buffered_close: file close error");
    }
    
    free(bf->read_buffer); 
    free(bf->write_buffer);
    free(bf);

    if (flush_res == -1 || close_res == -1) {
        return -1;
    }
    return 0;
}