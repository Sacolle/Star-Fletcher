#ifndef _ARGPARSE_GUARD
#define _ARGPARSE_GUARD

#include <stdbool.h>
#include <stdint.h>

#include "err.h"

#define ARG_i32 1
#define ARG_i64 2
#define ARG_u32 3
#define ARG_u64 4
#define ARG_f32 5
#define ARG_f64 6
#define ARG_str 7
#define ARG_usize 8


bool has_envvar(const char* key);

// get the environment variable key, and returns the value as a int64_t
err_t i64_get_envvar(int64_t* out, const char* key);

// get the environment variable key, and returns the value as a string
// does not modify out on err
err_t str_get_envvar(char** out, const char* key);


// Get the environment variable from key and put on output
// currently only implemented for string and int64_t
// returns 0 on sucess
// does not modify `output` on error
#define get_envvar(output, key) _Generic((output), \
    int64_t* : i64_get_envvar, \
    char** : str_get_envvar, \
    default: str_get_envvar \
)(output, key)

// 
// In the arglist made by pairs `char*, int`, returns the `int` whose 
// precious string matched with word
// returns 0 on sucess and `ME_NOMATCH` if no matches where made
err_t str_to_enum(const char* word, int* outp_res, int count, ...);

// Given a sequence of pairs `ARG_type, type*` of length `count`,
// read the `i`th element from `argv`, parse for type `ARG_type` 
// and set the value of `type*` to be the parsed value
// returns 0 on sucess
// on error, argc will hold the index of the element that failed to parse
err_t read_args(int* argc, char** argv, int count, ...);

#endif