
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

int waiting;
int chairs;
Semaphore customers,barbers,mutex;

void init(Semaphore *s,int value);
void cuthair();
void customer_come(int i);
void customer_wait(int i);
void get_haircut(int i);
void customer_leave(int i);
void end();

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 15;
	proc_table[1].ticks = proc_table[1].priority = 15;
	proc_table[2].ticks = proc_table[2].priority = 15;
	proc_table[3].ticks = proc_table[3].priority = 15;
	proc_table[4].ticks = proc_table[4].priority = 15;

	waiting=0;
	chairs=1;
	init(&customers,0);
	init(&barbers,0);
	init(&mutex,1);

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	restart();

	while(1){}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1) {
		disp_color_str("Normal!",BRIGHT | MAKE_COLOR(BLACK,RED));
		milli_delay(100);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		//disp_color_str("Barber!",BRIGHT | MAKE_COLOR(BLACK,GREEN));
		disp_color_str("Sleep!",BRIGHT | MAKE_COLOR(BLACK,GREEN));
		//disp_int(barbers.value);
		//disp_str("p");		
		sys_sem_p(&customers);
		disp_color_str("Wake up!",BRIGHT | MAKE_COLOR(BLACK,GREEN));
		//disp_str("m");		
		sys_sem_p(&mutex);
		waiting--;
		//disp_str("+baber");
		sys_sem_v(&barbers);
		sys_sem_v(&mutex);
		//cut hair
		cuthair();
	}
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	int i = 0x2000;

	sys_sem_p(&mutex);
	customer_come(1);
	//disp_int(waiting);
	//disp_int(chairs);
	if(waiting<chairs)
	{
		waiting++;
		sys_sem_v(&customers);
		//disp_str("cus");
		//disp_int(customers.value);
		sys_sem_v(&mutex);
		//customer_wait(1);
		//disp_str("baber");
		sys_sem_p(&barbers);	
		get_haircut(1);
	}
	else
	{
		disp_color_str("I am angry!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
		sys_sem_v(&mutex);
	}

	customer_leave(1);
}

/*======================================================================*
                               TestD
 *======================================================================*/
void TestD()
{
	int i = 0x3000;
	
	sys_sem_p(&mutex);
	customer_come(2);
	if(waiting<chairs)
	{
		waiting++;
		sys_sem_v(&customers);
		//disp_str("cus");
		//disp_int(customers.value);
		sys_sem_v(&mutex);
		//customer_wait(2);
		//disp_str("baber");
		sys_sem_p(&barbers);	
		get_haircut(2);
	}
	else
	{
		disp_color_str("I am angry!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
		sys_sem_v(&mutex);
	}

	customer_leave(2);
}

/*======================================================================*
                               TestE
 *======================================================================*/
void TestE()
{
	int i = 0x4000;
	
	sys_sem_p(&mutex);
	customer_come(3);
	if(waiting<chairs)
	{
		waiting++;
		sys_sem_v(&customers);
		//disp_str("cus");
		//disp_int(customers.value);
		sys_sem_v(&mutex);
		//customer_wait(3);
		//disp_str("E");
		//disp_str("baber");
		sys_sem_p(&barbers);		
		//disp_str("E");
		get_haircut(3);
	}
	else
	{
		disp_color_str("I am angry!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
		sys_sem_v(&mutex);
	}

	customer_leave(3);
}

void init(Semaphore* s,int v)
{
	s->value=v;
	
	s->q.index=0;
	s->q.tail=0;
	memset(s->q.arr,0,sizeof(s->q.arr));
}

void cuthair()
{
	disp_color_str("Cut hair!",BRIGHT | MAKE_COLOR(BLACK,GREEN));
	milli_delay(20);
}

void customer_come(int i)
{
	disp_color_str("Customer ",BRIGHT | MAKE_COLOR(BLACK,BLUE));
	disp_int(i);
	disp_color_str(" comes!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
}

void customer_wait(int i)
{
	disp_color_str("Customer ",BRIGHT | MAKE_COLOR(BLACK,BLUE));
	disp_int(i);
	disp_color_str(" waits!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
}

void get_haircut(int i)
{
	disp_color_str("Customer ",BRIGHT | MAKE_COLOR(BLACK,BLUE));
	disp_int(i);
	disp_color_str(" gets hair cut!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
}

void customer_leave(int i)
{	
	disp_color_str("Customer ",BRIGHT | MAKE_COLOR(BLACK,BLUE));
	disp_int(i);
	disp_color_str(" leaves!",BRIGHT | MAKE_COLOR(BLACK,BLUE));
	sys_process_sleep(MAX_TIME);
}
