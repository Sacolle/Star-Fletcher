#include "io.h"

#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define HAS_FLAG(x, f) !(~(x) & f)

int io_alignment_restrictions(FileDesc fd, uint32_t* alignment, uint32_t* offset){
    struct statx stx;
    if(statx(fd, "", AT_EMPTY_PATH, STATX_DIOALIGN, &stx) != 0){
        return errno;
    }
    if(!HAS_FLAG(stx.stx_mask, STATX_DIOALIGN)){
        return -1;
    }
    *alignment = stx.stx_dio_mem_align;
    *offset = stx.stx_dio_offset_align;

    return 0;
}

int io_open_disk_file(FileDesc *fd, const char* path){
    // not clear which permissions are necessary
    // NOTE: could use O_TMPFILE for the files of each tread and
    // add the congealing operation into the program. 
    // Possibly use of O_SYNC instead of O_DSYNC 
    if((*fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_DSYNC, S_IWUSR)) == -1){
        return errno;
    }
    return 0;
}

int io_write_file_to_disk(FileDesc fd, const void* buff, size_t count){
    ssize_t res = write(fd, buff, count);
    if(res != count){
        return -1;
    }
    if(res == -1){
        return errno;
    }
    return 0;
}

int io_close_disk_file(FileDesc fd){
    if(close(fd) == -1){
        return errno;
    }
    return 0;
}