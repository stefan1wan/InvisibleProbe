#include <malloc.h>
#include <stdlib.h>
#include<unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include "ib_dev.h"
#include "utils.h"
#include "worker.h"

#define ALLOCATE(var,type,size)                                     \
{ if((var = (type*)malloc(sizeof(type)*(size))) == NULL)        \
	{ fprintf(stderr," Cannot Allocate\n"); exit(1);}}

#define GET_STRING(orig,temp) 						            \
{ ALLOCATE(orig,char,(strlen(temp) + 1)); strcpy(orig,temp); }

const char *transport_str(enum ibv_transport_type type)
{
	switch (type) {
		case IBV_TRANSPORT_IB:
			return "IB";
		case IBV_TRANSPORT_IWARP:
			return "IW";
		default:
			return "Unknown";
	}
}

const char *link_layer_str(int8_t link_layer)
{
	switch (link_layer) {
		case IBV_LINK_LAYER_UNSPECIFIED:
            return "UNSPEC";
		case IBV_LINK_LAYER_INFINIBAND:
			return "IB";
		case IBV_LINK_LAYER_ETHERNET:
			return "Ethernet";
		default:
			return "Unknown";
	}
}

struct ibv_device* ctx_find_dev(const char *ib_devname)
{
	int num_of_device;
	struct ibv_device **dev_list;
	struct ibv_device *ib_dev = NULL;
    struct ibv_device* const FAILURE = NULL;

	dev_list = ibv_get_device_list(&num_of_device);

	if (num_of_device <= 0) {
		fprintf(stderr," Did not detect devices \n");
		return FAILURE;
	}

	if (!ib_devname) {
		ib_dev = dev_list[0];
		if (!ib_dev) {
			fprintf(stderr, "No IB devices found\n");
			return FAILURE;
		}
	} else {
		for (; (ib_dev = *dev_list); ++dev_list)
			if (!strcmp(ibv_get_device_name(ib_dev), ib_devname))
				break;
		if (!ib_dev){
			fprintf(stderr, "IB device %s not found\n", ib_devname);
            return FAILURE;
        }
	}
	return ib_dev;
}

struct ibv_context* ib_dev_init(struct ib_dev_conf* dev_conf){

    struct ibv_device		*ib_dev;
	struct ibv_context		*context;
	struct ibv_port_attr port_attr;
    struct ibv_context* const FAILURE = NULL;

	//Find and open an IB device.
	ib_dev = ctx_find_dev(dev_conf->ib_devname);
	if (!ib_dev) {
		fprintf(stderr," Unable to find the Infiniband/RoCE device\n");
		return FAILURE;
	}

	//Open the found IB device.
	context = ibv_open_device(ib_dev);
	if (!context) {
		fprintf(stderr, " Couldn't get context for the device\n");
		return FAILURE;
	}

	//Get the device name
	if (!dev_conf->ib_devname)
		GET_STRING(dev_conf->ib_devname,ibv_get_device_name(context->device)) //Memory Leakage There

	//Query and check port attributes
	if (ibv_query_port(context, dev_conf->ib_port, &port_attr)) {
		fprintf(stderr, " Unable to query port %d attributes\n", dev_conf->ib_port);
		return FAILURE;
	}
	if (port_attr.state != IBV_PORT_ACTIVE) {
		fprintf(stderr, " Port number %d state is %s\n"
				,dev_conf->ib_port
				,portStates[port_attr.state]);
		return FAILURE;
	}
	memcpy(&(dev_conf->port_attr), &port_attr, sizeof(struct ibv_port_attr));
	dev_conf->transport_type = context->device->transport_type;
	printf("Using dev: %s; Port: %d; Transport Type: %s; Link Type: %s;\n", \
            dev_conf->ib_devname, dev_conf->ib_port, \
            transport_str(dev_conf->transport_type), link_layer_str(dev_conf->port_attr.link_layer));
    return context;	
}

int init_ctx(struct pingpong_context *ctx){
	ctx->buf_size = 4*1024*1024;
	const int sq_depth = 5000;
	const int rq_depth = 20;
	int err;

	ctx->buf = memalign(4096, ctx->buf_size);
	if(!ctx->buf){
		fprintf(stderr, "Couldn't allocate Working Buffer\n");
		return -1;
	}

	memset(ctx->buf, 0xff, ctx->buf_size);

	ctx->pd = ibv_alloc_pd(ctx->context);
	if(!ctx->pd){
		fprintf(stderr, "Couldn't allocate Protection Domain\n");
		return -1;
	}

	ctx->mr = ibv_reg_mr(ctx->pd, ctx->buf, ctx->buf_size, IBV_ACCESS_LOCAL_WRITE | \
		IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if(!ctx->mr){
		fprintf(stderr, "Couldn't Register Memory Region\n");
		return -1;
	}
	
	struct ibv_exp_cq_init_attr ex_attr;
	memset(&ex_attr,0,sizeof(ex_attr));
	ex_attr.flags = IBV_EXP_CQ_TIMESTAMP;
	ex_attr.comp_mask = IBV_EXP_CQ_INIT_ATTR_FLAGS;
	ctx->cq = ibv_exp_create_cq(ctx->context, sq_depth+rq_depth, NULL, NULL, 0, &ex_attr);
	if(!ctx->cq){
		fprintf(stderr, "Couldn't Create CQ\n");
		return -1;
	}

	struct ibv_qp_init_attr init_attr = {
		.qp_context = NULL,
		.send_cq = ctx->cq,
		.recv_cq = ctx->cq,
		.srq = NULL,
		.cap = {
			.max_send_wr   = sq_depth,
			.max_recv_wr   = rq_depth,
			.max_send_sge  = 1,
			.max_recv_sge = 1,
			.max_inline_data = 0
		},
		.qp_type = IBV_QPT_RC,
		.sq_sig_all = 0    
	};
	ctx->qp = ibv_create_qp(ctx->pd, &init_attr);
	if(!ctx->qp){
		printf("%d\n",errno);
		fprintf(stderr, "Couldn't Create Queue Pair\n");
		return -1;
	}

	struct ibv_qp_attr attr;
	memset(&attr, 0, sizeof(attr));
	attr.qp_state = IBV_QPS_INIT;
	attr.port_num = ctx->dev_conf->ib_port;
	err = ibv_modify_qp(ctx->qp, &attr, IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);
	if(err){
		printf("%d\n",errno);
		fprintf(stderr, "Couldn't Init Queue Pair\n");
		return -1;
	}
	
	ctx->local.lid = ctx->dev_conf->port_attr.lid;
	ctx->local.qpn = ctx->qp->qp_num;
	ctx->local.port_num = ctx->dev_conf->ib_port;
	srand(time(NULL));
	ctx->local.psn = rand() & 0xffffff;
	return 0;

}

int exchange_address(int sockfd, struct pingpong_context* ctx){
	struct xhg_info buf=
	{
		.local_addr = ctx->local,
		.mem_addr   = ctx->mr->addr,
		.rkey       = ctx->mr->rkey
		};
	int err;

	err = write(sockfd, &buf, sizeof(buf));
	if(err == -1)
		return -1;
	
	err = read(sockfd, &buf, sizeof(buf));
	if(err == -1)
		return -1;

	ctx->remote = buf;
	printf("Remote Info: LID:%d, QPN:%d, ADDR:%p, RKEY:0x%x\n", \
		ctx->remote.local_addr.lid, ctx->remote.local_addr.qpn, ctx->remote.mem_addr, ctx->remote.rkey);
	return 0;
}

int ib_rtr(struct pingpong_context* ctx){
	struct ibv_qp_attr attr;
	struct ibv_ah_attr ah_attr;
	memset(&attr, 0, sizeof(attr));
	memset(&ah_attr, 0, sizeof(ah_attr));

	ah_attr.dlid = ctx->remote.local_addr.lid;
	ah_attr.sl = 1;
	ah_attr.port_num = ctx->local.port_num;

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_4096;
	attr.dest_qp_num = ctx->remote.local_addr.qpn;
	attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
	attr.rq_psn = ctx->remote.local_addr.psn;
	attr.max_dest_rd_atomic = 1;
	attr.min_rnr_timer = 12;
	attr.ah_attr= ah_attr;

	int err = ibv_modify_qp(ctx->qp, &attr, IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | \
		IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER | IBV_QP_ACCESS_FLAGS);
	if(err){
		printf("%d\n",errno);
		fprintf(stderr, "Couldn't Set RTR\n");
		return -1;
	}
	return 0;
}

int ib_rts(struct pingpong_context* ctx){
	struct ibv_qp_attr attr;
	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 14;
	attr.retry_cnt = 7;
	attr.rnr_retry = 7;
	attr.sq_psn = ctx->local.psn;
	attr.max_rd_atomic = 1;

	int err = ibv_modify_qp(ctx->qp, &attr, IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | \
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);
	if(err){
		printf("%d\n",errno);
		fprintf(stderr, "Couldn't Set RTS\n");
		return -1;
	}
	return 0;
}




int ib_read_send(struct pingpong_context* ctx){
	struct ibv_send_wr wr;
	struct ibv_send_wr *bad_wr;
	struct ibv_sge sg;
	memset(&wr, 0, sizeof(wr));
	wr.sg_list = &sg;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_RDMA_READ;
	wr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_FENCE;
	wr.wr.rdma.remote_addr = (uint64_t) ctx->remote.mem_addr;
	wr.wr.rdma.rkey = ctx->remote.rkey;

	sg.addr = (uint64_t) ctx->buf;
	sg.length = 2;
	sg.lkey = ctx->mr->lkey;

	struct ibv_send_wr* p = &wr;
	const int send_num = 10000000;
	const int send_batch_sz = 32;

	struct record{
		uint64_t poll_cnt;
		uint64_t cpu_ts;
		uint64_t hw_ts;
	};

	struct record* records = (struct record*) malloc(sizeof(struct record) * send_num);

	for(int i=1;i<send_batch_sz;i++){
		p->next = (struct ibv_send_wr*) malloc(sizeof(wr));
		memcpy(p->next, &wr, sizeof(wr));
		p = p->next;
		p->next = NULL;
	}

	memset(ctx->buf, 0, ctx->buf_size);

	int current = 0;
	struct send_args arg={
		.qp=ctx->qp,
		.wr = &wr,
		.bad_wr=&bad_wr,
		.send_num = send_num,
		.send_batch_sz = send_batch_sz,
		.current_num = &current
		};

	pthread_t tid;
	int err = pthread_create(&tid, NULL, send_worker, &arg);

	/*int err = ibv_post_send(ctx->qp, &wr, &bad_wr);
	if(err){
		fprintf(stderr, "Couldn't Send\n");
		return -1;
	}else
		printf("Sent\n");*/

	struct ibv_exp_wc wc;
	int num_comp;
 
	for(int i = 0;i<send_num;i++,current++){
		records[i].poll_cnt = 0;
		do {
			num_comp = ibv_exp_poll_cq(ctx->cq, 1, &wc, sizeof(wc));
			records[i].poll_cnt++;
		} while (num_comp == 0);

		records[i].cpu_ts = rdtscp();
		records[i].hw_ts  = wc.timestamp;

		if (num_comp < 0) {
			fprintf(stderr, "ibv_poll_cq() failed\n");
			return -1;
		}
		if (wc.status != IBV_WC_SUCCESS) {
			fprintf(stderr, "Failed status %s (%d) for wr_id %d\n", 
				ibv_wc_status_str(wc.status), wc.status, (int)wc.wr_id);
			return -1;
		}
	}

	FILE* fp;
	fp = fopen("records.bin","wb");
	fwrite(records, sizeof(struct record), send_num, fp);
	fclose(fp);
	return 0;
}

int ib_destroy(struct pingpong_context* ctx){
	int err;
	err = ibv_destroy_qp(ctx->qp);
	if(err) return -1;

	err = ibv_destroy_cq(ctx->cq);
	if(err) return -1;

	err = ibv_dereg_mr(ctx->mr);
	if(err) return -1;

	err = ibv_dealloc_pd(ctx->pd);
	if(err) return -1;

	err = ibv_close_device(ctx->context);
	if(err) return -1;

	return 0;
}