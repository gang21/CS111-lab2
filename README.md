## UID: 005677251

## RR
This program is the implementation for round robin scheduling found in many OS systems for process handling. Given a file input containing workload and quantem length for processes, this program outputs the average waiting time and average response time for Round Robin (RR) Scheduling. 

## Building
To correctly build this program, download rr.c and the Makefile from the github repository. Optionally, you may also download processes.txt to test the program, or create your own text file containing the processes. Run the following commands in the folder containing both the Makefile and rr.c:
```
make
```

## Running
To run this program, you first require a file in the following format: 
    - first line: number of processes run
    - following lines (processes): process id, arrival time, burst time
To see or use and example of the format, see the processes.txt file

To run the program, use the following command:
```
./rr processes.txt 3
``` 
where processes.txt can be replaced with the corresponding processes file and 3 refers to the quantum length for each process. 

## Cleaning up
To clean the program, run the following code in the same folder as the Makefile and rr.c:
```
make clean
```
