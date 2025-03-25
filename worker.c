#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>

#define SHM_KEY 854038
#define MSG_KEY 864049
#define NANO_TO_SEC 1000000000

// Author: Dat Nguyen
// Date: 03/24/2025
// worker.c is a program that is called as a child process from oss.c. It prints info every second about the PID, PPID, the workers lifetime, and info about simulatedClock.

// SimulatedClock structure that will be used as our system clock required in project.
typedef struct SimulatedClock {
	int seconds;
	int nanoseconds;
} SimulatedClock;

typedef struct ossMSG { // Structure to communicate with OSS
	long mtype;
	int msg;
} ossMSG;

int main(int argc, char** argv) {
	
	// Argument checking, this should not be true, if true, something is wrong in oss.c
	if (argc < 3) {
		printf("Error: two arguments expected \n");
		exit(1);
	}

	int secLimit = atoi(argv[1]);
	int nanoLimit = atoi(argv[2]);

	// Attach to the shared memory (created by oss.c)
    	int shmid = shmget(SHM_KEY, sizeof(SimulatedClock), 0666);
    	if (shmid == -1) {
        	printf("Error: attachment failed \n");
        	exit(1);
	}
	
	// Pointer to shared memory, getting access to SimulatedClock that oss.c has.
	SimulatedClock* clockSHM = (SimulatedClock*) shmat(shmid, NULL, 0);
    	if (clockSHM == (void*)-1) {
		printf("Error: pointing to shm failed. \n");
        	exit(1);
	}

	// Get the message queue (created by oss.c)
    	int msgqid = msgget(MSG_KEY, 0666);
    	if (msgqid == -1) {
        	printf("Error: worker msgget failed\n");
        	exit(1);
	}

	// time is a SimulatedClock type that holds the second and nanosecond from the SimulatedClock from oss.c
	SimulatedClock time;
	time.seconds = clockSHM->seconds;
	time.nanoseconds = clockSHM->nanoseconds;

	// lifeSec and targ is the lifetime of the worker, 
	int lifeSec = time.seconds + secLimit;
	int lifeNano = time.nanoseconds + nanoLimit;

	// In case during the addition of nanoseconds, the sum is more than or equal to a second.
	if (lifeNano >= NANO_TO_SEC) {
		lifeSec++;
		lifeNano -= NANO_TO_SEC;
	}
	
	// Initial print
	printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --Just Starting\n", getpid(), getppid(), time.seconds, time.nanoseconds, lifeSec, lifeNano );
	
	// Variables in main loop. 
	int iteration = 1;
	ossMSG msg, receiveMsg;

	do { // Getting message from oss, main loop.
		if (msgrcv(msgqid, &receiveMsg, sizeof(receiveMsg.msg), getpid(), 0) == -1) {
			printf("Error: worker msgrcv failed. \n");
			exit(1);
		}

		int currentSec = clockSHM->seconds;
		int currentNano = clockSHM->nanoseconds;

		int terminationStatus = 0; // Indicator if its time for worker to terminate
		if ((currentSec > lifeSec) || ((currentSec == lifeSec) && (currentNano >= lifeNano))) { // Check if termination status is reached
			terminationStatus = 1;
		}

		if (!terminationStatus) { // Main printing loop		
			if (iteration == 1) {
				printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --%d iteration have passed since it started\n", getpid(), getppid(), clockSHM->seconds, clockSHM->nanoseconds, lifeSec, lifeNano, iteration);
			}
			else {
				printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --%d iterations have passed since it started\n", getpid(), getppid(), clockSHM->seconds, clockSHM->nanoseconds, lifeSec, lifeNano, iteration);
			}
		}
		else { // Print final message before sending message and terminating. 
			if (iteration == 1) {
                                printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --Terminating after sending message back to oss after %d iteration\n", getpid(), getppid(), clockSHM->seconds, clockSHM->nanoseconds, lifeSec, lifeNano, iteration);
                        }
			else {
				printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --Terminating after sending message back to oss after %d iteration\n", getpid(), getppid(), clockSHM->seconds, clockSHM->nanoseconds, lifeSec, lifeNano, iteration);
			}
		}
		
		// Prepping to send message
		msg.mtype = getpid();
		if (terminationStatus) {
			msg.msg = 0;
		}
		else {
			msg.msg = 1;
		}
		
		// Sending message
		if (msgsnd(msgqid, &msg, sizeof(msg.msg), 0) == -1) {
			printf("Error: worker msgsnd failed \n.");
			exit(1);
		}
		
		// Increase iterations
		iteration++;

		// Terminate if lifespan reached.
		if (terminationStatus) {
			break;
		}
	
	} while(1);
	// Detach shared memory
    	if (shmdt(clockSHM) == -1) {
        	perror("Error: worker Detached Failed \n");
		exit(1);
	}
	return 0;	
}
