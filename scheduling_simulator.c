#include "scheduling_simulator.h"
#define DEBUG

int main()
{
	signal(SIGTSTP, ctrl_z_handler);
	list_init();
	shell_mode();
	while(1);
	return 0;
}

void ctrl_z_handler(int n)
{
}

// print error messange and consume all other input
void invalid(const char* err_input, const char* type)
{
	printf("%s: %s not found\n", err_input, type);
	while(getchar() != '\n');
}

void print_all()
{
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
		printf("%d %s %-17s %d\n", tmp->pid, tmp->task_name, state, tmp->quantum_time);
		tmp = tmp->next;
	}
}

void shell_mode()
{
	while(1) {
		printf("$ ");
		char input[100];
		scanf("%s", input);
		if(!strcmp(input, "add")) {
			char task_name[20];
			scanf("%s", task_name);

			// get option
			scanf("%s", input);
			if(strcmp(input, "-t")) {
				invalid(input, "option");
				continue;
			}

			// get option argument
			int quantum_time = 10;
			scanf("%s", input);
			if(!strcmp(input, "S") || !strcmp(input, "L")) {
				if(input[0] == 'L')
					quantum_time = 20;
			} else {
				invalid(input, "argument");
				continue;
			}
			int ret_val = enqueue(task_name, quantum_time);
			if(ret_val == -1) {
				invalid(task_name, "task");
				continue;
			}

		} else if(!strcmp(input, "remove")) {
			int n, pid;
			n = scanf("%d", &pid);
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

		} else if(!strcmp(input, "ps")) {
			print_all();
		} else if(!strcmp(input, "start")) {

		} else {
			invalid(input, "command");
			continue;
		}
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

	puts("before malloc");
	// allocate memory
	struct node_t *node = (struct node_t*)malloc(sizeof(struct node_t));

	puts("before init");
	// init
	node->pid = ++pid;
	node->state = TASK_READY;
	node->quantum_time = quantum_time;
	strcpy(node->task_name, task_name);
	node->total_waiting = 0;
	node->start_waiting = 0;
	node->prev_ready = NULL;
	node->next_ready = NULL;

	// push back to LIST
	puts("before insert queue");
	struct node_t *list_prev = LIST_HEAD.prev;
	LIST_HEAD.prev = node;
	list_prev->next = node;
	node->next = &LIST_HEAD;
	node->prev = list_prev;


	puts("before insert ready ueue");
	// insert into ready queue
	enqueue_ready(node);
	return 1; // TIDO success
}

void enqueue_ready(struct node_t *node)
{
	puts("in enqueue ready");
	// save timestamp
	if(node)
		node->start_waiting = 0; //TODO = now
	else
		throw_unexpected("added NULL to ready queue");

	puts("after enqueue ready if");

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
		tmp->total_waiting += 0; // TODO calculate
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

void list_init()
{
	LIST_HEAD.prev = &LIST_HEAD;
	LIST_HEAD.next = &LIST_HEAD;
	READY_HEAD.prev_ready = &READY_HEAD;
	READY_HEAD.next_ready = &READY_HEAD;
}

void throw_unexpected(const char *err_msg)
{
	fprintf(stderr, "%s\n", err_msg);
	exit(1);
}
