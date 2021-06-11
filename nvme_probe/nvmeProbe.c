#include "utils.h"

void Probe(void);

#define CPU_HZ 3600000000.0 // CHANGE: CPU clock frequency of your machine;in Linux, you can "cat /proc/cpuinfo" to see it;
#define Dot_Num 0x800000    // CHANGE: the read operations
#define LOG_NAME "records.bin"
#define LBA_NUM 8
#define BUFFER_SIZE (LBA_NUM * 512) // 4KB
#define READ_LBA 0x10000
#define CMD_BUFF_NUMS 128

struct hello_world_sequence {
	struct ns_entry	*ns_entry;
	char		*buf;
	unsigned        using_cmb_io;
	uint64_t   completed_io;
};

static void read_complete(void *arg, const struct spdk_nvme_cpl *completion)
{
	struct hello_world_sequence *sequence = arg;
	sequence->completed_io ++ ;
	if (spdk_nvme_cpl_is_error(completion)) {
		spdk_nvme_qpair_print_completion(sequence->ns_entry->qpair, (struct spdk_nvme_cpl *)completion);
		fprintf(stderr, "I/O error status: %s\n", spdk_nvme_cpl_get_status_string(&completion->status));
		fprintf(stderr, "Read I/O failed, aborting run\n");
	}
}


void Probe(void){
	// alloc memory to log the rdtscp stamp
    uint64_t* time_line = (uint64_t *)spdk_zmalloc(sizeof(uint64_t)*Dot_Num, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
	memset(time_line, 0, sizeof(uint64_t)*Dot_Num);
	volatile uint64_t p=0;

	// alloc Q pairs
	struct ns_entry		*ns_entry = g_namespaces;
    ns_entry->qpair = spdk_nvme_ctrlr_alloc_io_qpair(ns_entry->ctrlr, NULL, 0);
    if (ns_entry->qpair == NULL) {
            printf("ERROR: spdk_nvme_ctrlr_alloc_io_qpair() failed\n");
    }

    struct hello_world_sequence	sequence;
    sequence.buf = spdk_zmalloc(BUFFER_SIZE, 0x1000, NULL, SPDK_ENV_SOCKET_ID_ANY, SPDK_MALLOC_DMA);
    sequence.buf[0]=1;
	sequence.completed_io = 0;
	sequence.ns_entry = ns_entry;
    
    int rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence.buf,
                            READ_LBA,
                            LBA_NUM,
                            read_complete, &sequence, 0);

    if (rc != 0) {
        fprintf(stderr, "starting read I/O failed\n");
        exit(1);
    }

	uint64_t in_processing = 1;
    uint64_t already_completed_num = 0;
	uint64_t completion_left=Dot_Num;

	while (completion_left) {
		// wait until an operation completes
		while (sequence.completed_io == already_completed_num) {
                spdk_nvme_qpair_process_completions(ns_entry->qpair, 1);
            }
		// log rdtscp stamps
        time_line[p++] = rdtscp();

        completion_left --;
		in_processing --;
        already_completed_num++;

		// refill the send Q with read cmds
		while(in_processing < CMD_BUFF_NUMS && completion_left > 0){
            rc = spdk_nvme_ns_cmd_read(ns_entry->ns, ns_entry->qpair, sequence.buf,
                                    READ_LBA,
                                    LBA_NUM,
                                    read_complete, &sequence, 0);
            if (rc != 0) {
                fprintf(stderr, "starting read I/O failed\n");
                exit(1);
            }
            in_processing++; 
		}
    }
	
	// handle the left read commands
	while(in_processing -- ){
		while (sequence.completed_io == already_completed_num) {
                spdk_nvme_qpair_process_completions(ns_entry->qpair, 1);
            }
		already_completed_num++; 
	}
	spdk_nvme_ctrlr_free_io_qpair(ns_entry->qpair);
    
    puts("Detect done! Start analyzing..."); 


    FILE *fp = fopen(LOG_NAME,"wb");
	fwrite(time_line, sizeof(uint64_t), Dot_Num, fp);
    fclose(fp);

    puts("=========Summary==========");
    uint64_t total_gap = time_line[Dot_Num-1] - time_line[0];
    double total_gap_time = total_gap/CPU_HZ;
    double av_time = total_gap_time/(Dot_Num-1);
    double freq = (Dot_Num-1)/total_gap_time;
    double throughput = ((Dot_Num-1)*LBA_NUM/2)/total_gap_time;

    printf("Operation Average time: %.3les\n" , av_time);
    printf("IOPS: %15lf\n", freq);
    printf("Throughput(GB/s): %lfM, %lfG\n", throughput/1024, throughput/1024/1024);
	return;
}

int main(int argc, char **argv)
{
	int rc;
	struct spdk_env_opts opts;
	/*
	 * SPDK relies on an abstraction around the local environment
	 * named env that handles memory allocation and PCI device operations.
	 * This library must be initialized first.
	 *
	 */
	spdk_env_opts_init(&opts);
	opts.name = "hello_world";
	opts.shm_id = 0;
	if (spdk_env_init(&opts) < 0) {
		fprintf(stderr, "Unable to initialize SPDK env\n");
		return 1;
	}
	printf("Initializing NVMe Controllers\n");

	/*
	 * Start the SPDK NVMe enumeration process.  probe_cb will be called
	 *  for each NVMe controller found, giving our application a choice on
	 *  whether to attach to each controller.  attach_cb will then be
	 *  called for each controller after the SPDK NVMe driver has completed
	 *  initializing the controller we chose to attach.
	 */
	rc = spdk_nvme_probe(NULL, NULL, probe_cb, attach_cb, NULL);
	if (rc != 0) {
		fprintf(stderr, "spdk_nvme_probe() failed\n");
		cleanup();
		return 1;
	}

	if (g_controllers == NULL) {
		fprintf(stderr, "no NVMe controllers found\n");
		cleanup();
		return 1;
	}

	printf("Initialization complete.\n");
    Probe();
	cleanup();
	return 0;
}
