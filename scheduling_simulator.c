#include "scheduling_simulator.h"


int main()
{
	signal(SIGTSTP, ctrl_z_handler);
	while(1);
	return 0;
}

void ctrl_z_handler(int n)
{
	shell_mode();
}

void shell_mode()
{
	while(1) {
		printf("$ ");
		while(1);
	}
}

void hw_suspend(int msec_10)
{
	return;
}

void hw_wakeup_pid(int pid)
{
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	return 0;
}

int hw_task_create(char *task_name)
{
	return 0; // the pid of created task name
}

void debug(struct node_t* start, struct node_t* end)
{

}

void enqueue(int task, int quantum_time)
{
	static int pid = 0;

	// allocate memory
	struct node_t *node = (struct node_t*)malloc(sizeof(struct node_t));

	// init
	node->pid = ++pid;
	node->state = TASK_READY;
	node->quantum_time = quantum_time;
	node->total_waiting = 0;
	node->start_waiting = 0;
	node->next = NULL;
	node->next_ready = NULL;

	// insert into main queue
	if (list_front == NULL && list_rear == NULL)
		list_front = list_rear = node;
	else if(list_front && list_rear) {
		list_rear->next = node;
		list_rear = node;
	} else {
		throw_unexpected("main queue error");
	}

	// insert into ready queue
	enqueue_ready(node);
}

void enqueue_ready(struct node_t *node)
{
	// save timestamp
	if(node)
		node->start_waiting = 0; //TODO = now
	else
		throw_unexpected("added NULL to ready queue");

	// insert into ready queue
	if (ready_front == NULL && ready_rear == NULL)
		ready_front = ready_rear = node;
	else if(ready_front && ready_rear) {
		ready_rear->next_ready = node;
		ready_rear = node;
	} else {
		throw_unexpected("ready queue error");
	}
}

struct node_t* dequeue_ready()
{
	struct node_t* tmp = ready_front;
	if(ready_front) {
		// add waiting time
		tmp->total_waiting += 0; // TODO calculate

		// remove from ready queue
		if(ready_front == ready_rear)
			ready_front = ready_rear = NULL;
		else
			ready_front = ready_front->next_ready;
	}
	return tmp;
}

void throw_unexpected(const char *err_msg)
{
	fprintf(stderr, "%s\n", err_msg);
	exit(1);
}
