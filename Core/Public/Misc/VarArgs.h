#pragma once

#include "Core/Public/Misc/Types.h"
#include <stdarg.h>

#define CORE_GET_VARARGS_RESULT(msg, msgsize, lastarg, fmt, formatter, result) \
	do { \
		va_list ap; \
		va_start(ap, lastarg); \
		result = formatter(msg, msgsize, fmt, ap); \
		if (result >= msgsize) \
		{ \
			result = -1; \
		} \
		va_end(ap); \
	} while (0)
