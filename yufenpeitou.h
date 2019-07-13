#include <windows.h>
#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <string.h>



#define MAX_THREAD 3
#define BF_initialize_require_memory_list FF_initialize_require_memory_list
#define WF_initialize_require_memory_list FF_initialize_require_memory_list
#define BF_initialize_thread_residence_memory_list FF_initialize_thread_residence_memory_list
#define WF_initialize_thread_residence_memory_list FF_initialize_thread_residence_memory_list
#define WF_delete_freearea_list FF_delete_freearea_list
#define BF_delete_freearea_list FF_delete_freearea_list
#define WF_delete_require_memory_list FF_delete_require_memory_list
#define BF_delete_require_memory_list FF_delete_require_memory_list
#define WF_delete_thread_residence_memory_list FF_delete_thread_residence_memory_list
#define BF_delete_thread_residence_memory_list FF_delete_thread_residence_memory_list

typedef struct freearea {                    //表示空闲区域的数据结构
	struct freearea *next;                  //指向下一个结点的指针
	int start_address;                     //空闲区起始地址
	int size;                              //空闲区大小
}FREEAREA;

typedef struct require_memory {             //记录线程申请内存的数据结构
	struct require_memory *next;           //指向下一个结点的指针
	char thread_name[10];                //线程名
	int size;                             //申请内存大小（以KB为单位）
	int duration;                         //在内存的驻留时间（以秒为单位）
}REQUIRE_MEMORY;

typedef struct thread_residence_memory {         //描述线程驻留区的数据结构
	struct thread_residence_memory *next;       //指向下一个结点的指针
	char thread_name[10];                     //线程名
	int start_address;                          //驻留区起始地址
	int size;                                   //驻留区大小
}THREAD_RESIDENCE_MEMORY;

FREEAREA init_free_area_table[5] = {             //测试数据：初始空闲区表
	{NULL,10,10},
	{NULL,40,30},
	{NULL,80,5},
	{NULL,145,15},
	{NULL,180,20}
};

REQUIRE_MEMORY init_thread_require_memory_table[3] = {       //测试数据：初始内存申请表
	{NULL,"thread_1",20,4},
	{NULL,"thread_2",10,5},
	{NULL,"thread_3",5,6}
};
//测试数据：初始线程驻留区表
THREAD_RESIDENCE_MEMORY init_thread_residence_memory_table[5] = {
	{NULL,"a",0,10},
	{NULL,"b",20,20},
	{NULL,"c",70,10},
	{NULL,"d",85,60},
	{NULL,"e",160,20}
};

FREEAREA *p_free_area_list = NULL;                                              //空闲区链首
REQUIRE_MEMORY *p_thread_require_memory_queue = NULL;                //内存申请队列队首
THREAD_RESIDENCE_MEMORY *p_thread_residence_memory_list = NULL;      //线程驻留区链首
THREAD_RESIDENCE_MEMORY *tail_thread_residence_memory_list = NULL;     //线程驻留区链尾
CRITICAL_SECTION CS_THREAD_MEMORY_LIST;                //保护线程驻留区链表的临界区
CRITICAL_SECTION CS_SCREEN;                                         //保护屏幕的临界区
CRITICAL_SECTION CS_FREEAREA_LIST;                           //保护空闲区链表的临界区
HANDLE h_thread[MAX_THREAD];                                              //线程句柄数组

void print_space(int num);                                                     //输出若干个空格
void display_thread_residence_memory_list();                                 //显示线程驻留区表
//最先适应分配法的函数
FREEAREA *FF_initialize_freearea_list(FREEAREA *init_table, int num);          //初始化空闲区链表
void FF_delete_freearea_list();                                                //删除空闲区链表
REQUIRE_MEMORY *FF_initialize_require_memory_list(REQUIRE_MEMORY *init_table, int num);
//初始化内存申请链表
void FF_delete_require_memory_list();                                       //删除内存申请链表
THREAD_RESIDENCE_MEMORY  *FF_initialize_thread_residence_memory_list
(THREAD_RESIDENCE_MEMORY *init_table, int num);                   //初始化线程驻留区链表
void FF_delete_thread_residence_memory_list();                            //删除线程驻留区链表
void FF_thread(void *data);                                                          //线程函数
int FF_require_memory(int size);                                                  //内存申请函数
void FF_release_memory(int start_address, int size);                                 //内存释放函数
void FF();                                                      //最先适应分配算法的初始化函数

//最佳适应分配算法的函数
void BF_thread(void *data);                                                         //线程函数
int BF_require_memory(int size);                                                 //内存申请函数
void BF_release_memory(int start_address, int size);                                //内存释放函数
void BF_insert_freearea(FREEAREA *free_node);                            //空闲区结点插入函数
void BF();                                                                       //初始化程序
void BF_initialize_freearea_list(FREEAREA *init_table, int num);                  //初始化空闲区链表

//最坏适应分配算法的函数
void WF_thread(void *data);                                                        //线程函数
void WF_insert_freearea(FREEAREA *free_node);                          //空闲区结点插入函数
void WF_initialize_freearea_list(FREEAREA *init_table, int num);                //初始化空闲区链表
int WF_require_memory(int size);                                               //内存申请函数
void WF_release_memory(int start_address, int size);                              //内存释放函数
void WF();                                                                      //初始化程序
