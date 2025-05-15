#ifndef SSORT_H  // 头文件守卫防止重复包含
#define SSORT_H

#include "drawpic.h"

int* createArray(int size);
void insertionSort(int arr[],int size,int lEdge);
void bubbleSort(int arr[], int size,int lEdge);
#endif