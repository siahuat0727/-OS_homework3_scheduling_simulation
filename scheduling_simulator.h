#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
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

	// unit = ms
	unsigned int total_waiting;
	unsigned int start_waiting;
	unsigned int sleep_time;

	// pointers
	struct node_t* prev;
	struct node_t* next;
	struct node_t* prev_ready;
	struct node_t* next_ready;

	// context
	ucontext_t context;
};

struct node_t LIST_HEAD, READY_HEAD;

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

int set_timer(int msec);
void timer_handler(int n);
unsigned int get_time();
int set_to_running(struct node_t* task);
bool any_ready_task();
void simulating();
int enqueue(char* task, int quantum_time);
void enqueue_ready(struct node_t *node);
struct node_t* dequeue_ready();
void shell_mode();
void throw_unexpected(const char *err_msg);
void ctrl_z_handler(int n);
void list_init();
int remove_task();
void invalid(const char* err_input, const char* type);
void print_all();

ucontext_t SIMULATOR;
struct node_t* RUNNING_TASK;

#endif
