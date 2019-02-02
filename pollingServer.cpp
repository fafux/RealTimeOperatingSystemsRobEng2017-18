/* Fabio Fusaro 4068572
Starting from the examples that we have seen during the lesson (RM with aperiodic tasks in background) I make a different example in which aperiodic tasks are scheduled using a polling server. */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <sched.h>
#include <sys/types.h>
#include <queue>
#include <iostream>
using namespace std;

#define NTASKS 6 // define the number of the tasks
#define NPERIODICTASKS 4 // define the number of the periodic tasks

#define TASK4 1 // define the add element to queue for aperiodic task 4
#define TASK5 2 // define the add element to queue for aperiodic task 4
#define CAPACITY 10000000 // define total capacity

// application specific code
void task1_code( );
void task2_code( );
void task3_code( );
void task4_code( );
void task5_code( );
void polling_server_code();

// thread functions 
void *task1( void *);
void *task2( void *);
void *task3( void *);
void *polling_server(void *);

// initialization of queue
queue<int> queuePolling;

// initialization of mutex for queue and conditions
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

long int capTask4, capTask5; // initialization of the variables associated to the WCET of task 4 and 5
long int capTot = CAPACITY; // initialization of the total capacity

long int periods[NTASKS];
struct timeval next_arrival_time[NTASKS];
long int WCET[NTASKS];
int missed_deadlines[NTASKS];
pthread_attr_t attributes[NTASKS];
struct sched_param parameters[NTASKS];
pthread_t thread_id[NTASKS];

int main(){ 

	// setting threads periods: polling server with the lowest period and so the highest priority
	periods[0]= 100000000; 
	periods[1]= 200000000; 
	periods[2]= 400000000;
	periods[3]= 800000000; 

	struct sched_param priomax;
	priomax.sched_priority=sched_get_priority_max(SCHED_FIFO);	
	struct sched_param priomin;
	priomin.sched_priority=sched_get_priority_min(SCHED_FIFO);

	if (getuid() == 0)
		pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomax);

	for (int i =0; i < NTASKS; i++) {

		struct timeval timeval1;
	    struct timezone timezone1;
	    struct timeval timeval2;
	    struct timezone timezone2;
	    gettimeofday(&timeval1, &timezone1);

		if (i==0) {
	        polling_server_code();
			WCET[i] = CAPACITY;
		}

	    if (i==1)
			task1_code();
    	if (i==2)
			task2_code();
		if (i==3)
			task3_code();

	    //aperiodic tasks
	    if (i==4)
			task4_code();
	    if (i==5)
			task5_code();

	    gettimeofday(&timeval2, &timezone2);

	    WCET[i]= 1000*((timeval2.tv_sec - timeval1.tv_sec)*1000000+(timeval2.tv_usec-timeval1.tv_usec));

	    cout<<"Worst Case Execution Time "<<i<<"="<<WCET[i]<<endl; 
	}

	capTask4 = WCET[4]; //assign to a variable the WCET of the aperiodic task 4
	capTask5 = WCET[5]; //assign to a variable the WCET of the aperiodic task 5

	double Ulub = NPERIODICTASKS*(pow(2.0,(1.0/NPERIODICTASKS)) -1);

	double U = 0;

	for (int i = 0; i < NPERIODICTASKS; i++)
	    U+= ((double)WCET[i])/((double)periods[i]);

	if (U > Ulub) {
    	cout<<endl<<"U="<<U<<" Ulub="<<Ulub<<" Not schedulable"<<endl;
    	return(-1);
    }

	cout<<endl<<"U="<<U<<" Ulub="<<Ulub<<" Schedulable"<<endl;
	fflush(stdout);
	sleep(5);

	if (getuid() == 0)
	    pthread_setschedparam(pthread_self(),SCHED_FIFO,&priomin);

	for (int i =0; i < NPERIODICTASKS; i++) {

	    pthread_attr_init(&(attributes[i]));
	    pthread_attr_setinheritsched(&(attributes[i]), PTHREAD_EXPLICIT_SCHED);
      
	    pthread_attr_setschedpolicy(&(attributes[i]), SCHED_FIFO);

	    parameters[i].sched_priority = priomax.sched_priority - i;

	    pthread_attr_setschedparam(&(attributes[i]), &(parameters[i]));
    }

	int iret[NTASKS];
	struct timeval ora;
	struct timezone zona;
	gettimeofday(&ora, &zona);

	for (int i = 0; i < NPERIODICTASKS; i++) {

    	long int periods_micro = periods[i]/1000;
    	next_arrival_time[i].tv_sec = ora.tv_sec + periods_micro/1000000;
    	next_arrival_time[i].tv_usec = ora.tv_usec + periods_micro%1000000;
    	missed_deadlines[i] = 0;
    
	}

	// thread creation
	iret[0] = pthread_create( &(thread_id[0]), &(attributes[0]), polling_server, NULL);
	iret[1] = pthread_create( &(thread_id[1]), &(attributes[1]), task1, NULL);
	iret[2] = pthread_create( &(thread_id[2]), &(attributes[2]), task2, NULL);
	iret[3] = pthread_create( &(thread_id[3]), &(attributes[3]), task3, NULL);

	// thread main wait termination of all other threads
	pthread_join( thread_id[0], NULL);
	pthread_join( thread_id[1], NULL);
	pthread_join( thread_id[2], NULL);
	pthread_join( thread_id[3], NULL); //Join only for periodic tasks

    return 0;
}

void task1_code() {
  
	cout<<"1 start"<<endl;

	double uno;
	for (int i = 0; i < 10; i++) {
    	for (int j = 0; j < 1000; j++) {
			uno = rand()*rand()%10;
		}
    }

	// when the random variable uno=0, then aperiodic task 4 must be added to the queue

	if (uno == 0) {

    	cout<<"4 into queue"<<endl;fflush(stdout);

		pthread_mutex_lock(&queue_mutex);
    	queuePolling.push(TASK4); //add the variable of the aperiodic task 4 to the queue
		pthread_mutex_unlock(&queue_mutex);

    }

	// when the random variable uno=1, then aperiodic task 5 must be added to the queue

	if (uno == 1) {

	    cout<<"5 into queue"<<endl;fflush(stdout);

		pthread_mutex_lock(&queue_mutex);
	    queuePolling.push(TASK5); //add the variable of the aperiodic task 5 to the queue
		pthread_mutex_unlock(&queue_mutex);

    }
  
	cout<<"1 end"<<endl;
	fflush(stdout);
}

void *task1( void *ptr) {

	int i=0;
	struct timespec waittime;
	waittime.tv_sec=0; /* seconds */
	waittime.tv_nsec = periods[1]; /* nanoseconds */

	/* forcing the thread to run on CPU 0 */
    cpu_set_t cset;
    CPU_ZERO( &cset );
    CPU_SET( 0, &cset);

    pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

	while (1) {

    	task1_code(); // execute the task 1

    	struct timeval ora;
    	struct timezone zona;
    	gettimeofday(&ora, &zona);

    	long int timetowait= 1000*((next_arrival_time[1].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[1].tv_usec-ora.tv_usec));
    	if (timetowait <0)
			missed_deadlines[1]++;

    	waittime.tv_sec = timetowait/1000000000;
    	waittime.tv_nsec = timetowait%1000000000;

    	nanosleep(&waittime, NULL);

    	long int periods_micro=periods[1]/1000;
    	next_arrival_time[1].tv_sec = next_arrival_time[1].tv_sec + periods_micro/1000000;
    	next_arrival_time[1].tv_usec = next_arrival_time[1].tv_usec +periods_micro%1000000;
	}
}

void task2_code() {

	cout<<"2 start"<<endl; fflush(stdout);

	double uno;
	for (int i = 0; i < 10; i++) {
    	for (int j = 0; j < 1000; j++) 
			uno = rand()*rand();
    }

	cout<<"2 end"<<endl;
	fflush(stdout);
}

void *task2( void *ptr ) {

	int i=0;
	struct timespec waittime;
	waittime.tv_sec=0; /* seconds */
	waittime.tv_nsec = periods[2]; /* nanoseconds */

    /* forcing the thread to run on CPU 0 */
    cpu_set_t cset;
    CPU_ZERO( &cset );
    CPU_SET( 0, &cset);

    pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

	while (1) {

	    task2_code(); // execute the task 2

	    struct timeval ora;
	    struct timezone zona;
	    gettimeofday(&ora, &zona);

	    long int timetowait= 1000*((next_arrival_time[2].tv_sec - ora.tv_sec)*1000000 +(next_arrival_time[2].tv_usec-ora.tv_usec));
    	if (timetowait <0)
			missed_deadlines[2]++;

    	waittime.tv_sec = timetowait/1000000000;
    	waittime.tv_nsec = timetowait%1000000000;

    	nanosleep(&waittime, NULL);
    
		long int periods_micro=periods[2]/1000;
	    next_arrival_time[2].tv_sec = next_arrival_time[2].tv_sec + periods_micro/1000000;
	    next_arrival_time[2].tv_usec = next_arrival_time[2].tv_usec + periods_micro%1000000;
	}
}

void task3_code() {

	cout<<"3 start"<<endl; fflush(stdout);

	double uno;
	for (int i = 0; i < 10; i++) {
    	for (int j = 0; j < 1000; j++)	
      		uno = rand()*rand();
    }

	cout<<"3 end"<<endl;
	fflush(stdout);
}

void *task3( void *ptr) {

	int i=0;
	struct timespec waittime;
	waittime.tv_sec=0; /* seconds */
	waittime.tv_nsec = periods[3]; /* nanoseconds */

	/* forcing the thread to run on CPU 0 */
    cpu_set_t cset;
    CPU_ZERO( &cset );
    CPU_SET( 0, &cset);

    pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

	while (1) {

    	task3_code(); // execute the task 3

    	struct timeval ora;
    	struct timezone zona;
    	gettimeofday(&ora, &zona);
    
		long int timetowait= 1000*((next_arrival_time[3].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[3].tv_usec-ora.tv_usec));
    	if (timetowait <0)
			missed_deadlines[3]++;

    	waittime.tv_sec = timetowait/1000000000;
    	waittime.tv_nsec = timetowait%1000000000;

    	nanosleep(&waittime, NULL);

	    long int periods_micro=periods[3]/1000;
	    next_arrival_time[3].tv_sec = next_arrival_time[3].tv_sec + periods_micro/1000000;
    	next_arrival_time[3].tv_usec = next_arrival_time[3].tv_usec + periods_micro%1000000;
	}
}

void task4_code() {

	cout<<"-aperiodic 4- start"<<endl; fflush(stdout);

	double uno;
	for (int i = 0; i < 10; i++) {
    	for (int j = 0; j < 1000; j++)
			uno = rand()*rand();
    }

	cout<<"-aperiodic 4- end"<<endl;
	fflush(stdout);
}

void task5_code() {

	cout<<"-aperiodic 5- start"<<endl; fflush(stdout);

	double uno;
	for (int i = 0; i < 10; i++) {
    	for (int j = 0; j < 1000; j++)	
			uno = rand()*rand();
    }
	
	cout<<"-aperiodic 5- end"<<endl;
	fflush(stdout);
}

void polling_server_code(){
 
	cout<<"polling_server start"<<endl; fflush(stdout);

	int capLeft = capTot; //initialization of the left capacity with the total capacity

	while (!queuePolling.empty()) { // enter if the queue is not empty

		// control if the last queue value coinciding to the value of task 4
		// and if the left capacity is more or equal to the capacity of task 4
    	if (queuePolling.front() == TASK4 && capLeft >=  capTask4) { 

  			task4_code(); // execute the task 4

			pthread_mutex_lock(&queue_mutex);
			queuePolling.pop(); // remove the element associated to the task 4 from queue 
			pthread_mutex_unlock(&queue_mutex);

			capLeft -= capTask4; // calculate the left capacity after the execution of task 4
 
    	}

		// control if the last queue value coinciding to the value of task 5
		// and if the left capacity is more or equal to the capacity of task 5
		else if (queuePolling.front() == TASK5 && capLeft >=  capTask5) {

 			task5_code(); // execute the task 5

			pthread_mutex_lock(&queue_mutex);
			queuePolling.pop(); // remove the element associated to the task 5 from queue
			pthread_mutex_unlock(&queue_mutex);

			capLeft -= capTask5; // calculate the left capacity after the execution of task 5
  		}

		else {

			break;
			
		}

	}

    cout<<"polling_server end"<<endl; fflush(stdout);
}


void *polling_server( void *ptr) {
  
	int i=0;
	struct timespec waittime;
	waittime.tv_sec=0; /* seconds */
	waittime.tv_nsec = periods[0]; /* nanoseconds */

	/* forcing the thread to run on CPU 0 */
    cpu_set_t cset;
    CPU_ZERO( &cset );
    CPU_SET( 0, &cset);

    pthread_setaffinity_np( pthread_self(), sizeof(cpu_set_t), &cset);

	while (1) {

    	polling_server_code(); // execute the polling server

    	struct timeval ora;
    	struct timezone zona;
    	gettimeofday(&ora, &zona);

    	long int timetowait= 1000*((next_arrival_time[0].tv_sec - ora.tv_sec)*1000000+(next_arrival_time[0].tv_usec-ora.tv_usec));
        if (timetowait <0)
      		missed_deadlines[0]++;
      
    	waittime.tv_sec = timetowait/1000000000;
    	waittime.tv_nsec = timetowait%1000000000;
    	
		nanosleep(&waittime, NULL);

	    long int periods_micro=periods[0]/1000;
    	next_arrival_time[0].tv_sec = next_arrival_time[0].tv_sec + periods_micro/1000000;
    	next_arrival_time[0].tv_usec = next_arrival_time[0].tv_usec + periods_micro%1000000;
	}

}