#include "scheduling_simulator.h"
#define STACK_SIZE 16384
#define DEMO
//#define DEBUG
#define for_each_node(head, iter, nxt) for(iter = (head)->nxt; iter != head; iter = (iter)->nxt)
#define my_printf(format, arg) do{\
									printf(format, arg);\
									fflush(stdout);\
								}while(0)
#define my_puts(str) 	do{\
							puts(str);\
							fflush(stdout);\
						}while(0)

int main()
{
	list_init();
	tasks_init();

	shell_mode();

	signal_work(true);
	assert(getcontext(&TERMINATE) != -1);
	terminate(); //set to TERMINATE when any task return

	assert(getcontext(&SCHEDULER) != -1);
	simulating();

	my_puts("main terminate");
	return 0;
}

void terminate()
{
	assert(set_timer(0) == 0);
	if(RUNNING_TASK) {
#ifdef DEMO
		my_printf("pid %d terminate\n", RUNNING_TASK->pid);
#endif
		RUNNING_TASK->state = TASK_TERMINATED;
		RUNNING_TASK = NULL;
	}
}

void update_total_waiting(struct node_t* node)
{
	node->total_waiting += get_time() - node->start_waiting;
}

void update_all_total_waiting()
{
	struct node_t* iter;
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
	struct node_t* iter;
	for_each_node(&READY_HEAD, iter, next_ready) {
		update_start_waiting(iter);
	}
}
void update_all_sleeping(int msec)
{
	struct node_t *iter;
	for_each_node(&LIST_HEAD, iter, next) {
		if(iter->state == TASK_WAITING) {
			iter->sleep_time -= msec;
			if(iter->sleep_time <= 0) {
				iter->total_waiting += -iter->sleep_time;
				hw_wakeup_ptr(iter);
			}
		}
	}
}

void signal_work(bool work)
{
	struct sigaction stp;
	if(work)
		stp.sa_handler = &signal_handler;
	else
		stp.sa_handler = &signal_ignore;
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

bool my_sscanf(char** buf, char* dest)
{
	if(sscanf(*buf, "%s", dest) < 0)
		return false;
	*buf += strlen(dest);
	while(**buf == ' ')
		++*buf;
	return true;
}

void shell_mode()
{
	while(1) {
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
			int quantum_time = 500;

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

			// check too  many
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
			break;
		} else {
			my_printf("%s: command not found\n", command);
			continue;
		}
	}
}


void print_ready_queue_inverse()
{
#ifdef DEBUG
	my_puts("print ready queue inverse:");
#endif
	for(struct node_t* iter = READY_HEAD.prev_ready; iter != &READY_HEAD;
	    iter = iter->prev_ready)
		printf("%d %s %d\n", iter->pid, iter->task_name, iter->total_waiting);
}

void print_ready_queue()
{
#ifdef DEBUG
	my_puts("print ready queue:");
#endif
	for(struct node_t* iter = READY_HEAD.next_ready; iter != &READY_HEAD;
	    iter = iter->next_ready)
		printf("%d %s %d\n", iter->pid, iter->task_name, iter->total_waiting);
}

void simulating()
{
	while(1) {

		// if anyone running, set to ready + add waiting queue
		if(RUNNING_TASK != NULL) {
#ifdef DEBUG
			my_printf("found pid %d running\n", RUNNING_TASK->pid);
#endif
			update_all_sleeping(RUNNING_TASK->quantum_time);
			enqueue_ready(RUNNING_TASK);
			RUNNING_TASK = NULL;
		}

		if(any_ready_task()) {
			struct node_t *first_ready = READY_HEAD.next_ready;

			// count down
			assert(set_timer(first_ready->quantum_time) == 0);

			// never return if success
			assert(set_to_running(first_ready) != -1);

		} else if(any_waiting_task()) {
			const int sleep_msec = 10;
			usleep(1000 * sleep_msec);
			struct node_t* iter;
			for_each_node(&LIST_HEAD, iter, next) {
				if(iter->state == TASK_WAITING) {
					if((iter->sleep_time-=sleep_msec) <= 0) {
						; // TODO hmm...
						hw_wakeup_ptr(iter);
					}

				}

			}
			//		assert(setcontext(&SCHEDULER) != -1);
		} else {
			shell_mode();
		}
	}

}

int set_timer(int msec)
{
#ifdef DEBUG
	my_printf("set timer %d\n", msec);
#endif
	struct itimerval count_down;
	count_down.it_value.tv_sec = 0;
	count_down.it_value.tv_usec = msec * 1000;
	count_down.it_interval.tv_sec = 0;
	count_down.it_interval.tv_usec = 0;
	return setitimer(ITIMER_REAL, &count_down, NULL);
}

void signal_handler(int sig)
{
	if(sig == SIGALRM) {
#ifdef DEBUG
		my_puts("swap to simulator");
#endif
		assert(swapcontext(&(RUNNING_TASK->context), &SCHEDULER) != -1);

	} else if(sig == SIGTSTP) {
		set_timer(0);
		struct node_t* tmp = RUNNING_TASK;
		RUNNING_TASK = NULL;
		if(tmp)
			enqueue_ready(tmp);

		shell_mode();
		if(tmp) // save current task and goto scheduler
			assert(swapcontext(&(tmp->context), &SCHEDULER) != -1);

	} else {
		throw_unexpected("unexpected signal");
	}
}

void signal_ignore(int sig)
{
}

unsigned int get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000 + tv.tv_usec/1000;
}

int set_to_running(struct node_t* task)
{
	// calculate waiting time
	dequeue_ready(task);
#ifdef DEMO
	my_printf("pid %d set to run\n", task->pid);
#endif
	// set to running
	task->state = TASK_RUNNING;
	RUNNING_TASK = task;
	return setcontext(&(task->context));
}

bool any_ready_task()
{
	return READY_HEAD.next_ready != &READY_HEAD;
}

bool any_waiting_task()
{
	struct node_t* iter;
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
	struct node_t* tmp = LIST_HEAD.next;
	printf("pid task name state queueing time sleep\n");
	while(tmp != &LIST_HEAD) {
		char state[20];
		switch(tmp->state) {
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
		printf("%d %s %-17s %d %d\n", tmp->pid, tmp->task_name, state,
		       tmp->total_waiting, tmp->sleep_time);
		tmp = tmp->next;
	}
	my_puts("");
	my_puts("print all inverse:");
	tmp = LIST_HEAD.prev;
	printf("pid task name state queueing time sleep\n");
	while(tmp != &LIST_HEAD) {
		char state[20];
		switch(tmp->state) {
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
		printf("%d %s %-17s %d %d\n", tmp->pid, tmp->task_name, state,
		       tmp->total_waiting, tmp->sleep_time);
		tmp = tmp->prev;
	}
	my_puts("");
	print_ready_queue();
	my_puts("");

	print_ready_queue_inverse();
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
	if(node->state == TASK_WAITING)
		enqueue_ready(node);
}

void hw_wakeup_pid(int pid)
{
	struct node_t* iter;
	for_each_node(&LIST_HEAD, iter, next)
	if(iter->state == TASK_WAITING)
		enqueue_ready(iter);
	return;
}

int hw_wakeup_taskname(char *task_name)
{
	struct node_t* iter;
	for_each_node(&LIST_HEAD, iter, next) {
		if(!strcmp(iter->task_name, task_name) && iter->state == TASK_WAITING)
			enqueue_ready(iter);
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
	signal_work(false);
	struct node_t *list_prev = LIST_HEAD.prev;
	LIST_HEAD.prev = node;
	list_prev->next = node;
	node->next = &LIST_HEAD;
	node->prev = list_prev;
	signal_work(true);

	// insert into ready queue
	enqueue_ready(node);
	return pid;
}

void enqueue_ready(struct node_t *node)
{
#ifdef DEBUG
	my_printf("pid %d enqueue ready\n", node->pid);
#endif

	node->state = TASK_READY;
	// save timestamp
	if(node)
		node->start_waiting = get_time(); //TODO = now
	else
		throw_unexpected("added NULL to ready queue");

	// insert into ready queue
	struct node_t* ready_prev = READY_HEAD.prev_ready;
	READY_HEAD.prev_ready = node;
	ready_prev->next_ready = node;
	node->next_ready = &READY_HEAD;
	node->prev_ready = ready_prev;

}

struct node_t* dequeue_ready()
{
	struct node_t* tmp = READY_HEAD.next_ready;
	if(tmp != &READY_HEAD) {
		// add waiting time
#ifdef DEBUG
		my_printf("pid %d dequeue ready\n", tmp->pid);
#endif
		tmp->total_waiting += get_time() - tmp->start_waiting; // TODO calculate
		tmp->prev_ready->next_ready = tmp->next_ready;
		tmp->next_ready->prev_ready = tmp->prev_ready;
		tmp->prev_ready = NULL;
		tmp->next_ready = NULL;

	} else {
#ifdef DEMO
		my_puts("ready queue empty");
#endif
	}
	return tmp;
}

int remove_task(int pid)
{
	struct node_t* iter = LIST_HEAD.next;
	while(iter != &LIST_HEAD) {
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
		iter = iter->next;
	}
	return -1;
}


void throw_unexpected(const char *err_msg)
{
	fprintf(stderr, "%s\n", err_msg);
	exit(1);
}
