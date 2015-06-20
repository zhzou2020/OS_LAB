
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	//disp_str("scd");
	PROCESS* p;
	int	 greatest_ticks = 0;
//disp_str("s");disp_int((proc_table+1)->ticks);
	//disp_str("s");
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS; p++) {
			
			//disp_int(p->ticks);
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table+NR_TASKS; p++) {
				if(p->ticks!=-1)
					p->ticks = p->priority;
			}
		}

	//	disp_str("a");
	}
	p_proc_ready->ticks--;
	//disp_str("out");
	for (p = proc_table; p < proc_table+NR_TASKS; p++) {
		if(p->ticks==-1){
			if(((get_ticks() - p->delay_start_ticks ) * 1000 / HZ) > p->delay_ticks){
				p->ticks=p->old_ticks;
				//disp_int(i);
			}
		}
	}
	
	//disp_str("!!!");
	//disp_int(p_proc_ready-proc_table);
	

	//restart();
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_process_sleep(int milli_sec)
{
	PROCESS* p=p_proc_ready;
	if(p->ticks==-1)
		return;

	int t = get_ticks();	
	p->old_ticks=p->ticks;
	p->ticks=-1;
	p->delay_start_ticks=t;
	p->delay_ticks=milli_sec;	
	
	//disp_str("slk");
	//disp_int(p-proc_table);
	schedule();
}

PUBLIC void sys_disp_str(char* str)
{
	disp_str(str);
}

PUBLIC void sys_sem_p(Semaphore *sem)
{
	sem->value--;
	//disp_int(sem->value);
	if(sem->value<0)
	{

		sem->q.arr[sem->q.tail%MAX_SIZE]=p_proc_ready-proc_table;
		sem->q.tail++;
		//disp_str("p");
		//disp_int(p_proc_ready-proc_table);
		sys_process_sleep(MAX_TIME);
		//disp_str("sleep");
		//disp_int(sem->q->arr[(sem->q->index-1)%MAX_SIZE]);
	}

	//schedule();
}

PUBLIC void sys_sem_v(Semaphore *sem)
{
	sem->value++;
	//disp_int(sem->value);
	if(sem->value<=0)
	{
		//disp_str("index");
		//disp_int(sem->q->index);
		//disp_str("v");
		//disp_int(sem->q.index);
		//disp_int(sem->q.arr[sem->q.index%MAX_SIZE]);
		PROCESS *p=proc_table+sem->q.arr[sem->q.index%MAX_SIZE];
		//disp_str("1");
		p->ticks=p->old_ticks;
		//disp_str("2");
		sem->q.index++;
		
		//disp_str("ready");
		//disp_int(sem->q.arr[sem->q.index%MAX_SIZE]);
	}

	schedule();
}
