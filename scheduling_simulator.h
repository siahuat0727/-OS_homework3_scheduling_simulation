#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <ucontext.h>
#include <signal.h>

#include "task.h"

enum TASK_STATE {
	TASK_RUNNING,
	TASK_READY,
	TASK_WAITING,
	TASK_TERMINATED
};

struct node_t {
	int pid;
	enum TASK_STATE state;
	int quantum_time;
	char task_name[6];
	clock_t total_waiting;
	clock_t start_waiting;
	struct node_t* prev;
	struct node_t* next;
	struct node_t* prev_ready;
	struct node_t* next_ready;
};

struct node_t LIST_HEAD, READY_HEAD;

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);
int enqueue(char* task, int quantum_time);
void enqueue_ready(struct node_t *node);
struct node_t* dequeue_ready();
void shell_mode();
void throw_unexpected(const char *err_msg);
void ctrl_z_handler(int n);
void list_init();
int remove_task();

#endif
