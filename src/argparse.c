#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>

#include "argparse.h"

bool has_envvar(const char* key){
    return getenv(key) != NULL;
}

err_t i64_get_envvar(int64_t* out, const char* key){
    char* env_out = getenv(key);
    if(env_out == NULL){
      return ME_MISSING_ENVVAR;
    }
    char *rest;
    errno = 0;

    int64_t ret = strtoll(env_out, &rest, 10);
    int err = errno;
    if(err) {
      return err;
    }
    if(rest == key){
      // String vazia de entrada
      return ME_IMPROPER_INPUT;
    }
    if(*rest != '\0'){ 
      // há mais caracteres que não foram parseados
      return ME_INCOMPLETE_PARSE;
    };
    *out = ret;
    return 0;
}

err_t str_get_envvar(char** out, const char* key){
    char* env_out = getenv(key);
    if(env_out == NULL){
      return ME_MISSING_ENVVAR;
    }
    *out = env_out;
    return 0;
}


err_t str_to_enum(const char* word, int* outp_res, int count, ...){
	va_list valist;
	va_start(valist, count);
	*outp_res = 0;
	err_t err = ME_NOMATCH;
	for(int i = 0; i < count; i++){
		char* str = va_arg(valist, char*);
		int enum_value = va_arg(valist, int);

		if(strcmp(word, str) == 0){
			*outp_res = enum_value;
			err = 0;
			break;
		}
	}
	va_end(valist);
	return err;
}

bool is_neg(char* str){
	return *str == '-';
}

err_t read_args(int* argc, char** argv, int count, ...){
	va_list valist;
	va_start(valist, count);
	err_t err = 0;

	if(*argc < count){
		err = ME_IMPROPER_INPUT;
		goto exit;
	}
	//skipa o primeiro elemento
	argv++;

	// store in argc the element, read, in case of error, 
	// it is the index of the failed element
	*argc = 0;

	for(size_t i = 0; i < count; i++){
		int tag = va_arg(valist, int);
		
		if(tag < 0 || tag > 8) {
			err = ME_NOMATCH;
			goto exit;
		}

		void* val = va_arg(valist, void*);
		char* rest = NULL;
		// set error number to zero before converting
		errno = 0;

		// skip value if tag is zero
		if(tag == 0) continue;

		#define READ_TO_VAL(type, func) \
			*(type*) val = func; \
			if(errno) { err = errno; goto exit; } \
			else if(*rest) { err = ME_INCOMPLETE_PARSE; goto exit; };

		switch(tag){
			case ARG_i32: 
				READ_TO_VAL(int32_t, strtol(argv[i], &rest, 10)); 
				break;
			case ARG_i64: 
				READ_TO_VAL(int64_t, strtoll(argv[i], &rest, 10)); 
				break;
			case ARG_u32: 
				if(is_neg(argv[i])) { err = ME_OVERFLOW_CONVERT; goto exit; }
				READ_TO_VAL(uint32_t, strtoul(argv[i], &rest, 10)); 
				break;
			case ARG_u64: 
				if(is_neg(argv[i])) { err = ME_OVERFLOW_CONVERT; goto exit; }
				READ_TO_VAL(uint64_t, strtoull(argv[i], &rest, 10)); 
				break;
			case ARG_f32: 
				READ_TO_VAL(float, strtof(argv[i], &rest));
				break;
			case ARG_f64: 
				READ_TO_VAL(double, strtod(argv[i], &rest));
				break;
			case ARG_str: 
				*(char**) val = argv[i];
				break;
			case ARG_usize:
				if(is_neg(argv[i])) { err = ME_OVERFLOW_CONVERT; goto exit; }
				//size_max is the max val of type size_t, 
				// if its the same as u64, use strtoull, else use srtoul
				#if SIZE_MAX == UINT64_MAX
				READ_TO_VAL(size_t, strtoull(argv[i], &rest, 10)); 
				#else 
				READ_TO_VAL(size_t, strtoul(argv[i], &rest, 10)); 
				#endif
				break;
			default:
				assert(0 && "Unreachable!");

#undef READ_TO_VAL
		}
		*argc += 1;
	}
	exit:
	va_end(valist);
	return err;
}
