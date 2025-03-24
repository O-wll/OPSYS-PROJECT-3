#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>

#define SHM_KEY 854038
#define NANO_TO_SEC 1000000000

// Author: Dat Nguyen
// Date: 03/04/2025
// worker.c is a program that is called as a child process from oss.c. It prints info every second about the PID, PPID, the workers lifetime, and info about simulatedClock.

// SimulatedClock structure that will be used as our system clock required in project.
typedef struct SimulatedClock {
	int seconds;
	int nanoseconds;
} SimulatedClock;

int main(int argc, char** argv) {
	
	// Argument checking, this should not be true, if true, something is wrong in oss.c
	if (argc < 3) {
		printf("Error: two arguments expected \n");
		exit(1);
	}

	int secLimit = atoi(argv[1]);
	int nanoLimit = atoi(argv[1]);

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
	int lastTime = time.seconds; 
	int secPassed = 0;

	while(1) {
		// Keeping track of the time passed.
		int currentSec = clockSHM->seconds;
		int currentNano = clockSHM->nanoseconds;

		// If current sec, the simulated clock, is > than lifeSec, the lifetime of the worker, then terminate. Secondary condition in case first isn't fulfilled but still over lifetime.
		if ((currentSec > lifeSec) || ((currentSec >= lifeSec) && currentNano >= lifeNano)) {
			break;
		}

		if (currentSec != lastTime) { // If simulated clock changes, print this message.
			secPassed = currentSec - time.seconds;
			printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --%d seconds have passed\n", getpid(), getppid(), currentSec, currentNano, lifeSec, lifeNano, secPassed);
			lastTime = currentSec; // Updating lastTime for the next print message.
		}
	}

	// Printing final message after termination
	printf("\nWORKER PID: %d PPID: %d SysClockS: %d SysClockNano: %d TermTimeS: %d TermTimeNano: %d --Termination\n", getpid(), getppid(), clockSHM->seconds, clockSHM->nanoseconds, lifeSec, lifeNano);

	// Detach shared memory
    	if (shmdt(clockSHM) == -1) {
        	perror("Error: Detached Failed \n");
        	exit(1);
    	}

	return 0;	
}
