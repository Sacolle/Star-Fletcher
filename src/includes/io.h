#ifndef _IO_GUARD_
#define _IO_GUARD_

#define _GNU_SOURCE
#include <stddef.h>
#include <stdint.h>

#include "err.h"
#include "floatingpoint.h"

typedef int file_desc_t;

typedef struct io_state io_state_t;

err_t io_state_init(
    const char* filename, const size_t page_size, 
    const size_t max_iteration_count, const size_t volume_size
);

void io_state_write_file(
    const size_t i, const size_t j, const size_t k, const size_t t, 
    const size_t nx, const size_t ny, const size_t nz, const FP* block
);

err_t io_state_finish();

const char* io_state_get_filename();

err_t io_available_huge_page_sizes(size_t max_count, size_t* outp_respc, size_t* outp_available_sizes);






/*
// opens the file with the following options
// O_WRONLY : write the file
// O_CREAT  : create if not exists 
// O_TRUNC  : truncate if exists
// O_DIRECT : write direct to disk without using memory as a buffer
//            this one requires that writes be memory alligned
// O_DSYNC  : always assure the write operation finishes before continuing.
// more in https://man7.org/linux/man-pages/man2/open.2.html
// returns 0 on sucess
int io_open_disk_file(file_desc_t *fd, const char* path);

// writes the data to disk
// if the buffer used to write is not aligned the function will error
// returns 0 on sucess
int io_write_file_to_disk(file_desc_t fd, const void* buff, size_t count);

// closes the file
// returns 0 on sucess
int io_close_disk_file(file_desc_t fd);

// get the alignment and offset requirements of the system
// returns 0 on sucess
int io_alignment_restrictions(file_desc_t fd, uint32_t* alignment, uint32_t* offset);
*/

#endif
