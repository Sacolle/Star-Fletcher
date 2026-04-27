#include "io.h"

#define _GNU_SOURCE
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "macros.h"

struct io_state {
    // uint32_t mem_align;
    // uint32_t mem_offset;
    bool initialized;
    const char* filename;
    size_t page_size;
    size_t file_size;
    file_desc_t fd;
    FP* buff;
};

io_state_t _state = { .initialized = false };

io_state_t* get_io_state(){
    return &_state;
}
void delete_io_state(){
    return;
}

bool is_io_state_initialized(){
    return get_io_state()->initialized;
}

err_t io_state_init(
    const char* filename, const size_t page_size, 
    const size_t max_iteration_count, const size_t volume_size
){
    if(is_io_state_initialized()){
        return ME_REINITILIZATION;
    }


    // 1. open a file
    int fd = 0;
    if((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR)) == -1){
        return errno;
    }

    // 2. posix_fallocate it to the size
    int err = 0;
    const size_t total_file_size = max_iteration_count * volume_size * sizeof(FP);
    if((err = posix_fallocate64(fd, 0, total_file_size)) != 0){
        return err; //is errno
    }
    // pass page size of 4096 to disable 
    int huge_flags = 0;
    if(page_size > 4096){
        huge_flags = MAP_HUGETLB | (__builtin_ctzl(page_size) << MAP_HUGE_SHIFT);
    }

    // 3. mmap using the large page size
    FP* buff = NULL;
    if(
        (buff = (FP*) mmap(NULL, total_file_size, 
            PROT_WRITE | PROT_READ, MAP_SHARED | huge_flags, 
            fd, 0)
        ) == MAP_FAILED
    ){
        return errno;
    }

    *get_io_state() = (io_state_t){
        .initialized = true,
        .buff = buff,
        .fd = fd,
        .filename = filename,
        .page_size = page_size,
        .file_size = total_file_size
    };

    return 0;
}
#include <stdio.h>

extern size_t g_volume_width;

void io_state_write_file(
    const size_t i, const size_t j, const size_t k, const size_t t, 
    const size_t nx, const size_t ny, const size_t nz, const FP* block
){
    io_state_t* state = get_io_state();
    FP* buff = state->buff;
    const size_t total_volume_size = CUBE(g_volume_width);

    for(size_t x = 0; x < nx; x++)
    for(size_t y = 0; y < ny; y++)
    for(size_t z = 0; z < nz; z++){
        // get the absolute index for the point (x, y, z) of cube (i, j, k) in the total volue
        // then offset it by which iteration we are writing to.
        const size_t buff_idx = block_cube_to_volume_idx(x, y, z, i, j, k) + total_volume_size * t;
        //printf("%ld\n", buff_idx);
        buff[buff_idx] = block[cube_idx(x, y, z)];
    }
}

err_t io_state_finish(){
    if(!is_io_state_initialized()){
        return 0;
    }

    io_state_t* state = get_io_state();
    err_t err = 0;

    if((err = munmap(state->buff, state->file_size)) != 0){
        return errno;
    }
    if((err = close(state->fd)) != 0){
        return errno;
    }

    delete_io_state();
    return 0;
}

const char* io_state_get_filename(){
    return get_io_state()->filename;
}

err_t io_available_huge_page_sizes(size_t max_count, size_t* outp_respc, size_t* outp_available_sizes){
    struct dirent* dent;
    DIR* srcdir;
    *outp_respc = 0;

    if ((srcdir = opendir("/sys/kernel/mm/hugepages")) == NULL){
        return errno;
    }

    while((dent = readdir(srcdir)) != NULL){

        const char* folder = dent->d_name;
        const char* prepend = "hugepages-";
        const size_t prepend_size = strlen(prepend);
        if(strlen(folder) < prepend_size){
            continue;
        }
        if(strncmp(folder, prepend, prepend_size) == 0){
            char* out = "\0\0";

            const size_t res = strtoull(folder + prepend_size, &out, 10);
            if(errno){
                return errno;
            }

            if(strncmp(out, "kB", 2) != 0){
                return ME_INCOMPLETE_PARSE;
            }
            if(*outp_respc < max_count){
                outp_available_sizes[*outp_respc]  = res * 1024;
            }
            *outp_respc += 1;
        }
    }
    closedir(srcdir);
    return 0;
}



/*
#define HAS_FLAG(x, f) !(~(x) & f)
err_t io_alignment_restrictions(file_desc_t fd, uint32_t* alignment, uint32_t* offset){
    struct statx stx;
    if(statx(fd, "", AT_EMPTY_PATH, STATX_DIOALIGN, &stx) != 0){
        return errno;
    }
    if(!HAS_FLAG(stx.stx_mask, STATX_DIOALIGN)){
        return ME_MISSING_ATTR;
    }
    *alignment = stx.stx_dio_mem_align;
    *offset = stx.stx_dio_offset_align;

    return 0;
}

int io_open_disk_file(file_desc_t *fd, const char* path){
    // not clear which permissions are necessary
    // NOTE: could use O_TMPFILE for the files of each tread and
    // add the congealing operation into the program. 
    // Possibly use of O_SYNC instead of O_DSYNC 
    if((*fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT | O_DSYNC, S_IWUSR)) == -1){
        return errno;
    }
    return 0;
}
int io_write_file_to_disk(file_desc_t fd, const void* buff, size_t count){
    ssize_t res = write(fd, buff, count);
    if(res != count){
        return -1;
    }
    if(res == -1){
        return errno;
    }
    return 0;
}

int io_close_disk_file(file_desc_t fd){
    if(close(fd) == -1){
        return errno;
    }
    return 0;
}
*/