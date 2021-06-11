#include<infiniband/verbs.h>


struct ib_dev_conf{
    char* ib_devname;
    uint8_t ib_port;
	struct ibv_port_attr port_attr;
	enum ibv_transport_type transport_type;
};

struct ib_addr{
	uint16_t lid;
	uint32_t qpn;
	uint32_t psn;
	uint8_t port_num;
};

struct xhg_info{
	struct ib_addr local_addr;
	void*          mem_addr;
	uint32_t       rkey;
};

struct pingpong_context {
	struct ibv_context			*context;
	struct ib_dev_conf          *dev_conf;

	struct ibv_pd				*pd;
	struct ibv_mr				*mr;
    struct ibv_cq               *cq;
    struct ibv_qp				*qp;
	void					    *buf;
	uint64_t				    buf_size;
	struct ib_addr              local;
	struct xhg_info              remote;	

	// struct ibv_comp_channel			*channel;
	// struct ibv_ah				**ah;
	// struct ibv_qp				**qp;
	// struct ibv_srq				*srq;
	// struct ibv_sge				*sge_list;
	// struct ibv_sge				*recv_sge_list;
	// struct ibv_send_wr			*wr;
	// struct ibv_recv_wr			*rwr;
	
	// uint64_t				*my_addr;
	// uint64_t				*rx_buffer_addr;
	// uint64_t				*rem_addr;
	// uint64_t				buff_size;
	// uint64_t				send_qp_buff_size;
	// uint64_t				flow_buff_size;
	// int					tx_depth;
	// int					huge_shmid;
	// uint64_t				*scnt;
	// uint64_t				*ccnt;
	// int					is_contig_supported;
	// uint32_t                                *ctrl_buf;
	// uint32_t                                *credit_buf;
	// struct ibv_mr                           *credit_mr;
	// struct ibv_sge                          *ctrl_sge_list;
	// struct ibv_send_wr                      *ctrl_wr;
	// int                                     send_rcredit;
	// int                                     credit_cnt;
	// int					cache_line_size;
	// int					cycle_buffer;
};

static const char *portStates[] = {"Nop","Down","Init","Armed","","Active Defer"};

//struct ibv_device* ctx_find_dev(const char *ib_devname);
struct ibv_context* ib_dev_init(struct ib_dev_conf* dev_conf);

int init_ctx(struct pingpong_context *ctx);

int exchange_address(int sockfd, struct pingpong_context* ctx);

int ib_rtr(struct pingpong_context* ctx);
int ib_rts(struct pingpong_context* ctx);

int ib_read_send(struct pingpong_context* ctx);

int ib_destroy(struct pingpong_context* ctx);