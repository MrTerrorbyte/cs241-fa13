/** @file libscheduler.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
*/

typedef struct _job_t
{
	int num;
	int pri;
	int timeLeft;
	int timeArr;
	int dur;
	int timeStart;
	int timeUpdate;
	int responded;
} job_t;


/**
  Initalizes the scheduler.
 
  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
priqueue_t * queue;
scheme_t currScheme; 
int currTime;
job_t ** jobsArray;
int * coresArray;
int numCores;
int numJobs;
int response;
int numResponse;
int waiting;
int turnaround;

int comparer(const void * ptr1, const void * ptr2){
	job_t * job1= (job_t *)(ptr1);
	job_t * job2= (job_t *)(ptr2);
	if(currScheme == FCFS){
		return job1->timeArr - job2->timeArr;
	}
	if(currScheme == PRI || currScheme == PPRI){
		if(job1->pri == job2->pri)
			return job1->timeArr - job2->timeArr;
		return job1->pri - job2->pri;
	}
	if(currScheme == PSJF || currScheme == SJF){
		if(job1->timeLeft == job2->timeLeft)
			return job1->timeArr - job2->timeArr;
		return job1->timeLeft - job2->timeLeft;
	}
	if(currScheme == RR){
		return 0;
	}
	return 0;
}

void scheduler_start_up(int cores, scheme_t scheme)
{
	queue= malloc(sizeof(priqueue_t));
	currScheme= scheme;
	priqueue_init(queue, &comparer);
	currTime= 0;
	jobsArray= malloc(sizeof(job_t *) * cores);
	coresArray= malloc(sizeof(int) * cores);
	numCores= cores;
	numJobs= 0;
	response= 0;
	waiting= 0;
	turnaround= 0;

	int i= 0;
	for( ; i < cores; i++){
		jobsArray[i]= NULL;
	}
	for(i=0; i < cores; i++){
		coresArray[i]= 0;
	}
	
}

void addResponse(int time){
	response+= time;
	numResponse++;
}

void updateTime(int time){
	currTime= time;
	int i=0;
	for( ; i < numCores; i++){
		if(coresArray[i]){
			if(jobsArray[i]->responded == 0 && jobsArray[i]->timeUpdate != time){
				jobsArray[i]->responded= 1;
				jobsArray[i]->timeStart= jobsArray[i]->timeUpdate;
				addResponse(jobsArray[i]->timeStart - jobsArray[i]->timeArr);
			}
			jobsArray[i]->timeLeft -= (time - jobsArray[i]->timeUpdate);
			jobsArray[i]->timeUpdate= time;
		}
	}
}

/**
  Called when a new job arrives.
 
  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made. 
 
 */




int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
	updateTime(time);
	job_t * newJob= malloc(sizeof(job_t));
	newJob->num= job_number;
	newJob->timeArr= time;
	newJob->dur= running_time;
	newJob->timeLeft= running_time;
	newJob->pri= priority;
	newJob->timeUpdate= 0;
	newJob->timeStart= -1;
	newJob->responded= 0;

	int i=0;
	for( ; i < numCores; i++){
		if(coresArray[i] == 0){
			coresArray[i]= 1;
			jobsArray[i]= newJob;
			newJob->timeStart= time;
			newJob->timeUpdate= time;
			return i;
		}
	}
	if(currScheme == PSJF || currScheme == PPRI){
		int bestIndex= -1;
		int currBest= 0;
		int new= 0;
		for(i=0; i < numCores; i++){
			new= comparer(newJob, jobsArray[i]);
			if(new < currBest){
				currBest= new;
				bestIndex= i;
			}
			else if(new == currBest){
				if(bestIndex >= 0){
					if(jobsArray[bestIndex]->timeArr < jobsArray[i]->timeArr){//check this statement below
						bestIndex= i;
					}
				}
			}
		}
		if(bestIndex >= 0){
			priqueue_offer(queue, (void *)(jobsArray[bestIndex]));
			jobsArray[bestIndex]= newJob;
			newJob->timeStart= time;
			newJob->timeUpdate= time;
			return bestIndex;
		}
		else{
			priqueue_offer(queue, newJob);
			return -1;
		}
	}
	else{
		priqueue_offer(queue, newJob);
		return -1;
	}
	return -1;
}


/**
  Called when a job has completed execution.
 
  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.
 
  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
	updateTime(time);
	numJobs++;
	//response+= jobsArray[core_id]->timeStart - jobsArray[core_id]->timeArr;
	turnaround+= (time - jobsArray[core_id]->timeArr);
	waiting+= (time - jobsArray[core_id]->timeArr - jobsArray[core_id]->dur);

	coresArray[core_id]= 0;
	priqueue_remove(queue, jobsArray[core_id]);

	free(jobsArray[core_id]);
	job_t * contJob= priqueue_poll(queue);
	if(contJob){
		coresArray[core_id]= 1;
		jobsArray[core_id]= contJob;
		contJob->timeUpdate= time;
		if(contJob->timeStart < 0)
			contJob->timeStart= time;
		return contJob->num;
	}

	return -1;
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.
 
  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
	updateTime(time);
	priqueue_offer(queue, jobsArray[core_id]);
	job_t * newJob= priqueue_poll(queue);
	if(newJob){
		jobsArray[core_id]= newJob;
		newJob->timeUpdate= time;
		newJob->timeStart= time;
		return newJob->num;
	}
	return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	return waiting/(float)(numJobs);
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
	return turnaround/(float)(numJobs);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
	return response/(float)numResponse;
}


/**
  Free any memory associated with your scheduler.
 
  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
	priqueue_destroy(queue);
	free(jobsArray);
	free(coresArray);
	free(queue);
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)  
  
  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{

}
