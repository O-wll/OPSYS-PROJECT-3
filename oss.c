#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h> // For shared memory
#include <sys/ipc.h> // Also for shared memory, allows worker class to access shared memory
#include <time.h>

#define SHM_KEY 854038
#define NANO_TO_SEC 1000000000 

// Author: Dat Nguyen
// 03/4/2025
// oss.c is the main program that creates shared memory and have the SimulatedClock knowledge, Program explained in ./oss -h

// Using a structure for our simulated clock, storing seconds and nanoseconds.
typedef struct SimulatedClock {
        int seconds;
        int nanoseconds;
} SimulatedClock;


// Structure for our PCB table
struct PCB {
	int occupied;
	pid_t pid;
	int startSeconds;
	int startNano;
};
struct PCB processTable[20];

void incrementClock(SimulatedClock *clock);
void signalHandler(int sig, int shmid, SimulatedClock* clock);
void printTable(SimulatedClock *clock);
void help();

int main(int argc, char **argv) {
	int userInput = 0;
	int childProcess = 3;
	int simul = childProcess;
	int timeLimit = 5;
	int interval = 100;
	int totalProc = 0;
	int currentProc = 0;
	int status;
	pid_t pid;
	// These variables are specifically for our table.
	int checkSec = 0;
	int checkNano = 0;
	// Variables for Interval
	int lastLaunchSec = 0;
	int lastLaunchNano = 0;

	// User Input handler
	while ((userInput = getopt(argc, argv, "n:s:t:i:h")) != -1) {
		switch(userInput) {
			case 'n': // How many child processes to launch.
				childProcess = atoi(optarg);
				if (childProcess <= 0) {
					printf("Error: Total child processes must be at least one. \n");
					exit(1);
				}
				break;
			case 's': // How many simulations to run at once.
				simul = atoi(optarg);
				// Ensuring simulations isn't zero or below for the program to work.
				if (simul < 0) {
					printf("Error: Simulations must be positive. \n");
					exit(1);
				}	
				break;
			case 't': // Upper bound of when calling worker process
				timeLimit = atoi(optarg);
				if (timeLimit <= 0) {
                                        printf("Error: Time limit must be above one. \n");
                                        exit(1);
                                }
                                break;

			case 'i': // How often to launch child interval
				interval = atoi(optarg);
				if (childProcess <= 0) {
					printf("Error: interval must be positive. \n");
                                        exit(1);
                                }
                                break;
			case 'h': // Prints out help function.
				help();
				return 0;
			case '?': // Invalid user argument handling.
				printf("Error: Invalid argument detected \n");
				printf("Usage: ./oss.c -h to learn how to use this program \n");
				exit(1);
		}
	}


	int shmid = shmget(SHM_KEY, sizeof(SimulatedClock), IPC_CREAT | 0666); // Creating shared memory using shmget. 
    	
	if (shmid == -1) { // If shmid is -1 as a result of shmget failing and returning -1, error message will print.
        	printf("Error: shmget failed. \n");
        	exit(1);
    	}
	
	SimulatedClock *clock = (SimulatedClock *)shmat(shmid, NULL, 0); // Attach shared memory, clock is now a pointer to SimulatedClock structure.
	if (clock == (void *)-1) { // if shmat, the attaching shared memory function, fails, it returns an invalid memory address.
        	printf("Error: shared memory attachment failed. \n");
        	exit(1);
    	}

    	// Initialize clock
    	clock->seconds = 0;
    	clock->nanoseconds = 0;

	// Initialize table
	for (int i = 0; i < 20; i++) {
		processTable[i].occupied = 0;
        	processTable[i].pid = 0;
        	processTable[i].startSeconds = 0;
       		processTable[i].startNano = 0;
	}

	// Start the timer for 60 real seconds.
	signal(SIGALRM, (void(*)(int))signalHandler);
	alarm(60);

	srand(time(NULL));

	while(1) { // OSS loop
		incrementClock(clock);

		pid_t killedPid = waitpid(-1, &status, WNOHANG); // Non blocking check for terminated child.

		while (killedPid > 0) { // If there is a killed pid, make the PCB slot it occupies free.
			for(int i = 0; i < 20; i++) {
				if (processTable[i].occupied && processTable[i].pid == killedPid) {
					processTable[i].occupied = 0;
					currentProc--; // Decreasing to indicate that a process has been killed.
				}
			}

			killedPid = waitpid(-1, &status, WNOHANG);
		}
		
		// When dealing with something as big as nano seconds (1 * 10^9), you should use long.
		long currentTimeNano = (long)clock->seconds * NANO_TO_SEC + clock->nanoseconds;
        	long lastPrintNano = (long)checkSec * NANO_TO_SEC + checkNano;
		
		// Check to see if the difference between the current time and the time last print >= 0.5 seconds which is 5 followed by 9 zeros in nanoseconds.
		if (currentTimeNano - lastPrintNano >= 500000000) {
			printTable(clock);
            		checkSec = clock->seconds;
            		checkNano = clock->nanoseconds;
        	}
		// Child launching process, ensure that we have not reached total processes ran and that we are not exceeding sim.
                if ((totalProc < childProcess) && (currentProc < simul)) {
                        // Check to see if time since last child process launch exceeds interval
                        long lastLaunchTimeNano = (long)lastLaunchSec * NANO_TO_SEC + lastLaunchNano;
                        if ((currentTimeNano - lastLaunchTimeNano) >= (long)interval * 1000000 ) {
				// Find empty slot in PCB
				int pcbIndex = -1;
				for (int i = 0; i < 20; i++) {
					if (!processTable[i].occupied) {
						pcbIndex = i;
						break;
					}
				}
				if (pcbIndex != -1) { // If found free PCB slot
                                	pid = fork();

					if (pid < 0) { // If fork fails
						printf("Error: Fork Failed");
						exit(1);
					}
					else if (pid == 0) {
                                        	// We are initializing random inputs for ./worker which takes in sec and nanosec
                                        	int randomSec = (rand() % timeLimit) + 1;
                                        	int randomNano = rand() % NANO_TO_SEC;

                                        	// Converting our inputs for ./worker to strings since that's what the argument will be expecting.
                                        	char secStr[20];
                                        	char nanoStr[20];
                                        	// snprintf is safer than sprintf when dealing with large values and possible overflow.
                                        	snprintf(secStr, sizeof(secStr), "%d", randomSec);
                                        	snprintf(nanoStr, sizeof(nanoStr), "%d", randomNano);

                                        	// Execute child function
                                        	execl("./worker", "worker", secStr, nanoStr, (char *)NULL);
					}
					else {
					       	// Update process table
                                        	processTable[pcbIndex].occupied = 1;
                                        	processTable[pcbIndex].pid = pid;
                                        	processTable[pcbIndex].startSeconds = clock->seconds;
                                        	processTable[pcbIndex].startNano = clock->nanoseconds;
                                        	currentProc++;
                                       		totalProc++;

                                        	// Update last launch time
                                        	lastLaunchSec = clock->seconds;
						lastLaunchNano = clock->nanoseconds;
					}
				}
			}
		}
		// Check if launched all child process and that there are no currently running processes.
		if ((totalProc == childProcess) && (currentProc ==0)) {
			break;
		}
	}

	// Detach shared memory
    	if (shmdt(clock) == -1) {
        	printf("Error: Shared memory detachment failed");
		exit(1);
    	}	

    	// Remove shared memory
    	if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        	printf("Error: Removing memory failed");
		exit(1);
    	}
	
	return 0;
}

void incrementClock(SimulatedClock *clock) { // This function simulates the increment of our simulated clock.
	clock->nanoseconds += 100000; // Start increasing in milliseconds

	while (clock->nanoseconds >= NANO_TO_SEC) { // Expect even after reducing nano seconds to have a bit of remaining nano seconds, while ensures that if they build up, that they'll be reduced properly.
		clock->seconds++;
		clock->nanoseconds -= NANO_TO_SEC;	
	}
}

void printTable(SimulatedClock *clock) { // This function prints out the info about the 20 processes.
	printf("\nOSS PID: %d SysClockS: %d SysClockNano: %d\n", getpid(), clock->seconds, clock->nanoseconds);
	printf("Process Table: \n");
	printf("Entry \t Occupied \t PID \t\t StartS \t StartN\n");
	for (int i = 0; i < 20; i++) {
		printf("%d \t %d \t\t %d \t\t %d\t\t %d \n", i, processTable[i].occupied, processTable[i].pid, processTable[i].startSeconds, processTable[i].startNano);
	}
}

void signalHandler(int sig, int shmid, SimulatedClock* clock) { // This is our signal handler, if 60 real seconds have passed, then kill all child processes and exit.
	fprintf(stderr, "\n[OSS] Alarm signal caught, terminating all processes.\n");
    	
	for(int i = 0; i < 20; i++){
        	if(processTable[i].occupied) {
		       	kill(processTable[i].pid, SIGTERM);
		}
    	}

    	exit(1);
}

void help() {
	printf("This program forks and splits off worker processes. It also controls the simulated clock and each loop, it increments. See below for parameters. \n");
	printf("Usage: ./oss [-h] [-n proc] [-s simul] [-t timelimit] [-i intervalMs]\n");
    	printf("-h : Show this help message.\n");
    	printf("-n : Total number of child processes to launch.\n");
    	printf("-s : Maximum number of children to run simultaneously.\n");
    	printf("-t : Upper bound (seconds) for random child runtime generation.\n");
    	printf("-i : Interval in milliseconds to wait before launching another child.\n");
}
