#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "sharedResource.h"
// #include "workerProcess.h"
#include <termios.h>
//globals

void main_printPogramMenu();
void main_openPipeChannels();
void main_closeUnusedPipe();
void main_sendZipSignalToWorker();
void main_sendInputToWorker(char option);
void main_sigintHandler(int signum);
void main_sendStringInputFromUserToWorker(const char* msgToScreen);
void main_waitExitStatusOnWorker();
void main_printStreamFromWorker(int stream_fd);
void main_receiveDataFromWorker();
void main_getFinishSignalFromWorker();
char new_getch();
pid_t worker_pid;//used only in mainProcess
const int STREAM_BUFFER_SIZE=4096;
sigset_t setSignal;
const char* WORKER_CONTAINER_NAME="worker_cont";
const int LINE_BUFFER_SIZE=512;
int main()
{
	
	main_openPipeChannels();
	main_printPogramMenu();	
	printf("The Main process terminated normally.\n");
   return EXIT_SUCCESS;
}
void main_getFinishSignalFromWorker(){
	char sig,isSucces;
	isSucces = read(worker_process_finish_signal_fd, &sig, sizeof(char));
	if(isSucces>0&&sig==ERROR_SIGNAL)
	{
		main_printStreamFromWorker(worker_process_ERR_fd);
		main_printStreamFromWorker(worker_process_OUT_fd);
		sleep(3);
		cleanup();
		exit(EXIT_SUCCESS);
	}	
}
	


void main_receiveDataFromWorker()
{
		main_getFinishSignalFromWorker();
		main_printStreamFromWorker(worker_process_ERR_fd);
		main_printStreamFromWorker(worker_process_OUT_fd);
		
}


void main_sendStringInputFromUserToWorker(const char* msgToScreen)
{
	char string_input[LINE_BUFFER_SIZE];
	printf("%s\n",msgToScreen);
	fflush(stdin);
	fgets(string_input,LINE_BUFFER_SIZE,stdin);
	string_input[strlen(string_input)-1]='\0';//remove '\n' from the end
	if(write(worker_process_IN_fd,string_input,strlen(string_input))<0)
				fprintf(stdout,"runtime error when sending request to process: %s\nplease try again later\n", strerror(errno));


}


void main_sendInputToWorker(char option)
{

	

		if(write(worker_process_IN_fd,&option,1)<0)
			{fprintf(stdout,"runtime error when trying process the request: %s\nplease try again later", strerror(errno));}

			switch (option)
			{
			case FECTH_DATA:
					main_sendStringInputFromUserToWorker("please enter line of stock names \n Usage:<stock_name1> <stock_name2> <stock_name3>... \nExmple: GOOGL AAPL MSTF...");
					break;
			case PRINT_DATA:
					main_sendStringInputFromUserToWorker("please enter stock name and year\n Usage:<stock name>  <year>\nExmple: GOOGL 2020");
					break;
			default:
					break;
				}

				
	

}

void main_printPogramMenu()
{
    char opt= NOT_INIT;
    while (opt != EXIT)
    {
       
        printf("##############################\n");
        printf("#1  -Fecth stock data        #\n");
        printf("#2  -List fecthed stocks     #\n");
        printf("#3  -Print stock data        #\n");
        printf("#4  -Save all stock data     #\n");
        printf("#5  -Exit                    #\n");
        printf("##############################\n");
      
       opt=new_getch();
	   
	
        if(opt < '1' || opt>'5')
            {
                system("clear");
                fprintf(stderr, "Not valid pogram menu option\nPlease enter number between 1 to 5\n");
                opt = NOT_INIT;
				continue;
        	}
	main_sendInputToWorker(opt);
	if(opt != EXIT)
	{
	main_receiveDataFromWorker();
	}
  }
}
void main_openPipeChannels()
{

	worker_process_IN_fd=open(worker_IN_pipe,O_WRONLY);
	if(worker_process_IN_fd < 0)
	{
		fprintf(stderr,"runtime error worker_process_IN_fd;path:%s error:%s\n",worker_IN_pipe ,strerror(errno));
		exit(EXIT_FAILURE);
	}
	worker_process_OUT_fd=open(worker_OUT_pipe,O_RDONLY|O_NONBLOCK);
	if(worker_process_OUT_fd < 0)
	{
	close(worker_process_IN_fd);
	fprintf(stderr,"runtime error worker_process_OUT_fd;path:%s error:%s\n",worker_OUT_pipe ,strerror(errno));
	exit(EXIT_FAILURE);
	}
	worker_process_ERR_fd=open(worker_ERR_pipe,O_RDONLY|O_NONBLOCK);
	if(worker_process_ERR_fd < 0)
	{

		close(worker_process_IN_fd);
		close(worker_process_OUT_fd);
		fprintf(stderr,"runtime error worker_process_ERR_fd;path:%s error:%s\n",worker_ERR_pipe ,strerror(errno));
		exit(EXIT_FAILURE);
	}
	worker_process_finish_signal_fd=open(worker_finish_signal_pipe,O_RDONLY);
	if(	worker_process_finish_signal_fd < 0)
	{

		close(worker_process_IN_fd);
		close(worker_process_OUT_fd);
		close(worker_process_ERR_fd);
		fprintf(stderr,"runtime error worker_process_finish_signal_fd;path:%s error:%s\n",worker_finish_signal_pipe ,strerror(errno));
		exit(EXIT_FAILURE);
	}



}

 void main_printStreamFromWorker(int stream_fd)
 {
	const int chunck = 256;
	char buffer[chunck+1];
	char streamBuffer[STREAM_BUFFER_SIZE+1];
	ssize_t bytes = 0;
	int currentSize = 0;
	do
	{
		if(STREAM_BUFFER_SIZE-currentSize<chunck)
		{
			streamBuffer[currentSize]='\0';
			//fprintf(stderr,"%s",streamBuffer);//for unbuffer print 
			write(STDERR_FILENO,streamBuffer,currentSize);
			currentSize=0;
			streamBuffer[0]='\0';
		}
		
		bytes = read(stream_fd, buffer, chunck);

		// fprintf(stderr,"byte:%ld\n",bytes);
		if(bytes >0)
		{
		buffer[bytes]='\0';
        for (size_t i = 0; i < strlen(buffer); i++)
            streamBuffer[currentSize++] = buffer[i];
		buffer[0]='\0';}
	}
	while(bytes >=0/*&&errno!=EAGAIN*/);

	if(currentSize>0)
	{
			streamBuffer[currentSize]='\0';
			write(STDERR_FILENO,streamBuffer,currentSize);;//for unbuffer print 		
	}
	
 }


char new_getch() {
    char buf = 0;
    struct termios old = { 0 };
    fflush(stdout);
    if (tcgetattr(0, &old) < 0) perror("tcsetattr()");
    old.c_lflag    &= ~ICANON;   // local modes = Non Canonical mode
    old.c_lflag    &= ~ECHO;     // local modes = Disable echo. 
    old.c_cc[VMIN]  = 1;         // control chars (MIN value) = 1
    old.c_cc[VTIME] = 0;         // control chars (TIME value) = 0 (No time)
    if (tcsetattr(0, TCSANOW, &old) < 0) perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0) perror("read()");
    old.c_lflag    |= ICANON;    // local modes = Canonical mode
    old.c_lflag    |= ECHO;      // local modes = Enable echo. 
    if (tcsetattr(0, TCSADRAIN, &old) < 0) perror ("tcsetattr ~ICANON");
    return buf;
 }