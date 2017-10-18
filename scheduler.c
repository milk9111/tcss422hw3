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

unsigned int sysstack;
int switchCalls;

/*
	This function is our main loop. It creates a Scheduler object and follows the
	steps a normal Round Robin Scheduler would to "run" for a certain length of time,
	push new PC onto stack when timer interrupt occurs, then call the ISR, scheduler,
	dispatcher, and eventually an IRET to return to the top of the loop and start
	with the new process.
*/
void timer () {
	unsigned int pc = 0;
	int totalProcesses = 0;
	Scheduler thisScheduler = schedulerConstructor();
	for (;;) {
		if (totalProcesses >= MAX_PCB_TOTAL) {
			printf("Reached max PCBs, ending Scheduler.\r\n");
			break;
		}
		totalProcesses += makePCBList(thisScheduler);
		pc = runProcess(pc);
		sysstack = pc;
		pseudoISR(thisScheduler);
		pc = thisScheduler->running->context->pc;
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
	for (int i = 0; i < newPCBCount; i++) {
		PCB newPCB = PCB_create();
		newPCB->state = STATE_NEW;
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
			q_enqueue(theScheduler->ready, nextPCB);
		}
		printf("\r\n");
		if (theScheduler->isNew) {
			theScheduler->running = q_dequeue(theScheduler->ready);
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
	unsigned int jump = rand() % MAX_PC_JUMP;
	if (jump < MIN_PC_JUMP) jump += ((MIN_PC_JUMP - jump) + (rand() % PC_JUMP_LIMIT));
	pc += jump;
	
	return pc;
}


/*
	This acts as an Interrupt Service Routine, but only for the Timer interrupt.
	It handles changing the running PCB state to Interrupted, moving the running
	PCB to interrupted, saving the PC to the SysStack and calling the scheduler.
*/
void pseudoISR (Scheduler theScheduler) {
	theScheduler->running->state = STATE_INT;
	theScheduler->interrupted = theScheduler->running;
	theScheduler->running->context->pc = sysstack;
	scheduling(IS_TIMER, theScheduler);
	pseudoIRET(theScheduler);
}


/*
	If the interrupt that occurs was a Timer interrupt, it will simply set the 
	interrupted PCBs state to Ready and enqueue it into the Ready queue. It then
	calls the dispatcher to get the next PCB in the queue.
*/
void scheduling (int isTimer, Scheduler theScheduler) {
	if (isTimer) {
		theScheduler->interrupted->state = STATE_READY;
		q_enqueue(theScheduler->ready, theScheduler->interrupted);
	}
	if (switchCalls != (SWITCH_CALLS - 1)) {
		switchCalls++;
	}
	
	if (switchCalls == (SWITCH_CALLS - 1)) {
		char *runningPCBState = toStringPCB(theScheduler->running, 0);
		printf("Before switching:\r\n");
		printf("%s\r\n", runningPCBState);
		free(runningPCBState);
		printf("Switching to:\r\n");
		char *nextPCBState = toStringPCB(q_peek(theScheduler->ready), 0);
		printf("%s\r\n\r\n", nextPCBState);
		free(nextPCBState);
	}
	
	dispatcher(theScheduler);
	
	if (switchCalls == (SWITCH_CALLS - 1)) {
		printf("After switching:\r\n");
		char *runningPCBState = toStringPCB(theScheduler->running, 0);
		printf("%s\r\n", runningPCBState);
		free(runningPCBState);
		char *interruptedPCBState = toStringPCB(theScheduler->interrupted, 0);
		printf("%s\r\n\r\n", interruptedPCBState);
		free(interruptedPCBState);
		char *queueState = toStringReadyQueue(theScheduler->ready, 0);
		printf("%s\r\n\r\n", queueState);
		free(queueState);
		switchCalls = 0;
	} 
}


/*
	This simply gets the next ready PCB from the Ready queue and moves it into the
	running state of the Scheduler.
*/
void dispatcher (Scheduler theScheduler) {
	theScheduler->running = q_dequeue(theScheduler->ready);
	theScheduler->running->state = STATE_RUNNING;
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
	newScheduler->ready = q_create();
	newScheduler->running = PCB_create();
	newScheduler->interrupted = PCB_create();
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
	q_destroy(theScheduler->ready);
	PCB_destroy(theScheduler->running);
	if (theScheduler->interrupted == theScheduler->running) {
		PCB_destroy(theScheduler->interrupted);
	}
	free (theScheduler);
}


void main () {
	setvbuf(stdout, NULL, _IONBF, 0);
	time_t t;
	srand((unsigned) time(&t));
	sysstack = 0;
	switchCalls = 0;
	timer();
}