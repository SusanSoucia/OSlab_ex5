/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <syscall.h>
#include <stdio.h>

struct chunk {
    char signature[4];  /* "OSEX" */
    struct chunk *next; /* ptr. to next chunk */
    int state;          /* 0 - free, 1 - used */
#define FREE   0
#define USED   1

    int size;           /* size of this chunk */
};

static int heap_lock = 0;
static struct chunk *chunk_head;
static int head_size = sizeof(struct chunk);  //单独计算出chunk_head的大小，便于后面计算

void *g_heap;
void *tlsf_create_with_pool(uint8_t *heap_base, size_t heap_size)
{
    chunk_head = (struct chunk *)heap_base;
    strncpy(chunk_head->signature, "OSEX", 4);
    chunk_head->next = NULL;
    chunk_head->state = FREE;
    chunk_head->size  = heap_size;

    heap_lock = sem_create(1);
    return NULL;
}

void *malloc(size_t size)
{
  
  if (size==0)  //输入大小为0时,不做分配
      return NULL;

    sem_wait(heap_lock); //加锁
  int find =0;
  struct chunk * start = chunk_head;
  while(start!=NULL){
    if (start->state == FREE){
      //printf ("chunk size = %d\n",start->size);
      if (start->size >= (size + 2*head_size)){
        struct chunk *nxt = (struct chunk *)((uint8_t *)start + head_size + size );
        memcpy(nxt, start, head_size); //将下一个块的指针复制到当前块
        nxt->size = start->size - size - head_size;
        start->next = nxt;
        start->state = USED;    //先标记为已使用
        start->size = size + head_size;  //分配大小
        find =1;
        break;
      }
      else if (start->size == (size + head_size)){
        start->state = USED;    //先标记为已使用
        find = 1;
        break;
    }
  }
  start = start->next;  
} 
  sem_signal(heap_lock); //解锁
  if (find==1)
    {return (void*) (start+1);} //在C语言中，指针加1表示加上sizeof(struct chunk)的大小
  else
    {return NULL;} //没有找到合适的内存块
  
}

void free(void *ptr)
{
  if (ptr==NULL)
    return; //输入指针为空时,不做释放

  sem_wait(heap_lock);

  struct chunk *achunk=(struct chunk *)(((uint8_t *)ptr)-sizeof(struct chunk));

  if (achunk->signature[0] == 'O' &&
    achunk->signature[1] == 'S' &&
    achunk->signature[2] == 'E' &&
    achunk->signature[3] == 'X')

    {
    achunk->state = FREE; //将当前块标记为未使用
    achunk = chunk_head;
  while (achunk->next != NULL){
    if (achunk->state == FREE && achunk->next->state == FREE){ //如果下一个块也是未使用的,则合并
      achunk->size += achunk->next->size;
      achunk->next = achunk->next->next;
    }
    else{
      achunk = achunk->next;
    }
  }
}
sem_signal(heap_lock);  // 解锁
}

void *calloc(size_t num, size_t size)
{
    size_t space = num*size;
    void *ptr = malloc(space);
    memset(ptr, 0, space);
    return ptr;
}

void *realloc(void *oldptr, size_t size)
{
    if (size == 0) {
        free(oldptr);
        return NULL;
    }
    if (oldptr == NULL) {
        return malloc(size);
    }

    

    struct chunk *oldchunk = (struct chunk *)(((uint8_t *)oldptr)-sizeof(struct chunk));
    void *newptr = malloc(size);
    if (!newptr) {
      
      return NULL;
    }
      struct chunk *newchunk = (struct chunk *)(((uint8_t *)newptr) - sizeof(struct chunk));

    //下面的复制操作的复制内容包括了chunk的头部
    if (oldchunk->size<=newchunk->size){
      memcpy(newptr,oldptr,oldchunk->size-sizeof(struct chunk)); //将原来的数据拷贝到新的内存块中
      free(oldptr); //释放原来的内存块
      
      return newptr;
    }
    else if (oldchunk->size>newchunk->size){
      memcpy(newptr,oldptr,newchunk->size-sizeof(struct chunk)); //只复制newchunk长度的数据
      free(oldptr); //释放原来的内存块
      
      return newptr;
      
    }
}

/*************D O  N O T  T O U C H  A N Y T H I N G  B E L O W*************/
static void tsk_malloc(void *pv)
{
  int i, c = (int)pv;
  char **a = malloc(c*sizeof(char *));
  for(i = 0; i < c; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < c; i++) {
	  free(a[i]);
  }
  free(a);

  task_exit(0);
}

#define MESSAGE(foo) printf("%s, line %d: %s", __FILE__, __LINE__, foo)
void test_allocator()
{
  char *p, *q, *t;

  MESSAGE("[1] Test malloc/free for unusual situations\r\n");

  MESSAGE("  [1.1]  Allocate small block ... ");
  p = malloc(17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");

  MESSAGE("  [1.2]  Allocate big block ... ");
  q = malloc(4711);
  if (q == NULL) {
    printf("FAILED\r\n");
	return;
  }
  q[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [1.3]  Free big block ... ");
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.4]  Free small block ... ");
  free(p);
  printf("PASSED\r\n");

  MESSAGE("  [1.5]  Allocate huge block ... ");
  q = malloc(32*1024*1024-sizeof(struct chunk));
  if (q == NULL) {
    printf("FAILED\r\n");
	return;
  }
  q[32*1024*1024-sizeof(struct chunk)-1]=17;
  free(q);
  printf("PASSED\r\n");

  MESSAGE("  [1.6]  Allocate zero bytes ... ");
  if ((p = malloc(0)) != NULL) {
    printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.7]  Free NULL ... ");
  free(p);
  printf("PASSED\r\n");

  MESSAGE("  [1.8]  Free non-allocated-via-malloc block ... ");
  int arr[5] = {0x55aa4711,0x5a5a1147,0xa5a51471,0xaa551741,0x5aa54171};
  free(&arr[4]);
  if(arr[0] == 0x55aa4711 &&
     arr[1] == 0x5a5a1147 &&
	 arr[2] == 0xa5a51471 &&
	 arr[3] == 0xaa551741 &&
	 arr[4] == 0x5aa54171) {
	  printf("PASSED\r\n");
  } else {
	  printf("FAILED\r\n");
	  return;
  }

  MESSAGE("  [1.9]  Various allocation pattern ... ");
  int i;
  size_t pagesize = sysconf(_SC_PAGESIZE);
  for(i = 0; i < 7411; i++){
    p = malloc(pagesize);
	p[pagesize-1]=17;
    q = malloc(pagesize * 2 + 1);
	q[pagesize*2]=17;
    t = malloc(1);
	t[0]=17;
    free(p);
    free(q);
    free(t);
  }

  char **a = malloc(2741*sizeof(char *));
  for(i = 0; i < 2741; i++) {
	  a[i]=malloc(i+1);
	  a[i][i]=17;
  }
  for(i = 0; i < 2741; i++) {
	  free(a[i]);
  }
  free(a);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [1.10] Allocate using calloc ... ");
  int *x = calloc(17, 4);
  for(i = 0; i < 17; i++)
	  if(x[i] != 0) {
		  printf("FAILED\r\n");
		  return;
	  } else
	      x[i] = i;
  free(x);
  printf("PASSED\r\n");

  MESSAGE("[2] Test realloc() for unusual situations\r\n");

  MESSAGE("  [2.1]  Allocate 17 bytes by realloc(NULL, 17) ... ");
  p = realloc(NULL, 17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  p[0] = p[16] = 17;
  printf("PASSED\r\n");
  MESSAGE("  [2.2]  Increase size by realloc(., 4711) ... ");
  p = realloc(p, 4711);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
    printf("FAILED\r\n");
	return;
  }
  p[4710] = 47;
  printf("PASSED\r\n");

  MESSAGE("  [2.3]  Decrease size by realloc(., 17) ... ");
  p = realloc(p, 17);
  if (p == NULL) {
    printf("FAILED\r\n");
	return;
  }
  if ( p[0] != 17 || p[16] != 17 ) {
    printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");

  MESSAGE("  [2.4]  Free block by realloc(., 0) ... ");
  p = realloc(p, 0);
  if (p != NULL) {
	printf("FAILED\r\n");
    return;
  } else
	printf("PASSED\r\n");

  MESSAGE("  [2.5]  Free block by realloc(NULL, 0) ... ");
  p = realloc(realloc(NULL, 0), 0);
  if (p != NULL) {
    printf("FAILED\r\n");
    return;
  } else
	printf("PASSED\r\n");

  MESSAGE("[3] Test malloc/free for thread-safe ... ");

  int t1, t2;
  char *s1 = malloc(1024*1024),
       *s2 = malloc(1024*1024);
  t1=task_create(s1+1024*1024, tsk_malloc, (void *)5000);
  t2=task_create(s2+1024*1024, tsk_malloc, (void *)5000);
  task_wait(t1, NULL);
  task_wait(t2, NULL);
  free(s1);
  free(s2);

  if(chunk_head->next != NULL || chunk_head->size != 32*1024*1024) {
	printf("FAILED\r\n");
	return;
  }
  printf("PASSED\r\n");
}
/*************D O  N O T  T O U C H  A N Y T H I N G  A B O V E*************/
