
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef MEM_HOOK_MT
#include <pthread.h>
#endif

#define REC_LIST_NUM (4)
#define REC_NUM_PER_LIST (40*1024)
#ifndef FILENAME_MAX
#define FILENAME_MAX (32)
#endif
#define FUNCTION_MAX (32)
#define DES_LIST_NUM (64)


struct mem_des{
	char filename[FILENAME_MAX];
	char function[FUNCTION_MAX];
	int lineno;
	struct mem_des *next;
};

struct mem_rec{
	int used :1;
	int index :31;
	unsigned int size;
	struct mem_des* des;
	struct mem_rec* next;
};


/*! 
 * Index of mem_rec in each list:
 * mem_rec_list[0] {0, 1, ..., REC_NUM_PER_LIST-1}
 * mem_rec_list[1] {REC_NUM_PER_LIST, REC_NUM_PER_LIST+1, ..., REC_NUM_PER_LIST*2 -1}
 * mem_rec_list[2] {REC_NUM_PER_LIST*2, REC_NUM_PER_LIST*2+1, ..., REC_NUM_PER_LIST*3 -1}
 * ...
 */
static struct mem_rec * mem_rec_list [REC_LIST_NUM] = {NULL};

static struct mem_rec *free_list = NULL;

/* Description records  hashed by lineno. */
static struct mem_des * mem_des_list [DES_LIST_NUM] = {NULL};

#ifdef MEM_HOOK_MT
pthread_mutex_t mutex;
#define MEM_HOOK_LOCK() pthread_mutex_lock(&mutex)
#define MEM_HOOK_UNLOCK() pthread_mutex_unlock(&mutex)
#else
#define MEM_HOOK_LOCK()
#define MEM_HOOK_UNLOCK()
#endif


static struct mem_des * get_mem_des(const char* filename, 
		const char* function, int lineno)
{
	struct mem_des *des;
	des = mem_des_list[lineno % DES_LIST_NUM];
	while (des) {
		if (0 == strcmp(filename, des->filename)){
			return des;
		}
		des = des->next;
	}
	/* If there is not an exist record for the code line, then 
	  * create one and append it to the list. 
	  */
	des = malloc(sizeof(struct mem_des));
	des->next = mem_des_list[lineno % DES_LIST_NUM];
	strncpy(des->filename, filename, FILENAME_MAX);
	strncpy(des->function, function, FUNCTION_MAX);
	des->lineno = lineno;
	mem_des_list[lineno % DES_LIST_NUM] = des;
	return des;
}


static int init_mem_rec_list(int idx)
{
	int i;
	struct mem_rec* list;
	
	list = malloc(REC_NUM_PER_LIST * sizeof(struct mem_rec));
	if (NULL == list){
		return -1;
	}
	mem_rec_list[idx] = list;
	for(i = 0; i < REC_NUM_PER_LIST-1; i++){
		list[i].used = 0;
		list[i].index = i + REC_NUM_PER_LIST*idx;
		list[i].next = &list[i+1];
	}
	list[i].used = 0;
	list[i].index = i + REC_NUM_PER_LIST*idx;
	list[i].next = free_list;
	free_list = list;
	return 0;
}

int memory_hook_init()
{
	int i;
	for (i = 0; i < REC_LIST_NUM; i++)
		mem_rec_list[i] = NULL;

	for (i = 0; i < DES_LIST_NUM; i++)
		mem_des_list[i] = NULL;
	free_list = NULL;
	/*Init one memory record list at the first time*/
	init_mem_rec_list(0);
#ifdef MEM_HOOK_MT
	pthread_mutex_init(&mutex, NULL);
#endif
	return 0;
}

static struct mem_rec* pop_mem_rec()
{
	struct mem_rec* ret;
	if(NULL == free_list){
		int i;
		for (i = 0; i < REC_LIST_NUM; i++){
			if (mem_rec_list[i] == NULL){
				if (init_mem_rec_list(i))
					return NULL;
			}
		}
	}
	ret = free_list;
	free_list = free_list->next;
	return ret;
}

static inline struct mem_rec* get_mem_rec_by_index(int index)
{
	return &mem_rec_list[index/REC_NUM_PER_LIST][index % REC_NUM_PER_LIST];
}

static void push_mem_rec(struct mem_rec *rec)
{
	rec->used = 0;
	rec->next = free_list;
	free_list = rec;
}
/*  memory structure:
  * |sizeof(int) bytes|...
  *          ^              ^
  * | record index    |returned address  
  */
void* malloc_hook(size_t size, const char *filename,
		const char *function, int lineno)
{
	void *addr;
	struct mem_rec *rec;
        addr = malloc(size + sizeof(int) * 1);
	if (NULL == addr)
		return NULL;
	MEM_HOOK_LOCK();
	rec = pop_mem_rec();
	if (NULL == rec){
		MEM_HOOK_UNLOCK();
		return addr;
	}
	rec->size = size;
	rec->used = 1;
	*((int*)addr) = rec->index;
	//*((unsigned int*)addr+1) = get_mem_des(filename, function, lineno);
	rec->des = get_mem_des(filename, function, lineno);
	MEM_HOOK_UNLOCK();
	return ((char*)addr + sizeof(int) * 1);
}


void free_hook(void *ptr)
{
	struct mem_rec *rec;
	if (NULL == ptr)
		return;
	MEM_HOOK_LOCK();
	rec = get_mem_rec_by_index(*((int *)ptr - 1));
	push_mem_rec(rec);
	MEM_HOOK_UNLOCK();
	free((int *)ptr - 1);
}

char *strdup_hook(const char *str, const char *filename,
		const char *function, int lineno)
{
	int len;
	char *ret;
	len = strlen(str);
	ret = malloc_hook(len+1, filename, function, lineno);
	if (NULL != ret)
		strcpy(ret, str);
	return ret;
}

void * realloc_hook(void *ptr, size_t size, const char *filename,
		const char *function, int lineno)
{
	struct mem_rec *rec;
	void *ret;
	if (NULL == ptr){
		ret = malloc_hook(size, filename, function, lineno);
	}else{
		MEM_HOOK_LOCK();
		rec = get_mem_rec_by_index(*((int *)ptr - 1));
		rec->size = size;
		rec->des = get_mem_des(filename, function, lineno);
		MEM_HOOK_UNLOCK();
		ret = realloc((int *)ptr - 1, size + sizeof(int)*1);
        ret = ((int *)ret) + 1;
	}
	return ret;
}

void * calloc_hook(size_t nmemb, size_t size, const char *filename,
		const char *function, int lineno)
{
	void *ret;
	ret = malloc_hook(nmemb*size, filename, function, lineno);
	memset(ret, 0, nmemb*size);
	return ret;
}

void dump_memory(FILE* fp)
{
	int i, j;
	unsigned int total = 0;
	fprintf("Unfreed memory records :\n");
	for(i = 0; i < REC_LIST_NUM; i++){
		if(NULL != mem_rec_list[i]){
			for(j = 0; j < REC_NUM_PER_LIST; j++){
				if(mem_rec_list[i][j].used){
					fprintf(fp, "%s, %s, %d: %d bytes\n",
							mem_rec_list[i][j].des->filename, 
							mem_rec_list[i][j].des->function,
						       	mem_rec_list[i][j].des->lineno,
							mem_rec_list[i][j].size);
					total += mem_rec_list[i][j].size;
				}
			}
		}
	}
	fprintf(fp, "total : %d bytes\n", total);
}


