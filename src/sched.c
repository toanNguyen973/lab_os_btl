
#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

extern int time_slot; //Lấy giá trị quantum time trong file os.c

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif


int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++){
		mlq_ready_queue[i].size = 0;
		mlq_ready_queue[i].time_slot = MAX_PRIO - i; // slot = Max_prio - level_queue.
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
		unsigned long curr_prio = 0; //prio cua queue hien tai
		// unsigned long curr_slot = MAX_PRIO - curr_prio; //non usefull
	while(curr_prio < MAX_PRIO){
		if(empty(&mlq_ready_queue[curr_prio]) != 0) break;
		if(mlq_ready_queue[curr_prio] <= 0){
			curr_prio++;
			continue;
		} 
		// get proc 
		pthread_mutex_lock(&queue_lock);
		proc = mlq_ready_queue[curr_prio];
		mlq_ready_queue[curr_prio].time_slot -= time_slot; // decrease slot
		pthread_mutex_unlock(&queue_lock);
		if(proc != NULL) return proc;
		curr_prio++;
	}
	if(proc == NULL && !empty(&mlq_ready_queue[curr_prio])) {
		pthread_mutex_lock(&queue_lock);
		for(int i = 0; i < MAX_PRIO; i++) reset_slot(&mlq_ready_queue[i], MAX_PRIO - i);
		pthread_mutex_unlock(&queue_lock);
		
		curr_prio = 0;
		while(curr_prio < MAX_PRIO){
			if(empty(&mlq_ready_queue[curr_prio])) break;
			if(mlq_ready_queue[curr_prio] <= 0){
				curr_prio++;
				continue;
			}	 
			// get proc 
			//pthread_mutex_lock(&queue_lock);
			pthread_mutex_lock(&queue_lock);
			proc = mlq_ready_queue[curr_prio];
			mlq_ready_queue[curr_prio].time_slot -= time_slot; // decrease slot
			pthread_mutex_unlock(&queue_lock);
			//pthread_mutex_unlock(&queue_lock);
			if(proc != NULL) return proc;
			curr_prio++;
		}
	}
	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	return add_mlq_proc(proc);
}
#else
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	
	if(!empty(&ready_queue)){
		pthread_mutex_lock(&queue_lock);
		proc = dequeue(&ready_queue);
		pthread_mutex_unlock(&queue_lock);
	}
	return proc;
}

void put_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif


