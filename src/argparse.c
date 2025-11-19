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



int str_to_enum(const char* word, int* err, int count, ...){
	va_list valist;
	va_start(valist, count);
	int ret = 0;
	*err = 1;
	for(int i = 0; i < count; i++){
		char* str = va_arg(valist, char*);
		int enum_value = va_arg(valist, int);

		if(strcmp(word, str) == 0){
			ret = enum_value;
			*err = 0;
			break;
		}
	}
	va_end(valist);
	return ret;
}

//mask is 3 bits
#define MASKSIZE 3
#define MASK 7 // 111
enum ParseErrors {
	NoError = 0,
	InvalidTag = 1,
	ImproperConversion = 2,
	OverflowConversion = 3,
	InsuficientArgs = 4
};

bool is_neg(char* str){
	return *str == '-';
}
int read_args(int argc, char** argv, int count, ...){
	va_list valist;
	va_start(valist, count);
	int err = NoError;
	int i = 0;
	if(argc < count){
		err = InsuficientArgs;
		goto exit;
	}
	//skipa o primeiro elemento
	argv++;

	for(i = 0; i < count; i++){
		int tag = va_arg(valist, int);
		
		if(tag < 0 || tag > 7) {
			err = InvalidTag;
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
			if(errno) { err = OverflowConversion; goto exit; } \
			else if(*rest) { err = ImproperConversion; goto exit; };

		switch(tag){
			case ARG_i32: 
				READ_TO_VAL(int32_t, strtol(argv[i], &rest, 10)); 
				break;
			case ARG_i64: 
				READ_TO_VAL(int64_t, strtoll(argv[i], &rest, 10)); 
				break;
			case ARG_u32: 
				if(is_neg(argv[i])) { err = OverflowConversion; goto exit; }
				READ_TO_VAL(uint32_t, strtoul(argv[i], &rest, 10)); 
				break;
			case ARG_u64: 
				if(is_neg(argv[i])) { err = OverflowConversion; goto exit; }
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
				if(is_neg(argv[i])) { err = OverflowConversion; goto exit; }
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
		}
	}
	exit:
	va_end(valist);
	if(err == 0){
		return 0;
	}else{
		return (i << MASKSIZE) | err;
	}
}

int get_parse_errors_local(int err){
	return err >> MASKSIZE;
}

char* get_parse_errors_name(int err){
	#define ERR(e) e: return #e;  
	switch (err & MASK) {
		case ERR(NoError);
		case ERR(InvalidTag);
		case ERR(ImproperConversion);
		case ERR(OverflowConversion);
		case ERR(InsuficientArgs);
	default:
		return "<Invalid Error Code>";
	}
}