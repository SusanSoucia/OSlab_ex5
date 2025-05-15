#include <syscall.h>
#include "vars.h"
#include "drawpic.c"
int pid, cid;
int full;
int empty;

#define NZERO 20
#define UP 0x4800
#define DOWN 0x5000
#define LEFT 0x4d00
#define RIGHT 0x4b00

extern void drawBar(int barSize, int length, int lEdge, int upHead, COLORREF r);
extern void drawProcess(int lEdge,int upHead,int size,int length,COLORREF color);

void control(int pid,int cid){
	int p1 = 20, p2 = 20;
	
	setpriority(task_getid(), 0);
	setpriority(pid, p1);
	setpriority(cid, p2);
	int key;
	while(1){

		int temp1 = 0;int temp2 = 0;
		int rightPrio = getpriority(2);
		if(temp1 != rightPrio)
			drawProcess(0,570,15,rightPrio,RGB(50,50,50));
		temp1 = rightPrio;
	
		
		int leftPrio = getpriority(3);
		if (temp2 != leftPrio)
			drawProcess(0,585,15,leftPrio,RGB(100,0,50));
		temp2 = leftPrio;
			
		key = getchar();
		
		if(UP == key){
			if(p1 < (2 * NZERO - 1)){
				p1++;
				setpriority(pid, p1);
			}
		}
		else if(DOWN == key){
			if(p1 > 0){
				p1--;
				setpriority(pid, p1);
			}
		}
		else if(LEFT == key){
			if(p2 < (2 * NZERO - 1)){
				p2++;
				setpriority(cid, p2);
			}
		}
		else if(RIGHT == key){
			if(p2 > 0){
				p2--;
				setpriority(cid, p2);
			}
		}


	}
}

// void producer(void *p){
// 	int i;
// 	for(i=0;;i++,i %= blockSize){
// 		sem_wait(empty);
// 		if (arrs[i]!=NULL){
// 			free(arrs[i]);
// 		}

// 		srand(time(NULL));
// 		int j;
// 		int * arr;
// 		arr = malloc((arrSize)*sizeof(int));
// 		for(j=0;j<arrSize;j++){
// 			arr[j] = rand()%100;
// 		}
// 		arrs[i] = arr;


// 	}
// }

// void consumer(void *p){
// 	int i;
// 	for (i =0 ;;i++){
// 		sem_wait(full);

// 		insertionSort(arrs[i%blockSize],arrSize,130*(i%blockSize));    //调用排序函数
// 		sem_signal(empty);
// 	}

// }
