/**
 * vim: filetype=c:fenc=utf-8:ts=4:et:sw=4:sts=4
 */
#include <stddef.h>
#include "kernel.h"

#define SEMAPHORE_MAX 32 //信号量的最大数量
struct semaphore {
    int id; 
    int value; 
    struct wait_queue *wq; 
};



// 改为指针数组，sems数组存储指针，每个指针指向一个动态分配的结构体
struct semaphore *sems[SEMAPHORE_MAX] = {NULL}; // 初始化为NULL

int sys_sem_create(int value)
{
    int i;
    struct semaphore *sem;

    // 从 1 开始遍历（保留 0 作为无效 ID）
    for(i = 1; i < SEMAPHORE_MAX; i++) {
        if(sems[i] == NULL) { // 找到空槽位
            break;
        }
    }

    if(i >= SEMAPHORE_MAX) {
        return -1; // 无可用槽位
    }

    // 使用 kmalloc 动态分配内存
    if((sem = kmalloc(sizeof(struct semaphore))) == NULL) {
        return -1; // 内存分配失败
    }

    // 初始化信号量
    sem->id = i;       // ID 等于数组索引
    sem->value = value;
    sem->wq = NULL;    // 等待队列初始为空

    // 将指针存入数组
    sems[i] = sem;
    return i; // 返回信号量 ID
}

int sys_sem_destroy(int semid)
{
    if(semid <= 0 || semid >= SEMAPHORE_MAX || sems[semid] == NULL) {
        return -1; // 无效 ID
    }
    kfree(sems[semid]);
    sems[semid] = NULL; //置空指针
    return 0;
}

int sys_sem_wait(int semid)
{
    if (semid <= 0 || semid >= SEMAPHORE_MAX || sems[semid] == NULL) {
        return -1; // 无效 ID
    }
    uint32_t flags;
    save_flags_cli(flags);
    sems[semid]->value--;
    if (sems[semid]->value < 0) {
        // 如果信号量值小于0，线程需要等待
        sleep_on(&sems[semid]->wq);
    }
    restore_flags(flags);
    return 0;
}

int sys_sem_signal(int semid)
{
    if (semid <= 0 || semid >= SEMAPHORE_MAX || sems[semid] == NULL) {
        return -1; // 无效 ID
    }
    uint32_t flags;
    save_flags_cli(flags);
    sems[semid]->value++;
    if (sems[semid]->value <= 0) {
        // 如果信号量值小于等于0，唤醒一个等待的线程
        wake_up(&sems[semid]->wq, 1);
    }
    restore_flags(flags);
    return 0;
}

