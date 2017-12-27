#include "scheduling_simulator.h"
#define STACK_SIZE 8192
//#define DEMO
//#define DEBUG
#define for_each_node(head, iter, nxt) for(struct node_t* iter = (head)->nxt; iter != head; iter = (iter)->nxt)
#define push_back(head, node, pre, nxt) do{											\
											struct node_t* list_prev = (head)->pre;	\
											(head)->pre = node;						\
											list_prev->nxt = node;					\
											node->nxt = head;						\
											node->pre = list_prev;					\
										}while(0)

int main()
{
	assert(getcontext(&SHELL) != -1);
	SHELL.uc_stack.ss_sp = malloc(STACK_SIZE);
	SHELL.uc_stack.ss_size = STACK_SIZE;
	SHELL.uc_stack.ss_flags = 0;
	SHELL.uc_link = &MAIN;
	makecontext(&SHELL, shell_main, 0);

	assert(getcontext(&SCHEDULER) != -1);
	SCHEDULER.uc_stack.ss_sp = malloc(STACK_SIZE);
	SCHEDULER.uc_stack.ss_size = STACK_SIZE;
	SCHEDULER.uc_stack.ss_flags = 0;
	SCHEDULER.uc_link = &MAIN;
	makecontext(&SCHEDULER, scheduler_main, 0);

	assert(getcontext(&TERMINATE) != -1);
	TERMINATE.uc_stack.ss_sp = malloc(STACK_SIZE);
	TERMINATE.uc_stack.ss_size = STACK_SIZE;
	TERMINATE.uc_stack.ss_flags = 0;
	TERMINATE.uc_link = &MAIN;
	makecontext(&TERMINATE, terminate_main, 0);

	list_init();
	tasks_init();
	signal_init();

	assert(swapcontext(&MAIN, &SHELL) != -1);

	my_puts("main terminate");
	return 0;
}

void signal_init()
{
	struct sigaction stp;
	stp.sa_handler = &signal_handler;
	sigemptyset(&stp.sa_mask);
	sigaddset(&stp.sa_mask, SIGALRM);
	sigaddset(&stp.sa_mask, SIGTSTP);
	stp.sa_flags = 0;
	assert(sigaction(SIGALRM, &stp, NULL) == 0);
	assert(sigaction(SIGTSTP, &stp, NULL) == 0);
}

void list_init()
{
	LIST_HEAD.prev = &LIST_HEAD;
	LIST_HEAD.next = &LIST_HEAD;
	READY_HEAD.prev_ready = &READY_HEAD;
	READY_HEAD.next_ready = &READY_HEAD;
}

void tasks_init()
{
	func tasks_default[6] = {task1, task2, task3, task4, task5, task6};
	memcpy(TASKS, tasks_default, sizeof(tasks_default));
}

void shell_main()
{
	while(1) {
		CUR_UCONTEXT = &SHELL;

		printf("$ ");
		// getline
		char buf[200];
		fgets(buf, sizeof(buf), stdin);
		char* buf_ptr = (char*)buf;

		// get command
		char command[20];
		if(!my_sscanf(&buf_ptr, command))
			continue;

		// check command
		char dummy[20];
		if(!strcmp(command, "add")) {
			// get task name
			char task_name[20];
			if(!my_sscanf(&buf_ptr, task_name)) {
				my_puts("missing task name");
				continue;
			}

			// default quantum time
			int quantum_time = 10;

			// try to get option (if any)
			char option[20];
			if(my_sscanf(&buf_ptr, option)) {

				if(strcmp(option, "-t")) {
					my_printf("%s: option not found\n", option);
					continue;
				}

				// get argument
				char argument[20];
				if(!my_sscanf(&buf_ptr, argument)) {
					my_puts("missing argument");
					continue;
				}

				// check too many
				if(my_sscanf(&buf_ptr, dummy)) {
					my_puts("too many arguments");
					continue;
				}

				// check argument
				if(!strcmp(argument, "S"))
					quantum_time = 10;
				else if(!strcmp(argument, "L"))
					quantum_time = 20;
				else {
					my_printf("%s: invalid argument\n", argument);
					continue;
				}
			}

			int ret_val = enqueue(task_name, quantum_time);
			if(ret_val == -1) {
				my_printf("%s: task not found\n", task_name);
				continue;
			}

		} else if(!strcmp(command, "remove")) {
			// TODO : detect remove 2bla

			// get pid
			char pid_str[20];
			if(!my_sscanf(&buf_ptr, pid_str)) {
				puts("missing pid");
				continue;
			}
			int pid = atoi(pid_str);

			// check too many
			if(my_sscanf(&buf_ptr, dummy)) {
				puts("too many arguments");
				continue;
			}

			// remove task by pid
			if(remove_task(pid) < 0)
				my_printf("pid %d does not exist\n", pid);

		} else if(!strcmp(command, "ps")) {
			// check too  many
			if(my_sscanf(&buf_ptr, dummy)) {
				puts("too many arguments");
				continue;
			}

			print_all();
		} else if(!strcmp(command, "start")) {
			// check too  many
			if(my_sscanf(&buf_ptr, dummy)) {
				puts("too many arguments");
				continue;
			}

			update_all_start_waiting();
			swapcontext(&SHELL, &SCHEDULER);
		} else {
			my_printf("%s: command not found\n", command);
			continue;
		}
	}
}

void scheduler_main()
{
	while(1) {
		CUR_UCONTEXT = &SCHEDULER;

		// if anyone running, set to ready + add waiting queue
		if(RUNNING_TASK != NULL) {
			struct node_t* tmp = RUNNING_TASK;
			RUNNING_TASK = NULL;
			enqueue_ready(tmp);
			update_all_sleeping(tmp->quantum_time);
		}
		if(any_ready_task()) {
			struct node_t *first_ready = dequeue_ready();
			// count down and swap
			assert(set_timer(first_ready->quantum_time) == 0);
			assert(swap_to_running(first_ready) != -1);
		} else if(any_waiting_task()) {
			const int sleep_msec = 1;
			usleep(1000 * sleep_msec);
			update_all_sleeping(sleep_msec);
		} else {
			assert(swapcontext(&SCHEDULER, &SHELL) != -1);
		}
	}
}

void terminate_main()
{
	while(1) {
		assert(set_timer(0) == 0);
		CUR_UCONTEXT = &TERMINATE;
		assert(RUNNING_TASK != NULL);
#ifdef DEMO
		my_printf("pid %d terminate\n", RUNNING_TASK->pid);
#endif
		RUNNING_TASK->state = TASK_TERMINATED;
		RUNNING_TASK = NULL;
		assert(swapcontext(&TERMINATE, &SCHEDULER) != -1);
	}
}

void update_total_waiting(struct node_t* node)
{
	node->total_waiting += get_time() - node->start_waiting;
}

void update_all_total_waiting()
{
	for_each_node(&READY_HEAD, iter, next_ready) {
		update_total_waiting(iter);
	}
}

void update_start_waiting(struct node_t* node)
{
	node->start_waiting = get_time();
}

void update_all_start_waiting()
{
	for_each_node(&READY_HEAD, iter, next_ready) {
		update_start_waiting(iter);
	}
}
void update_all_sleeping(int msec)
{
	for_each_node(&LIST_HEAD, iter, next) {
		if(iter->state == TASK_WAITING && (iter->sleep_time-=msec) <= 0) {
			// += is because iter wakeup before msec passed
			iter->total_waiting += -iter->sleep_time;
			hw_wakeup_ptr(iter);
		}
	}
}

bool my_sscanf(char** buf, char* dest)
{
	if(sscanf(*buf, "%s", dest) < 0)
		return false;
	*buf += strlen(dest);
	while(**buf == ' ') // hmm... no need consume
		++*buf;
	return true;
}

void print_ready_queue()
{
	my_puts("print ready queue:");
	for_each_node(&READY_HEAD, iter, next_ready) {
		printf("%d %s %d\n", iter->pid, iter->task_name, iter->total_waiting);
	}
}

int set_timer(int msec)
{
	static struct itimerval count_down;
	count_down.it_value.tv_sec = 0;
	count_down.it_value.tv_usec = msec * 1000;
	count_down.it_interval.tv_sec = 0;
	count_down.it_interval.tv_usec = 0;
	return setitimer(ITIMER_REAL, &count_down, NULL);
}

void signal_handler(int sig)
{
	if(sig == SIGALRM) {
		assert(swapcontext(&(RUNNING_TASK->context), &SCHEDULER) != -1);
	} else if(sig == SIGTSTP) {
		set_timer(0);
		if(RUNNING_TASK)
			enqueue_ready(RUNNING_TASK);
		RUNNING_TASK = NULL;
		assert(swapcontext(CUR_UCONTEXT, &SHELL) != -1);
	} else {
		throw_unexpected("unexpected signal");
	}
}

unsigned int get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int swap_to_running(struct node_t* task)
{
#ifdef DEMO
	my_printf("pid %d set to run\n", task->pid);
#endif
	// set to running
	task->state = TASK_RUNNING;
	RUNNING_TASK = task;
	CUR_UCONTEXT = &(RUNNING_TASK->context);
	return swapcontext(&SCHEDULER, &(task->context));
}

bool any_ready_task()
{
	return READY_HEAD.next_ready != &READY_HEAD;
}

bool any_waiting_task()
{
	for_each_node(&LIST_HEAD, iter, next) {
		if(iter->state == TASK_WAITING)
			return true;
	}
	return false;
}

void print_all()
{
	my_puts("");
	my_puts("print all:");
	printf("pid task name state queueing time sleep\n");
	for_each_node(&LIST_HEAD, iter, next) {
		char state[20];
		switch(iter->state) {
		case TASK_RUNNING:
			strcpy(state, "TASK_RUNNING");
			break;
		case TASK_READY:
			strcpy(state, "TASK_READY");
			break;
		case TASK_WAITING:
			strcpy(state, "TASK_WAITING");
			break;
		case TASK_TERMINATED:
			strcpy(state, "TASK_TERMINATED");
			break;
		default:
			throw_unexpected("state not found\n");
		}
		printf("%d %s %-17s %d %d\n", iter->pid, iter->task_name, state,
		       iter->total_waiting, iter->sleep_time);
	}
	print_ready_queue();
}

void hw_suspend(int msec_10)
{
	set_timer(0);
	RUNNING_TASK->state = TASK_WAITING;
	RUNNING_TASK->sleep_time = msec_10 * 10;
	struct node_t* tmp = RUNNING_TASK;
	RUNNING_TASK = NULL;
	assert(swapcontext(&(tmp->context), &SCHEDULER) != -1);
	return;
}

void hw_wakeup_ptr(struct node_t* node)
{
	if(node->state == TASK_WAITING) {
		node->sleep_time = 0;
		enqueue_ready(node);
	}
}

void hw_wakeup_pid(int pid)
{
	for_each_node(&LIST_HEAD, iter, next) {
		if(iter->pid == pid)
			hw_wakeup_ptr(iter);
	}
}

int hw_wakeup_taskname(char *task_name)
{
	for_each_node(&LIST_HEAD, iter, next) {
		if(!strcmp(iter->task_name, task_name))
			hw_wakeup_ptr(iter);
	}
	return 0; // ?
}

int hw_task_create(char *task_name)
{
	return enqueue(task_name, 10);
}

int enqueue(char* task_name, int quantum_time)
{
	if(strlen(task_name) != 5)
		return -1;

	char str[4];
	char task_num_c;
	sscanf(task_name, "%4s%c", str, &task_num_c);
	if(strcmp(str, "task") || task_num_c < '1' || task_num_c > '6')
		return -1;

	static int pid = 0;

	// allocate memory
	struct node_t *node = (struct node_t*)malloc(sizeof(struct node_t));

	// init
	node->pid = ++pid;
	node->state = TASK_READY;
	node->quantum_time = quantum_time;
	strcpy(node->task_name, task_name);
	node->total_waiting = 0;
	node->start_waiting = 0;
	node->prev_ready = NULL;
	node->next_ready = NULL;

	assert(getcontext(&(node->context)) != -1);
	node->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	node->context.uc_stack.ss_size = STACK_SIZE;
	node->context.uc_stack.ss_flags = 0;
	node->context.uc_link = &TERMINATE;
	makecontext(&(node->context), TASKS[task_num_c - '1'], 0);

	// push back to LIST
	push_back(&LIST_HEAD, node, prev, next);

	// insert into ready queue
	enqueue_ready(node);
	return pid;
}

void enqueue_ready(struct node_t *node)
{
	node->state = TASK_READY;
	// save timestamp
	if(node)
		node->start_waiting = get_time();
	else
		throw_unexpected("added NULL to ready queue");

	// push back into ready queue
	push_back(&READY_HEAD, node, prev_ready, next_ready);
}

struct node_t* dequeue_ready()
{
	struct node_t* tmp = READY_HEAD.next_ready;
	if(tmp != &READY_HEAD) {
		update_total_waiting(tmp);

		tmp->prev_ready->next_ready = tmp->next_ready;
		tmp->next_ready->prev_ready = tmp->prev_ready;
		tmp->prev_ready = NULL;
		tmp->next_ready = NULL;
	} else { // never happened if run correctly
		my_puts("ready queue empty");
	}
	return tmp;
}

int remove_task(int pid)
{
	for_each_node(&LIST_HEAD, iter, next) {
		if(iter->pid == pid) {
			iter->next->prev = iter->prev;
			iter->prev->next = iter->next;
			if(iter->state == TASK_READY) {
				iter->next_ready->prev_ready = iter->prev_ready;
				iter->prev_ready->next_ready = iter->next_ready;
			}
			free(iter->context.uc_stack.ss_sp);
			free(iter);
			return 0;
		}
	}
	return -1;
}

void throw_unexpected(const char *err_msg)
{
	fprintf(stderr, "%s\n", err_msg);
	exit(1);
}
