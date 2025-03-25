Project Author: Dat Nguyen

Course: Comp Sci 4760

Date: 3/24/2025

GitHub Link: github.com/O-wll/OPSYS4760-PROJECT-3

Description of Project:

This project utilizes a simulated clock and shared memory. oss.c creates shared memory and shares the SimulatedClock struct, worker.c will have access to this shared memory. oss.c initializes the clock and forks child processes, also known as workers. worker.c prints out the PID, PPID, time on simulated clock, the lifetime of the worker, and how many seconds have passed since worker child was initiated. This project differs from project 2 because it uses a message queueing system that is communication between OSS and worker, the msges they send are 0 or 1, 0 indicates termination of the child, 1 indicates keep running, they use this message system to communicate when a child should terminate.
 
User will be able to:
Control how many total child/workers there are.
Control how many total child/workers run at once.
Control how long the child process will last/
Control the interval of when child processes run.
View the log file of OSS output (worker not included.)

How to compile, build, and use project:

The project comes with a makefile so ensure that when running this project that the makefile is in it.

Type 'make' and this will generate both the oss exe and user exe along with their object file.

user exe is ONLY for testing if user works, to properly use this program, type './oss -h' for instructions on how to use it.

When done and want to delete, run 'make clean' and the exe and object files will be deleted.

Issues Ran Into:

Couldn't test if alarm logic, for 60 seconds real time, works, theoretically it should.
Ran into a sizeof error that caused error messages to send in the output.
Github updates weren't as consistent as I hoped they were.
General syntax/coding slipups that weren't noticed until testing.
