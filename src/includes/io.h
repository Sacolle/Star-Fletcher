#ifndef _IO_GUARD_
#define _IO_GUARD_

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>

typedef int FileDesc;

// get the alignment and offset requirements of the system
// returns 0 on sucess
int io_alignment_restrictions(FileDesc fd, uint32_t* alignment, uint32_t* offset);

// opens the file with the following options
// O_WRONLY : write the file
// O_CREAT  : create if not exists 
// O_TRUNC  : truncate if exists
// O_DIRECT : write direct to disk without using memory as a buffer
//            this one requires that writes be memory alligned
// O_DSYNC  : always assure the write operation finishes before continuing.
// more in https://man7.org/linux/man-pages/man2/open.2.html
// returns 0 on sucess
int io_open_disk_file(FileDesc *fd, const char* path);

// writes the data to disk
// if the buffer used to write is not aligned the function will error
// returns 0 on sucess
int io_write_file_to_disk(FileDesc fd, const void* buff, size_t count);

// closes the file
// returns 0 on sucess
int io_close_disk_file(FileDesc fd);


#endif
