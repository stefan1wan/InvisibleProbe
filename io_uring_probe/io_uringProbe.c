#include "utils.h"

#define TEST_FILE "YOURFILE" // CHANGE: your file to read

#define CPU_HZ 2500000000.0  // CHANGE: CPU clock frequency of your machine;in Linux, you can "cat /proc/cpuinfo" to see it;
#define Dot_Num 0x10000  // CHANGE: the read operations
#define LOG_NAME "records.bin"
#define QD	128
#define BUFFER_SIZE (4096*2)
#define REG_SIZE 8

int Probe()
{	
    //  1.1 define the variables
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    int i, ret, fd;
	int fds[2];
    void *buf;
    off_t offset;
    
    //  1.2 init timeline
	uint64_t*  time_line = (uint64_t *)mmap(NULL, sizeof(uint64_t)*Dot_Num, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB | MAP_HUGE_1GB, -1, 0);
    if (time_line == (void *)(-1)) {
        perror("ERROR: mmap of array a failed! ");
        exit(1);
    }
	memset(time_line,0, Dot_Num*sizeof(uint64_t));
    uint64_t p=0;

    //  1.3 init lib_uring IORING_SETUP_SQPOLL
    ret = io_uring_queue_init(QD, &ring, IORING_SETUP_SQPOLL);
    if (ret < 0) {
		fprintf(stderr, "queue_init: %s\n", strerror(-ret));
		return 1;
	}

    //  1.4 open the file use O_DIRECT flag
    fd = open(TEST_FILE, O_RDONLY|O_DIRECT); 
        //how to generate a random file : dd if=/dev/urandom of=/home/vam/test.file count=4096 bs=4096
    if (fd < 0) {
		perror("open");
		return 1;
	}
	fds[0]=fd;
	ret = io_uring_register_files(&ring, fds, 1);
	if(ret) {
        fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
        return 1;
    }
	// 2 fill the SQ and submit;
    // 2.1 init iovecs
	struct iovec iov[REG_SIZE];
    for (i = 0; i < REG_SIZE; i++) {
        if (posix_memalign(&buf, BUFFER_SIZE, BUFFER_SIZE)) // 4K
	        return 1;
        iov[i].iov_base = buf;
        iov[i].iov_len = BUFFER_SIZE;
        memset(iov[i].iov_base, 0, BUFFER_SIZE);
    }

    ret = io_uring_register_buffers(&ring, iov, REG_SIZE);
    if(ret) {
        fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
        return 1;
    }

	buf = iov[0].iov_base;
	
    // 2.2 fill the SQ and submit;
    offset = 0;
	i = 0;
	do {
		sqe = io_uring_get_sqe(&ring);
		if (!sqe)
			break;
		io_uring_prep_read_fixed(sqe, 0, iov[i%REG_SIZE].iov_base, BUFFER_SIZE, offset, i%REG_SIZE);
		sqe->flags |= IOSQE_FIXED_FILE;
		offset += BUFFER_SIZE;
		i++;
	} while (1);

    ret = io_uring_submit(&ring); 
    if (ret < 0) {
		fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
		return 1;
	}

	// 3. probe
	uint64_t in_processing = QD; 
	uint64_t completion_left=Dot_Num;

	while(completion_left--){
		int ret_cqe = io_uring_wait_cqe(&ring, &cqe); 
		if (ret_cqe < 0) {
			fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret_cqe));
			return 1;
		}
		time_line[p++] = rdtscp();

		in_processing --;

		if (cqe->res != BUFFER_SIZE) {
			printf("cqe->res: %d\n", cqe->res);
			fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
		}
		io_uring_cqe_seen(&ring, &cqe);

		if(in_processing<QD-16){
			while(in_processing<QD){
				sqe = io_uring_get_sqe(&ring);
				if (!sqe) break;
				io_uring_prep_read_fixed(sqe, 0, iov[i%REG_SIZE].iov_base , BUFFER_SIZE, offset, i%REG_SIZE);
				sqe->flags |= IOSQE_FIXED_FILE;
				offset += BUFFER_SIZE;
				i++;
				in_processing++;
			}
			ret = io_uring_submit(&ring); 
			if (ret < 0) {
				fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
				return 1;
			}
		}
	}
	while(in_processing -- ){
		int ret_cqe = io_uring_wait_cqe(&ring, &cqe); 
		if (ret_cqe < 0) {
			fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret_cqe));
			return 1;
		}
	}

	close(fd);
	io_uring_queue_exit(&ring);

	puts("========Detect done! Start analyzing..."); 
    FILE *fp = fopen(LOG_NAME,"wb");
	fwrite(time_line, sizeof(uint64_t), Dot_Num, fp);
    fclose(fp);

	puts("=========Summary==========");
    uint64_t total_gap = time_line[Dot_Num-1] - time_line[0];
    double total_gap_time = total_gap/(double)CPU_HZ;
    double av_time = total_gap_time/(Dot_Num-1);
    double freq = (double)Dot_Num/total_gap_time;
    double throughput =(Dot_Num* (double)BUFFER_SIZE)/total_gap_time;

    printf("Operation Average time: %.3les\n" , av_time);
    printf("IOPS: %15lf\n", freq);
    printf("Throughput(GB/s) per second: %lfM, %lfG\n", throughput/1024.0/1024.0, throughput/1024.0/1024.0/1024.0 );
	return 0;
}

int main(){
	Probe();
}