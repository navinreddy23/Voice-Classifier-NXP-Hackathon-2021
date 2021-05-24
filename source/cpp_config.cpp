//*****************************************************************************
//   +--+       
//   | ++----+   
//   +-++    |  
//     |     |  
//   +-+--+  |   
//   | +--+--+  
//   +----+    Copyright (c) 2009-2012 Code Red Technologies Ltd.
//   +----+    Copyright 2013, 2019 NXP
//
// Minimal implementations of the new/delete operators and the verbose 
// terminate handler for exceptions suitable for embedded use,
// plus optional "null" stubs for malloc/free (only used if symbol
// CPP_NO_HEAP is defined).
//
//
// Version : 120126
//
// Software License Agreement
// 
// The software is owned by Code Red Technologies and/or its suppliers, and is 
// protected under applicable copyright laws.  All rights are reserved.  Any 
// use in violation of the foregoing restrictions may subject the user to criminal 
// sanctions under applicable laws, as well as to civil liability for the breach
// of the terms and conditions of this license.
// 
// THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
// OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
// USE OF THIS SOFTWARE FOR COMMERCIAL DEVELOPMENT AND/OR EDUCATION IS SUBJECT
// TO A CURRENT END USER LICENSE AGREEMENT (COMMERCIAL OR EDUCATIONAL) WITH
// CODE RED TECHNOLOGIES LTD. 
//
//*****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include "ei_classifier_porting.h"
#include "fsl_debug_console.h"

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void *p)
{
    free(p);
}

void operator delete[](void *p)
{
    free(p);
}

void DebugLog(const char* s)
{

}

void *ei_malloc(size_t size)
{
	return malloc(size);
}

void *ei_calloc(size_t nitems, size_t size)
{
	return calloc(nitems, size);
}

void ei_free(void *ptr)
{
	free(ptr);
}

uint64_t ei_read_timer_us()
{
	return 0;
}

uint64_t ei_read_timer_ms()
{
	return 0;
}

EI_IMPULSE_ERROR ei_run_impulse_check_canceled()
{
	return EI_IMPULSE_OK;
}

void ei_printf(const char *format, ...)
{
	static char buffer[400];
	// Write the string to the buffer
	int len = 0;
	va_list args;
	va_start(args, format);
	len = vsprintf(buffer, format, args);
	buffer[len] = '\r';
	buffer[len+1] = '\n';
	buffer[len+2] = '\0';
	va_end(args);

	PRINTF(buffer);
}

void ei_printf_float(float f)
{
	PRINTF("%f", f);
}

extern "C" int __aeabi_atexit(void *object,
		void (*destructor)(void *),
		void *dso_handle)
{
	return 0;
}

#ifdef CPP_NO_HEAP
extern "C" void *malloc(size_t) {
	return (void *)0;
}

extern "C" void free(void *) {
}
#endif

#ifndef CPP_USE_CPPLIBRARY_TERMINATE_HANDLER
/******************************************************************
 * __verbose_terminate_handler()
 *
 * This is the function that is called when an uncaught C++
 * exception is encountered. The default version within the C++
 * library prints the name of the uncaught exception, but to do so
 * it must demangle its name - which causes a large amount of code
 * to be pulled in. The below minimal implementation can reduce
 * code size noticeably. Note that this function should not return.
 ******************************************************************/
namespace __gnu_cxx {
void __verbose_terminate_handler()
{
  while(1);
}
}
#endif
