//
//  main.cpp
//  Producer&&Comsumer
////////////////////////////////////////////////
//  get command line arguments argv[1]..argv[3]
//  Initialize buffer
//  Create produce thread
//  Create consumer thread
//  Sleep
//  Exit
//  Created by 魏晓 on 10/29/17.
//  Copyright © 2017 魏晓. All rights reserved.
////////////////////////////////////////////////

#include <pthread.h>
#include <semaphore.h>//信号量
#include <unistd.h>
#include "buffer.hpp"
#include <stdio.h>
#include <stdlib.h>

#define MAXSLEEP 5

bufferQueue buffer;

sem_t *full,*empty;//kong man xin hao liang
pthread_mutex_t mutex;
void *consume(void *args);
void *produce(void *args);

int main(int argc, const char * argv[]) {
    if(4!=argc){
        fprintf(stderr,"Usage:PCmain.out <int> <int> ,<int>\n");
        return -1;
    }
    if(0>atoi(argv[1])||0>atoi(argv[2])||0>atoi(argv[3]))
    {
        fprintf(stderr,"Input should be postive integer!\n");
        return -1;
    }//parameters checking
    int numofCsm=atoi(argv[1]);
    int numofPd=atoi(argv[2]);
    int timeofSleep=atoi(argv[3]);
    srand((unsigned int)time(NULL));
    
    buffer.head=buffer.tail=0;
    pthread_mutex_init(&mutex,NULL);
    empty=sem_open("empty", O_CREAT|O_RDWR,0666,BUFFER_SIZE-1);
    full=sem_open("full",O_CREAT|O_RDWR,0666,0);
    if(SEM_FAILED==empty||SEM_FAILED==full)
    {
        perror("semaphore failed");
        return -1;
    }//initialize data and semaphore
    
    pthread_t *consumers,*producers;
    consumers=(pthread_t*)malloc(numofCsm * sizeof(pthread_t));
    producers=(pthread_t*)malloc(numofPd * sizeof(pthread_t));
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    
    int i;
    for(i=0;i<numofCsm;++i){
        int num=i;
        pthread_create(&consumers[i],&attr,consume,&num);
    }
    for(i=0;i<numofPd;++i){
        int num=i;
        pthread_create(&producers[i],&attr,produce,&num);
    }
    sleep(timeofSleep);
    
    printf("The process is completed!\n");
    sem_unlink("empty");
    sem_unlink("full");
    pthread_mutex_destroy(&mutex);
    exit(0);
}
void *consume(void *args)
{
    int CsmID=(int)(*((int*)args));
    while(true)
    {
        sleep(rand()%MAXSLEEP+1);
        
        sem_wait(full);
        pthread_mutex_lock(&mutex);
        int x=remove_item(&buffer);
        if(x<0){
            fprintf(stderr, "The queue is empty!\n");
            exit(-1);
        }
        else
            printf("Consumer %d get %d from buffer\n",CsmID,x);
        
        pthread_mutex_unlock(&mutex);
        sem_post(empty);
    }
}
void *produce(void *args)
{
    int pdsID=(int)(*((int*)args));
    while(true)
    {
        sleep(rand()%MAXSLEEP);
        sem_wait(empty);
        pthread_mutex_lock(&mutex);
        
        int num=rand()%MAXSLEEP;
        int x=insert_item(&buffer, num);
        if(x<0)
        {
            fprintf(stderr,"The queue is full! \n");
            exit(-1);
        }
        else
            printf("Producer %d add %d to buffer\n",pdsID,x);
        
        pthread_mutex_unlock(&mutex);
        sem_post(full);
    }
}
























