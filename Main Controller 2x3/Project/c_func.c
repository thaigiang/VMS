#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "stm32f10x.h"
#include "c_func.h"

#define RAM_BASE_ADDR	0x20000000

//============================================
int CaptureLine(void* pBuffer, char const* pFormatString, ...)
{
	char*	pStringEnd = (char*)pBuffer + strlen((char*)pBuffer);
	va_list valist;
	
	va_start(valist, pFormatString);
	
	return vsprintf(pStringEnd, pFormatString, valist);
}

//============================================
u32 checkHeapSize(void)
{
	u16 i = 0;
	char OutputBuffer[256];
	u32 freeHeapSize = 0;
	
	OutputBuffer[0] = '\0';
	__heapstats(CaptureLine, OutputBuffer);

	while(OutputBuffer[i] != ' ')
	{
		freeHeapSize = (freeHeapSize*10) + (OutputBuffer[i] - 0x30);
		i++;
	}
	return freeHeapSize;
}

//============================================
void* m_malloc (u32 size)
{
	u32 freeHeapSize = 0;

	freeHeapSize = checkHeapSize();
	if(freeHeapSize>=size)
		return malloc(size);
	else
		return NULL;
}

//============================================
void* m_calloc (u32 num, u32 size)
{
	u32 freeHeapSize = 0;

	freeHeapSize = checkHeapSize();
	if(freeHeapSize>=(num*size))
		return calloc(num,size);
	else
		return NULL;
}

//============================================
void* m_realloc (void* ptr, u32 size)
{
	u32 freeHeapSize = 0;

	freeHeapSize = checkHeapSize();
	if(freeHeapSize>=size)
		return realloc(ptr,size);
	else
		return NULL;
}

//============================================
void m_free (void* ptr)
{
	if(ptr>=(void*)RAM_BASE_ADDR)
	{
		free(ptr);
		ptr = NULL;
	}
}

