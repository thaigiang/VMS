#ifndef __C_FUNC__
#define __C_FUNC__

void* m_malloc (u32 size);
void* m_calloc (u32 num, u32 size);
void* m_realloc (void* ptr, u32 size);
void m_free (void* ptr);

#endif
