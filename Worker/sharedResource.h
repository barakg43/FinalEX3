#ifndef ___SHARED_RESOURCE_H___
#define  ___SHARED_RESOURCE_H___
int	ExitStatus;
int worker_process_IN_fd;
int worker_process_OUT_fd;
int worker_process_ERR_fd;
int worker_process_finish_signal_fd;
static const char* worker_IN_pipe="/pipes/worker_in_fifo";
static const char* worker_OUT_pipe="/pipes/worker_out_fifo";
static const char* worker_ERR_pipe="/pipes/worker_err_fifo";
static const char* worker_finish_signal_pipe="/pipes/worker_signal_fifo";

const int STDOUT_BUFFER_SIZE;
enum USER_OPTIONS{FECTH_DATA='1',LIST_AVAILABE_STK,PRINT_DATA,WRITE_CSV,EXIT};
enum SETTING {NOT_INIT=-1,FINISH_SIGNAL='A',ERROR_SIGNAL} ;
void cleanup();

#endif
