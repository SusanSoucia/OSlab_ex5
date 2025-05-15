#include "graphics.h"

// drawpic.h

struct timespec delay;
void nanosleep_delay();
void drawBar(int barSize, int length, int lEdge, int upHead, COLORREF r);
void drawArr(int* arr, int arrSize, int lEdge, COLORREF color);
void drawProcess(int lEdge,int upHead,int size,int length,COLORREF color);
void clear(int x,int y, int x1, int y1);