/*!
 * \file mem_hook.h
 * \brief 
 * \author chandlerw@caton.com.cn
 * \date 2011.08.21
 */

#ifndef _MEM_HOOK_H
#define _MEM_HOOK_H

#define MEM_ALIGN 4  /*memory align in bytes*/


#if defined(MEM_HOOK) || defined(MEM_HOOK_MT)
/*!
 * \brief Initialize memory hook liberary.
 *
 * \return 0 if succeed, -1 if failed.
 */
int memory_hook_init();

/*!
 * \brief 
 *
 * \param size Requested size in bytes.
 * \param file File name of which invokes this function.
 * \param function Function name of which invokes this function.
 * \param line Line number in the file that invokes this function.
 * \return Returns the memory pointer if allocation succeed, or NULL if failed.
 */

void *malloc_hook(unsigned int size, const char *file, const char *function, int line);

#ifdef malloc
#undef malloc
#endif
#define malloc(size) malloc_hook(size,__FILE__, __FUNCTION__, __LINE__)

#define strdup(str) strdup_hook(str, __FILE__, __FUNCTION__, __LINE__)
#define calloc(nmemb, size) calloc_hook(nmemb, size, __FILE__, __FUNCTION__, __LINE__)
#define realloc(ptr, size) realloc_hook(ptr, size, __FILE__, __FUNCTION__, __LINE__)

#define free(ptr) free_hook(ptr)

#else

#endif

#endif

