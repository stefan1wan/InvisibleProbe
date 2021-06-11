#include<stdio.h>
#include<unistd.h>
#include "ib_dev.h"
#include "ip_srv.h"



int main(int argc, char* argv[])
{
    //for(int i =0;i<argc;i++)
        //puts(argv[i]);
	const int MODE_SERVER = 0, MODE_CLIENT = 1;
	int mode = (argc == 1) ? MODE_SERVER : MODE_CLIENT;
	char* servername = (mode == MODE_SERVER) ? NULL : argv[1];
	int err;


	struct ib_dev_conf      *dev_conf = (struct ib_dev_conf*) malloc(sizeof(struct ib_dev_conf)); 
	dev_conf->ib_devname = NULL;
	dev_conf->ib_port = 1;

	struct pingpong_context *ctx = (struct pingpong_context*) malloc(sizeof(struct pingpong_context));
	ctx->context = ib_dev_init(dev_conf);
	if(!ctx->context)
		return -1;
	ctx->dev_conf = dev_conf;
	
	err = init_ctx(ctx);
	if(err){
		return -1;
	}

	struct ip_srv_conf *ip_conf = (struct ip_srv_conf*) malloc(sizeof(struct ip_srv_conf ));
	ip_conf->port = 8650;
	ip_conf->servername = servername;

	err = (mode == MODE_SERVER) ? ip_server_connect(ip_conf) : ip_client_connect(ip_conf);
	if(err)
		return -1;

	err = exchange_address(ip_conf->sockfd, ctx);
	if(err)
		return -1;

	err = ib_rtr(ctx);
	if(err)
		return -1;

	err = ib_rts(ctx);
	if(err)
		return -1;

	if (mode == MODE_CLIENT)
		err = ib_read_send(ctx);
	else
		sleep(20);
	if(err)
		return -1;
	
	ib_destroy(ctx);


    return 0;
}