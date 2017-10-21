/*
	10/12/2017
	Authors: Connor Lundberg, Carter Odem
	
	In this project we will be making a simple Round Robin scheduling algorithm
	that will take a single ReadyQueue of PCBs and run them through our scheduler.
	It will simulate the "running" of the process by randomly changing the PC value
	of the process as well as incorporating various interrupts to show the effects
	it has on the scheduling simulator.
	
	This file holds the defined functions declared in the scheduler.h header file.
*/

#include "scheduler.h"

#define MAX_VALUE_PRIVILEGED 15
#define RANDOM_VALUE 101
#define TOTAL_TERMINATED 10

unsigned int sysstack;
int switchCalls;

PCB privileged[4];
int privilege_counter = 0;
int ran_term_num = 0;
int terminated = 0;

/*
	This function is our main loop. It creates a Scheduler object and follows the
	steps a normal Round Robin Sfrtcheduler would to "run" for a certain length of time,
	push new PC onto stack when timer interrupt occurs, then call the ISR, scheduler,
	dispatcher, and eventually an IRET to return to the top of the loop and start
	with the new process.
*/
void timer () {
	unsigned int pc = 0;
	int totalProcesses = 0, firstRun = 1, count = 0, stop = 2;
	Scheduler thisScheduler = schedulerConstructor();
	for (;;) {
		if (totalProcesses >= MAX_PCB_TOTAL) {
			printf("Reached max PCBs, ending Scheduler.\r\n");
			break;
		}
		printf("Beginning of loop\n");
		totalProcesses += makePCBList(thisScheduler);		
		pc = runProcess(pc);
		sysstack = pc;
		terminate(thisScheduler);
		
		if (totalProcesses > 1) {		

			pseudoISR(thisScheduler);
			pc = thisScheduler->running->context->pc;
		}
			

		
		printf("MLFQ state at loop end\n");
		printSchedulerState(thisScheduler);
		/*if (count == stop) {
			break;
		} else {
			count++;
		}*/
		
	}
	schedulerDeconstructor(thisScheduler);
}


/*
	This creates the list of new PCBs for the current loop through. It simulates
	the creation of each PCB, the changing of state to new, enqueueing into the
	list of created PCBs, and moving each of those PCBs into the ready queue.
*/
int makePCBList (Scheduler theScheduler) {
	int newPCBCount = rand() % MAX_PCB_IN_ROUND;
	//int newPCBCount = 1;
	
	int lottery = rand() % 5;
	for (int i = 0; i < newPCBCount; i++) {
		PCB newPCB = PCB_create();
		newPCB->state = STATE_NEW;
		
		// creates privileged pcb
		if (privilege_counter < 4 && lottery == 1) {
			printf("Privileged: ");
			privileged[privilege_counter] = newPCB;
			
			// char *nextPCBState = toStringPCB(privileged[privilege_counter], 0);
			// printf("%s\r\n", nextPCBState);
			// free(nextPCBState);
			privilege_counter++;
		}
		
		
		
		
		q_enqueue(theScheduler->created, newPCB);
		
	}
	
	printf("Making New PCBs: \r\n");
	if (newPCBCount) {
		while (!q_is_empty(theScheduler->created)) {
			PCB nextPCB = q_dequeue(theScheduler->created);
			nextPCB->state = STATE_READY;
			char *nextPCBState = toStringPCB(nextPCB, 0);
			printf("%s\r\n", nextPCBState);
			free(nextPCBState);
			pq_enqueue(theScheduler->ready, nextPCB);
		}
		printf("\r\n");

		if (theScheduler->isNew) {
			char *toRun = toStringPCB(pq_peek(theScheduler->ready), 0);
			printf("Dequeueing PCB %s\r\n\r\n", toRun);
			free(toRun);
			theScheduler->running = pq_dequeue(theScheduler->ready);
			theScheduler->running->state = STATE_RUNNING;
			theScheduler->isNew = 0;
		}
	}
	
	return newPCBCount;
}


/*
	Creates a random number between 3000 and 4000 and adds it to the current PC.
	It then returns that new PC value.
*/
unsigned int runProcess (unsigned int pc) {
	//printf ("last PC: 0x%04X\r\n", pc);
	unsigned int jump = rand() % MAX_PC_JUMP;
	if (jump < MIN_PC_JUMP) jump += ((MIN_PC_JUMP - jump) + (rand() % PC_JUMP_LIMIT));
	pc += jump;
	//printf ("new PC: 0x%04X\r\n", pc);
	return pc;
}

void terminate(Scheduler theScheduler) {
	ran_term_num = rand() % 101;
	
	printf("RAN TERM NUM: %d\n", ran_term_num);
	
	if (theScheduler->running != NULL && ran_term_num <= MAX_VALUE_PRIVILEGED) {
		theScheduler->running->state = STATE_HALT;
		printf("TERMINATING PCB: ");
				
		char *nextPCBState = toStringPCB(theScheduler->running, 0);
		printf("%s\r\n", nextPCBState);
		free(nextPCBState);
		
		
	}
	
}


/*
	This acts as an Interrupt Service Routine, but only for the Timer interrupt.
	It handles changing the running PCB state to Interrupted, moving the running
	PCB to interrupted, saving the PC to the SysStack and calling the scheduler.
*/
void pseudoISR (Scheduler theScheduler) {
	
	if (theScheduler->running->state != STATE_HALT) {
	theScheduler->running->state = STATE_INT;
	theScheduler->interrupted = theScheduler->running;
	theScheduler->running->context->pc = sysstack;
	}

	/*char *toInt = toStringPCB(theScheduler->interrupted, 0);
	printf("New Interrupted PCB %s\r\n", toInt);
	free(toInt);*/
	scheduling(IS_TIMER, theScheduler);
	pseudoIRET(theScheduler);
}


void printSchedulerState (Scheduler theScheduler) {
	char *queueState = toStringPriorityQueue(theScheduler->ready, 0);
	printf("%s", queueState);
	free(queueState);
	if (theScheduler->running && theScheduler->interrupted) {
		char *runningState = toStringPCB(theScheduler->running, 0);
		char *interruptedState = toStringPCB(theScheduler->interrupted, 0);
		/*printf("Running %s\r\n", runningState);
		printf("Interrupted %s\r\n", interruptedState);*/
		free(runningState);
		free(interruptedState);
	}
	printf("\r\n");
	
	
	if (pq_peek(theScheduler->ready)) {
		char *toRun = toStringPCB(pq_peek(theScheduler->ready), 0);
		printf("Next highest priority PCB %s\r\n", toRun);
		free(toRun);
	} else {
		printf("Next highest priority PCB contents: The MLFQ is empty!\r\n");
	}
	
	char *running = toStringPCB(theScheduler->running, 0);
	printf("Going to be running next: %s\r\n\r\n\r\n", running);
	free(running);
}


/*
	If the interrupt that occurs was a Timer interrupt, it will simply set the 
	interrupted PCBs state to Ready and enqueue it into the Ready queue. It then
	calls the dispatcher to get the next PCB in the queue.
*/
void scheduling (int isTimer, Scheduler theScheduler) {
	if (isTimer && theScheduler->running->state != STATE_HALT) {
		theScheduler->interrupted->state = STATE_READY;
		if (theScheduler->interrupted->priority < (NUM_PRIORITIES - 1)) {
			theScheduler->interrupted->priority++;
		}
		pq_enqueue(theScheduler->ready, theScheduler->interrupted);
		
		/*printf("MLFQ After first loop through\n");
		char *queue = toStringPriorityQueue(theScheduler->ready, 0);
		printf("%s\n", queue);
		free(queue);*/
		//exit(0);
	}
	
	

	if (theScheduler->running->state == STATE_HALT) {
		q_enqueue(theScheduler->killed, theScheduler->running);
		theScheduler->running = NULL;
		
		terminated++;
		
		printf("Killed: ");
		toStringReadyQueue(theScheduler->killed, 0);

	}
	
	// working state (switched from dequeue to peek);
	theScheduler->running = pq_peek(theScheduler->ready);
	
	dispatcher(theScheduler);

	
	
	
	if (terminated >= TOTAL_TERMINATED) {
		while(!q_is_empty(theScheduler->killed)) {
			PCB_destroy(q_dequeue(theScheduler->killed));
		}
	}
	
	

	
	
}


/*
	This simply gets the next ready PCB from the Ready queue and moves it into the
	running state of the Scheduler.
*/
void dispatcher (Scheduler theScheduler) {
	
	if (pq_peek(theScheduler->ready)->state != STATE_HALT) {
		theScheduler->running = pq_dequeue(theScheduler->ready);
		theScheduler->running->state = STATE_RUNNING;
	}

	/*char *toRun = toStringPCB(theScheduler->running, 0);
	printf("New Running PCB %s\r\n", toRun);
	free(toRun);*/
}


/*
	This simply sets the running PCB's PC to the value in the SysStack;
*/
void pseudoIRET (Scheduler theScheduler) {
	theScheduler->running->context->pc = sysstack;
}


/*
	This will construct the Scheduler, along with its numerous ReadyQueues and
	important PCBs.
*/
Scheduler schedulerConstructor () {
	Scheduler newScheduler = (Scheduler) malloc (sizeof(scheduler_s));
	newScheduler->created = q_create();
	newScheduler->killed = q_create();
	newScheduler->blocked = q_create();
	newScheduler->ready = pq_create();
	newScheduler->running = NULL;
	newScheduler->interrupted = NULL;
	newScheduler->isNew = 1;
	
	return newScheduler;
}


/*
	This will do the opposite of the constructor with the exception of 
	the interrupted PCB which checks for equivalancy of it and the running
	PCB to see if they are pointing to the same freed process (so the program
	doesn't crash).
*/
void schedulerDeconstructor (Scheduler theScheduler) {
	q_destroy(theScheduler->created);
	q_destroy(theScheduler->killed);
	q_destroy(theScheduler->blocked);
	pq_destroy(theScheduler->ready);
	PCB_destroy(theScheduler->running);
	if (theScheduler->interrupted == theScheduler->running) {
		PCB_destroy(theScheduler->interrupted);
	}
	free (theScheduler);
}

int isPrivileged(PCB pcb) {
	if (pcb != NULL) {
		for (int i = 0; i < 4; i++) {
			if (privileged[i] == pcb) {
				return 1;
				break;
			}	
		}
	}
	
	
	for (int j = 0; j < 4; j++) {
		char *nextPCBState = toStringPCB(privileged[privilege_counter], 0);
			printf("%s\r\n", nextPCBState);
			free(nextPCBState);
	}
	

	
	return 0;	
}


void main () {
	setvbuf(stdout, NULL, _IONBF, 0);
	time_t t;
	srand((unsigned) time(&t));
	sysstack = 0;
	switchCalls = 0;
	timer();
}