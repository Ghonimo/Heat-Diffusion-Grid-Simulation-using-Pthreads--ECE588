#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/errno.h>
#include <time.h>

/* Global Defines*/

#define Xmax 1000
#define Ymax 1000
#define Cx 0.125
#define Cy 0.11
#define CMin 200
#define CMax 800
#define M_NPROCS 16
#define TIMESTEPS 4000

/* Global Definitions*/

float old[Xmax][Ymax];
float new[Xmax][Ymax];
int NumThreads;
int Count;
int Remainder;
struct timespec StartTime;
struct timespec EndTime;

/*Variables of Barrier*/

pthread_mutex_t SyncLock;
pthread_cond_t SyncCV;
int SyncCount;
/*Barrier function is same from synchronization from sum.c*/

void Barrier()
{
 int ret;
 pthread_mutex_lock(&SyncLock);
 SyncCount++;
 if(SyncCount == NumThreads)
 {
 ret = pthread_cond_broadcast(&SyncCV);
 assert(ret == 0);
 SyncCount=0;
 }
 else
 {
 ret = pthread_cond_wait(&SyncCV, &SyncLock);
 assert(ret == 0);
 }
 pthread_mutex_unlock(&SyncLock);
}

/*Function to calculate the temperature split by the rows x*/

void* Temp(void* tmp)

{
 long int threadId = (long int) tmp;
 int ret;
 int j,k,block;
 int start = threadId*Count;
 if (threadId <= Remainder)
 start += threadId;
 else
 start += Remainder;
 int end = start+Count-1;
 if (threadId < Remainder)
 end ++;
 Barrier();

 /*Job Execution for the number of steps counted as grid/number of threads*/

 for(block=1;block<=TIMESTEPS;block++) {
	for (j=start;j<=end;j++) {
		for(k=1;k<=Ymax;k++) {
			new[j][k]=(old[j][k])+(Cx*(old[j+1][k]+old[j-1][k]-2*(old[j][k])))+(Cy*(old[j][k+1]+old[j][k-1]-2*(old[j][k])));
			old[j][k]=new[j][k];
		}
	}

 Barrier();

 if((block%200)==0)
 printf("\nThe Temperature values at points:[1,1]=%f [200,200]=%f [400,400]=%f [500,500]=%f[700,700]=%f [900,900]=%f",new[1][1],old[200][200],new[400][400],new[500][500],new[700][700],new[900][900] );
 }
}

/*Main Func*/

main(int argc, char** argv)
{
 /*Loop Variables*/
 pthread_t* Threads;
 pthread_attr_t attr;
 int ret;
 long int ThreadID;

 /*Loop Variables*/

 int x,y;
 /*Arguments passed by command line*/

 if(argc<2)
 {
 fprintf(stderr, "Syntax: %s <numProcesors>\nExiting Program...\n", argv[0]);
 exit(1);
 }
 NumThreads=atoi(argv[1]);
 if (NumThreads < 1 || NumThreads >M_NPROCS)
 {
   fprintf(stderr,"Number of processors has to be between 1 and %d\nExiting Program...\n",M_NPROCS);
   exit(1);
 }

 /*Array Init*/
 for (x = 0; x < Xmax; x++) {
	 for (y = 0; y < Ymax; y++) {
			old[x][y] = 0;
	}
 }
 for(x=CMin-1;x<=CMax-1;x++) {
	for(y=CMin-1;y<=CMax-1;y++) {
		old[x][y]=500.0;
	}
 }

 /*Initializing threads*/

 Threads = (pthread_t *) malloc(sizeof(pthread_t) * NumThreads);
 assert(Threads!=NULL);

 /*Step Calculation*/

 Count = (Xmax-1) / NumThreads;
 Remainder = (Xmax-1) % NumThreads;

 /* Initialize thread attribute */

 pthread_attr_init(&attr);
 pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
 ret=pthread_mutex_init(&SyncLock,NULL);
 assert(ret==0);
 ret=pthread_cond_init(&SyncCV,NULL);
 assert(ret==0);
 SyncCount=0;

/*Starting the clock*/

ret = clock_gettime(CLOCK_REALTIME, &StartTime);
assert(ret == 0);

 /*Start threading*/

 for(ThreadID=0; ThreadID < NumThreads; ThreadID++)
 {
 ret = pthread_create(&Threads[ThreadID], &attr, Temp, (void*) ThreadID);
 assert(ret == 0);
 }

 /*Join threads after termination*/

 for(ThreadID=0; ThreadID < NumThreads; ThreadID++)
 {
 ret = pthread_join(Threads[ThreadID], NULL);
 assert(ret == 0);
 }

 /*Ending Clock*/

 ret = clock_gettime(CLOCK_REALTIME, &EndTime);
 assert(ret == 0);
 unsigned long long int runtime = 1000000000 * (EndTime.tv_sec - StartTime.tv_sec) + EndTime.tv_nsec - StartTime.tv_nsec;
 printf("\n(%d.%09lld sec)\n", runtime / 1000000000, runtime % 1000000000);
 pthread_mutex_destroy(&SyncLock);
 pthread_cond_destroy(&SyncCV);
 pthread_attr_destroy(&attr);
 return 0;
}
