#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <assert.h>
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

typedef void(*func)(void);
func TASKS[6];

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
void hw_wakeup_ptr(struct node_t* node);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

// about queueing time
void update_start_waiting();
void update_total_waiting();
void update_all_start_waiting();
void update_all_total_waiting();
void update_all_sleep();

// about timer
int set_timer(int msec);
unsigned int get_time();

int set_to_running(struct node_t* task);
bool any_ready_task();
bool any_waiting_task();
void terminate();
void simulating();

// about linked list
int enqueue(char* task, int quantum_time);
int remove_task();
void enqueue_ready(struct node_t *node);
struct node_t* dequeue_ready();

void shell_mode();
void signal_handler(int sig);

// init
void signal_init();
void tasks_init();
void list_init();

// print
void print_all();
void print_ready_queue();

// error
void throw_unexpected(const char *err_msg);
void invalid(const char* err_input, const char* type);

ucontext_t SIMULATOR;
ucontext_t TERMINATE;
struct node_t* RUNNING_TASK;

#endif
