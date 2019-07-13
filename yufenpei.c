#include "yufenpeitou.h"

void display_freearea_list();

int main(int argc, char *argv[]) {
	FF();
	return 0;
}


void print_space(int num) {                           //显示若干个空格
	int i;
	for (i = 0;i < num;i++) {
		printf(" ");
	}
}

void display_thread_residence_memory_list() {          //显示驻留线程链表
	THREAD_RESIDENCE_MEMORY *p;
	char buffer[20];
	p = p_thread_residence_memory_list;
	printf("|-------------------|--------------------|------------------|\n");
	printf("| thread_name       | start_address(kB)  | size(KB)         |\n");
	printf("|-------------------|--------------------|------------------|\n");
	while (p != NULL) {
		printf("| %s", p->thread_name);
		print_space(18 - strlen(p->thread_name));
		printf("| %d", p->start_address);
		itoa(p->start_address, buffer, 10);
		print_space(19 - strlen(buffer));
		printf("| %d", p->size);
		itoa(p->size, buffer, 10);
		print_space(17 - strlen(buffer));
		printf("|\n");
		p = p->next;
	};
	printf("|-------------------|--------------------|------------------|\n\n");
}

//最先适应分配法：初始化空闲区链表
FREEAREA *FF_initialize_freearea_list(FREEAREA *init_table, int num) {
	FREEAREA *temp;
	FREEAREA *head = NULL;
	FREEAREA *tail = NULL;
	int i;
	for (i = 0;i < num;i++) {
		temp = (FREEAREA *)malloc(sizeof(FREEAREA));
		temp->start_address = init_table[i].start_address;
		temp->size = init_table[i].size;
		temp->next = NULL;
		if (head == NULL)
			head = tail = temp;
		else {
			tail->next = temp;
			tail = tail->next;
		}
	};
	return head;
}

//最先适应分配法：删除空闲区链表
void FF_delete_freearea_list() {
	FREEAREA *temp;
	temp = p_free_area_list;
	while (temp != NULL) {
		temp = p_free_area_list->next;
		free(p_free_area_list);
		p_free_area_list = temp;
	}
	p_free_area_list = NULL;
}


//最先适应分配法：初始化内存申请链表
REQUIRE_MEMORY *FF_initialize_require_memory_list(REQUIRE_MEMORY *init_table, int num) {
	REQUIRE_MEMORY *temp;
	REQUIRE_MEMORY *head = NULL;
	REQUIRE_MEMORY *tail = NULL;
	int i;
	for (i = 0;i < num;i++) {
		temp = (REQUIRE_MEMORY *)malloc(sizeof(REQUIRE_MEMORY));
		strcpy(temp->thread_name, init_table[i].thread_name);
		temp->size = init_table[i].size;
		temp->duration = init_table[i].duration;
		temp->next = NULL;
		if (head == NULL)
			head = tail = temp;
		else {
			tail->next = temp;
			tail = tail->next;
		}
	};
	return head;
}

//最先适应分配法：删除内存申请链表
void FF_delete_require_memory_list() {
	REQUIRE_MEMORY *temp;
	temp = p_thread_require_memory_queue;
	while (temp != NULL) {
		temp = p_thread_require_memory_queue->next;
		free(p_thread_require_memory_queue);
		p_thread_require_memory_queue = temp;
	}
	p_thread_require_memory_queue = NULL;
}

//最先适应分配法：初始化线程驻留区链表
THREAD_RESIDENCE_MEMORY *FF_initialize_thread_residence_memory_list(THREAD_RESIDENCE_MEMORY *init_table, int num) {
	THREAD_RESIDENCE_MEMORY *temp;
	THREAD_RESIDENCE_MEMORY *head = NULL;
	THREAD_RESIDENCE_MEMORY *tail = NULL;
	int i;
	for (i = 0;i < num;i++) {
		temp = (THREAD_RESIDENCE_MEMORY *)malloc(sizeof(THREAD_RESIDENCE_MEMORY));
		strcpy(temp->thread_name, init_table[i].thread_name);
		temp->start_address = init_table[i].start_address;
		temp->size = init_table[i].size;
		temp->next = NULL;
		if (head == NULL)
			head = tail = temp;
		else {
			tail->next = temp;
			tail = tail->next;
		}
	};
	tail_thread_residence_memory_list = tail;
	return head;
}

//最先适应分配法：删除线程驻留区链表
void FF_delete_thread_residence_memory_list() {
	THREAD_RESIDENCE_MEMORY *temp = p_thread_residence_memory_list;

	temp = p_thread_residence_memory_list;
	while (temp != NULL) {
		temp = p_thread_residence_memory_list->next;
		free(p_thread_residence_memory_list);
		p_thread_residence_memory_list = temp;
	}
	p_thread_residence_memory_list = NULL;
}

//线程：申请内存，驻留一段时间，释放内存
void FF_thread(void *data) {
	int start_address = -1;
	THREAD_RESIDENCE_MEMORY *temp;
	EnterCriticalSection(&CS_SCREEN);
	printf("创建线程:%s\n", ((REQUIRE_MEMORY *)(data))->thread_name);
	LeaveCriticalSection(&CS_SCREEN);


	while (1) {                                                        //申请内存
		start_address = FF_require_memory(((REQUIRE_MEMORY *)(data))->size);
		if (start_address >= 0)
			break;
		else
			Sleep(1000);
	}
	temp = (THREAD_RESIDENCE_MEMORY *)malloc(sizeof(THREAD_RESIDENCE_MEMORY));
	strcpy(temp->thread_name, ((REQUIRE_MEMORY *)(data))->thread_name);
	temp->start_address = start_address;
	temp->size = ((REQUIRE_MEMORY *)(data))->size;
	temp->next = NULL;
	EnterCriticalSection(&CS_THREAD_MEMORY_LIST);
	//加入线程驻留区链表
	tail_thread_residence_memory_list->next = temp;
	tail_thread_residence_memory_list = tail_thread_residence_memory_list->next;
	LeaveCriticalSection(&CS_THREAD_MEMORY_LIST);
	//显示线程驻留区链表
	EnterCriticalSection(&CS_SCREEN);
	printf("%s %s\n", ((REQUIRE_MEMORY *)(data))->thread_name, "进入内存之后:");
	display_thread_residence_memory_list();
	printf("空闲区表:\n");
	display_freearea_list();
	LeaveCriticalSection(&CS_SCREEN);

	Sleep(((REQUIRE_MEMORY *)(data))->duration);
	//释放内存
	FF_release_memory(start_address, ((REQUIRE_MEMORY *)(data))->size);
}


//最先适应分配法：内存申请函数
int FF_require_memory(int size) {
	int start_address = -1;
	FREEAREA *p;
	FREEAREA *p_next;
	EnterCriticalSection(&CS_FREEAREA_LIST);
	p = p_next = p_free_area_list;
	while (p_next != NULL) {
		if (size == p_next->size) {                           //刚好满足要求，删除空闲区结点
			start_address = p_next->start_address;
			if (p_next == p_free_area_list)
				p_free_area_list = p_next->next;
			else
				p->next = p_next->next;
			free(p_next);
			break;
		}
		else
			if (size < p_next->size) {                      //分割空闲区结点
				start_address = p_next->start_address;
				p_next->start_address += size;
				p_next->size -= size;
				break;
			}
			else
			{
				p = p_next;
				p_next = p_next->next;
			}
	}

	LeaveCriticalSection(&CS_FREEAREA_LIST);
	return start_address;
}

//最先适应分配法：内存释放函数
void FF_release_memory(int start_address, int size) {
	EnterCriticalSection(&CS_FREEAREA_LIST);

	FREEAREA *temp, *p, *pp, *del_p;
	//将空闲区按start_address由小到大排序，以便整合相邻空闲区
	while (1) {
		int change = 0;
		p = p_free_area_list;
		if (p->next != NULL) {
			if (p->start_address > p->next->start_address) {
				pp = p->next;
				p->next = pp->next;
				pp->next = p;
				p_free_area_list = pp;
				change = 1;
			}
		}
		if (p->next != NULL) {
			while (p->next->next != NULL) {
				if (p->next->start_address > p->next->next->start_address) {
					pp = p->next->next;
					p->next->next = pp->next;
					pp->next = p->next;
					p->next = pp;
					change = 1;
				}
				p = p->next;
			}
		}
		if (change == 0) {
			break;
		}
	}
	//插入空闲区        
	temp = new FREEAREA;
	p = new FREEAREA;
	del_p = new FREEAREA;
	temp->start_address = start_address;
	temp->size = size;
	temp->next = NULL;
	p->next = p_free_area_list;
	while (p->next != NULL) {
		if (p->next->start_address > temp->start_address) {
			temp->next = p->next;
			p->next = temp;
			break;
		}
		else {
			p = p->next;
		}
	}
	if (p->next == NULL) {
		p->next = temp;
	}
	else if (temp->next == p_free_area_list) {
		p_free_area_list = temp;
	}
	//整合碎片
	while (1) {
		int change = 0;
		p = p_free_area_list;
		if (p == NULL) {
			break;
		}
		while (p->next != NULL) {
			if ((p->start_address + p->size) == (p->next->start_address)) {
				p->size = p->next->size + p->size;
				change = 1;
				if (p->next->next == NULL) {
					free(p->next);
					p->next = NULL;
				}
				else {
					del_p = p->next;
					p->next = p->next->next;
					free(del_p);
				}
			}
			if (p->next == NULL) {
				break;
			}
			else {
				p = p->next;
			}
		}
		if (change == 0) {
			break;
		}
	}
	//整理线程结束后的驻留链表
	THREAD_RESIDENCE_MEMORY *q;
	q = p_thread_residence_memory_list;
	if (q->start_address == start_address) {
		p_thread_residence_memory_list = p_thread_residence_memory_list->next;
	}
	else {
		while (q->next != NULL) {
			if (q->next->start_address == start_address) {
				if (q->next == tail_thread_residence_memory_list) {
					tail_thread_residence_memory_list = q;
				}
				q->next = q->next->next;
				break;
			}
			q = q->next;
		}
	}

	LeaveCriticalSection(&CS_FREEAREA_LIST);
}

//最先适应分配算法的初始化程序
void FF() {
	int i = 0;
	REQUIRE_MEMORY *p;
	HANDLE h_thread[MAX_THREAD];
	InitializeCriticalSection(&CS_THREAD_MEMORY_LIST);
	InitializeCriticalSection(&CS_FREEAREA_LIST);
	InitializeCriticalSection(&CS_SCREEN);
	printf("最先适应分配算法\n\n");

	p_free_area_list = FF_initialize_freearea_list(init_free_area_table, 5);
	p_thread_require_memory_queue = FF_initialize_require_memory_list(init_thread_require_memory_table, 3);
	p_thread_residence_memory_list = FF_initialize_thread_residence_memory_list(init_thread_residence_memory_table, 5);
	p = p_thread_require_memory_queue;

	display_freearea_list();
	while (p != NULL) {
		h_thread[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(FF_thread), p, 0, NULL);
		i++;
		p = p->next;
	};
	WaitForMultipleObjects(MAX_THREAD, h_thread, TRUE, -1);              //等待所有线程结束

	//显示线程结束后的空闲区
	EnterCriticalSection(&CS_SCREEN);

	printf("所有线程完成后的驻留表:\n");
	//显示驻留线程链表
	display_thread_residence_memory_list();
	printf("空闲区表:\n");
	display_freearea_list();
	LeaveCriticalSection(&CS_SCREEN);

	//删除各种链表                                                      
	FF_delete_freearea_list();
	FF_delete_require_memory_list();
	FF_delete_thread_residence_memory_list();
	getch();
	printf("\n");
}



//显示空闲区列表 
void display_freearea_list() {
	FREEAREA *p;
	char buffer[20];
	p = p_free_area_list;
	printf("|--------------------|------------------|\n");
	printf("|  start_address(kB) |     size(KB)     |\n");
	printf("|--------------------|------------------|\n");
	while (p != NULL) {
		printf("| %d", p->start_address);
		itoa(p->start_address, buffer, 10);
		print_space(19 - strlen(buffer));
		printf("| %d", p->size);
		itoa(p->size, buffer, 10);
		print_space(17 - strlen(buffer));
		printf("|\n");
		p = p->next;

	};
	printf("|--------------------|------------------|\n\n");
