/*
 * gdt_common.c
 *
 * Copyright (c) 2011 Rickard Edström
 * Copyright (c) 2011 Sebastian Ärleryd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "gdt.h"
#include "sys/time.h"

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

void gdt_log(log_type_t type, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    gdt_logv(type, tag, format, args);
    
    va_end(args);
}

void gdt_fatal(const char* tag, const char* format, ...) {  
    va_list args;
    va_start(args, format);

    gdt_logv(LOG_ERROR, tag, format, args);

    va_end(args);  
    
    gdt_exit(EXIT_FAIL);
}

uint64_t gdt_time_ns(void) {
#if defined(TARGET_OS_IPHONE) || defined(TARGET_IPHONE_SIMULATOR)
    struct timeval now;
	gettimeofday(&now, NULL);
    return (uint64_t) now.tv_sec * 1000000000LL + (uint64_t) now.tv_usec * 1000LL;
#elif ANDROID
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (uint64_t) now.tv_sec * 1000000000LL + (uint64_t) now.tv_nsec;
#else
	return 0;	//Unsupported platform
#endif
    
}
