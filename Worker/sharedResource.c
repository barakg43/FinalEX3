
#include "sharedResource.h"
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
int	ExitStatus = 0;


const size_t COMMAND_MAX_LEN=256;
const int STDOUT_BUFFER_SIZE=4096;
int worker_process_IN_fd=0;
int worker_process_OUT_fd=0;
int worker_process_ERR_fd=0;
int worker_process_finish_signal_fd=0;

void cleanup(){
	

	close(worker_process_IN_fd);
	close(worker_process_OUT_fd);
	close(worker_process_ERR_fd);
	close(worker_process_finish_signal_fd);

}