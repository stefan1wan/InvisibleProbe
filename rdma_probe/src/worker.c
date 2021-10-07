#include<stdio.h>
#include<pthread.h>
#include <unistd.h>
#include "ib_dev.h"
#include "worker.h"



void* send_worker(void* arg){
	struct send_args* args = (struct send_args*)arg;
	int rounds = args->send_num/args->send_batch_sz;
	if(rounds*args->send_batch_sz < args->send_num)
		rounds++;
	for(int i = 0; i<rounds; i++){

		int sent_num = i*args->send_batch_sz;
		int queued_num = sent_num - *(args->current_num);
		if(queued_num > 5*args->send_batch_sz)
			usleep(10);

		int err = ibv_post_send(args->qp, args->wr, args->bad_wr);
		if(err){
			fprintf(stderr, "Couldn't Send\n");
			printf("%d\n",errno);
			return NULL;
		}else;
//printf("%d Sent\n", i);
	}
	return NULL;
}
