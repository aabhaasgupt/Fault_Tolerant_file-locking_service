#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>  //Header file for sleep(). man 3 sleep for details. 
#include <pthread.h> 


// A normal C function that is executed as a thread  
// when its name is specified in pthread_create() 
void *myThreadFun(void *vargp) 
{ 

	for(int i=0;i<10000;i++) {
	sleep(1);

	printf("\a");
	}
	
	return NULL; 
}
void *myThreadFun2(void *vargp) 
{ 

	for(int i=0;i<1000;i++) 
    printf("Printing GeeksQuiz from Thread 2 %d\n",i); 
    return NULL; 
} 
   
int main() 
{ 
    pthread_t thread_id; 
    pthread_t thread_id2; 
    printf("Before Thread\n"); 
    pthread_create(&thread_id, NULL, myThreadFun, NULL); 
    pthread_create(&thread_id2, NULL, myThreadFun2, NULL); 
    pthread_join(thread_id, NULL); 
    pthread_join(thread_id2, NULL); 
    for(int j=0;j<10;j++)
	{
		printf("After Thread\n"); 
	}
	exit(0); 
}