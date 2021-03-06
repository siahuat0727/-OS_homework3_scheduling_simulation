#include "task.h"
//#define DEMO

void task1(void)   // may terminated
{
#ifdef DEMO
	my_puts("I'm task1");
#endif

	unsigned int a = ~0;

	while (a != 0) {
		a -= 1;
	}
}

void task2(void) // run infinite
{
#ifdef DEMO
	my_puts("I'm task2");
#endif

	unsigned int a = 0;

	while (1) {
		a = a + 1;
	}
}

void task3(void) // wait infinite
{
#ifdef DEMO
	my_printf("pid %d I'm task3\n", RUNNING_TASK->pid);
#endif

	hw_suspend(32768);
#ifdef DEMO
	fprintf(stdout, "pid %d ", RUNNING_TASK->pid);
#endif

	fprintf(stdout, "task3: good morning~\n");
	fflush(stdout);
}

void task4(void) // sleep 5s
{
#ifdef DEMO
	my_puts("I'm task4");
#endif

	hw_suspend(500);
	fprintf(stdout, "task4: good morning~\n");
	fflush(stdout);
}

void task5(void)
{
#ifdef DEMO
	my_puts("I'm task5");
#endif

	int pid = hw_task_create("task3");

	hw_suspend(1000);
	fprintf(stdout, "task5: good morning~\n");
	fflush(stdout);

	hw_wakeup_pid(pid);
	fprintf(stdout, "Mom(task5): wake up pid %d~\n", pid);
	fflush(stdout);
}

void task6(void)
{
#ifdef DEMO
	my_puts("I'm task6");
#endif
	for (int num = 0; num < 5; ++num) {
		hw_task_create("task3");
	}

	hw_suspend(1000);
	fprintf(stdout, "task6: good morning~\n");
	fflush(stdout);

	int num_wake_up = hw_wakeup_taskname("task3");
	fprintf(stdout, "Mom(task6): wake up task3=%d~\n", num_wake_up);
	fflush(stdout);
}
