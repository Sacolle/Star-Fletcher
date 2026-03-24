#ifndef _ERR_GUARD_
#define _ERR_GUARD_

#include <stdint.h>
#include <errno.h>
#include <string.h>

typedef int64_t err_t;

#define MY_ERR(x) (((int64_t) x ) << 32)

#define IS_STARPU_ERR(x) ((x) < 0)
#define IS_SYS_ERR(x) (((x) & 0x0000FFFF) != 0)
#define IS_MY_ERR(x) (((x) & 0xFFFF0000) != 0)


// ME stands for my err
#define ME_INCOMPLETE_PARSE MY_ERR(1)
#define ME_MISSING_ATTR     MY_ERR(2)
#define ME_MISSING_ENVVAR   MY_ERR(3)
#define ME_IMPROPER_INPUT   MY_ERR(4)
#define ME_NOMATCH          MY_ERR(5)
#define ME_OVERFLOW_CONVERT MY_ERR(6)
#define ME_COUNT_DONT_MATCH MY_ERR(7)
#define ME_REINITILIZATION  MY_ERR(8)


static char* err_name(err_t err){
    if(IS_STARPU_ERR(err)){
        return strerror((int) -err);
    }
    if(IS_SYS_ERR(err)){
        return strerror((int) err);
    }
    #define NAMECASE(def) case def: return # def
    switch (err){
        NAMECASE(ME_INCOMPLETE_PARSE);
        NAMECASE(ME_MISSING_ATTR);
        NAMECASE(ME_MISSING_ENVVAR);
        NAMECASE(ME_IMPROPER_INPUT);
        NAMECASE(ME_NOMATCH);
        NAMECASE(ME_OVERFLOW_CONVERT);
        NAMECASE(ME_COUNT_DONT_MATCH);
        NAMECASE(ME_REINITILIZATION);
        default: return "ERR not found";
    }
    #undef NAMECASE
}

#endif