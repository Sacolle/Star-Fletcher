#ifndef _MEDIUM_GUARD_
#define _MEDIUM_GUARD_

#include <stddef.h>

#include "floatingpoint.h"

enum Form { ISO, VTI, TTI };

int str_to_medium(const char* str);

//int medium_initialize();

#endif