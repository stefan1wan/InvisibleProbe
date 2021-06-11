
struct send_args{
	struct ibv_qp* qp;
	struct ibv_send_wr* wr;
	struct ibv_send_wr** bad_wr;
    int send_num;
    int send_batch_sz;
    int* current_num;
};

void* send_worker(void* arg);