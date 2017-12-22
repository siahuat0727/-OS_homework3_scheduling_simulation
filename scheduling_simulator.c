#include "scheduling_simulator.h"
#define STACK_SIZE 8192
#define DEBUG
#define for_each_node(head, iter, nxt) for(iter = (head)->nxt; iter != head; iter = (iter)->nxt)

int main()
{
	signal(SIGTSTP, signal_handler);
	signal(SIGALRM, signal_handler);

//	int i = 0;
//	while(1){
//		int delay = 50000000;
//		while(delay--);
//		printf("while in main %d\n", ++i);
//	}
	list_init();
	tasks_init();

	shell_mode();
	assert(getcontext(&SIMULATOR) != -1);
	simulating();

	puts("main terminate");
	return 0;
}

void update_waiting_time()
{
	struct node_t* iter;
	for_each_node(&READY_HEAD, iter, next_ready) {
		iter->start_waiting = get_time();
	}
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
		char buf[1000];
		fgets(buf, sizeof(buf), stdin);
		char* buf_ptr = (char*)buf;
		char command[100];
		my_sscanf(&buf_ptr, command);

		if(!strcmp(command, "add")) {
			// get task name
			char task_name[20];
			my_sscanf(&buf_ptr, task_name);

			int quantum_time = 500;

			// try to get option and argument
			char option[20];
			if(my_sscanf(&buf_ptr, option)) {
				if(strcmp(option, "-t")) {
					printf("%s: option not found\n", option);
					continue;
				}
				char argument[20];
				my_sscanf(&buf_ptr, argument);
				if(!strcmp(argument, "S"))
					quantum_time = 10;
				else if(!strcmp(argument, "L"))
					quantum_time = 20;
				else {
					printf("%s: invalid argument\n", argument);
					continue;
				}
			}

			int ret_val = enqueue(task_name, quantum_time);
			if(ret_val == -1) {
				printf("%s: task not found\n", task_name);
				continue;
			}

		} else if(!strcmp(command, "remove")) {
			int n, pid;
			n = scanf("%d", &pid); // TODO change sscanf
			bool more_char = false;
			while(getchar()!='\n')
				more_char = true;
			if(!more_char && n == 1) {
				if(remove_task(pid) < 0)
					printf("pid %d does not exist\n", pid);
			} else {
				puts("Invalid parameter for remove");
				continue;
			}

		} else if(!strcmp(command, "ps")) {
			print_all();
		} else if(!strcmp(command, "start")) {
			update_waiting_time();
			break; // TODO check if any other input
		} else {
			printf("%s: command not found\n", command);
			continue;
		}
	}
}


void print_ready_queue()
{
	puts("print ready queue:");
	for(struct node_t* iter = READY_HEAD.next_ready; iter != &READY_HEAD;
	    iter = iter->next_ready)
		printf("%d %s %d\n", iter->pid, iter->task_name, iter->total_waiting);
}

// running
void simulating()
{
	puts("in simulating");

	// if anyone running, set to ready + add waiting queue
	if(RUNNING_TASK != NULL) {
#ifdef DEBUG
		printf("found pid %d running\n", RUNNING_TASK->pid);
#endif
		struct node_t *iter;
		puts("update waiting time");
		for_each_node(&LIST_HEAD, iter, next) {
			if(iter->state == TASK_WAITING)
				iter->sleep_time -= RUNNING_TASK->quantum_time;
			if(iter->sleep_time < 0) {
				iter->total_waiting += -iter->sleep_time;
				enqueue_ready(iter);
			}
		}
		enqueue_ready(RUNNING_TASK);
		RUNNING_TASK = NULL;
	}

	if(any_ready_task()) {
		struct node_t *first_ready = READY_HEAD.next_ready;
#ifdef DEBUG
		printf("found first ready pid=%d\n", first_ready->pid);
#endif
		// count down
		if(set_timer(first_ready->quantum_time)) {
			puts("set timer failed");
			exit(1);
		}

		// never return if success
		assert(set_to_running(first_ready) != -1);
		perror("setcontext failed");
		exit(1);

	} else {
		puts("no ready task");
	}

}

int set_timer(int msec)
{
	printf("set timer %d\n", msec);
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
		puts("in timer handler");
		puts("swap to simulator");
		assert(swapcontext(&(RUNNING_TASK->context), &SIMULATOR) != -1);
		//	if(running_task != NULL){
		//		enqueue_ready(running_task);
		//
		//	}
	} else if(sig == SIGTSTP) {
		set_timer(0);
		puts("in ctrl z handler");
		struct node_t* tmp = RUNNING_TASK;
		RUNNING_TASK = NULL;
		print_ready_queue();
		if(tmp)
			enqueue_ready(tmp);
		print_ready_queue();

		// TODO if someone running, pull back to ready
		shell_mode();
		if(tmp)
			assert(swapcontext(&(tmp->context), &SIMULATOR) != -1);
		else
			assert(setcontext(&SIMULATOR) != -1);
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

int set_to_running(struct node_t* task)
{
	// calculate waiting time
	dequeue_ready(task);
#ifdef DEBUG
	printf("set pid %d to run\n", task->pid);
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


void print_all()
{
	puts("");
	puts("print all:");
	struct node_t* tmp = LIST_HEAD.next;
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
		printf("%d %s %-17s %d\n", tmp->pid, tmp->task_name, state, tmp->total_waiting);
		tmp = tmp->next;
	}
	puts("");
	print_ready_queue();
	puts("");
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

void lower(char *str)
{
	for(char* c = str; *c != '\0'; ++c)
		*c |= 1<<5;
}

int enqueue(char* task_name, int quantum_time)
{
	if(strlen(task_name) != 5)
		return -1;

	char str[4];
	char task_num_c;
	sscanf(task_name, "%4s%c", str, &task_num_c);
	lower(str);
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
	node->context.uc_link = &SIMULATOR;
	makecontext(&(node->context), TASKS[task_num_c - '1'],
	            0); //TODO function pointer array

	// push back to LIST
	struct node_t *list_prev = LIST_HEAD.prev;
	LIST_HEAD.prev = node;
	list_prev->next = node;
	node->next = &LIST_HEAD;
	node->prev = list_prev;


	// insert into ready queue
	enqueue_ready(node);
	return 1; // TIDO success
}

void enqueue_ready(struct node_t *node)
{
#ifdef DEBUG
	printf("pid %d enqueue ready\n", node->pid);
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
		printf("pid %d dequeue ready\n", tmp->pid);
#endif
		tmp->total_waiting += get_time() - tmp->start_waiting; // TODO calculate
		tmp->prev_ready->next_ready = tmp->next_ready;
		tmp->next_ready->prev_ready = tmp->prev_ready;
		tmp->prev_ready = NULL;
		tmp->next_ready = NULL;

	} else {
		puts("ready queue empty");
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
			iter->next_ready->prev_ready = iter->prev_ready;
			iter->prev_ready->next_ready = iter->next_ready;
			// TODO free stack
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
