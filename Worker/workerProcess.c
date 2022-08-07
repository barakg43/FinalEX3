 #include "workerProcess.h"
#include "sharedResource.h"
#include "UtilityLib.h"
#include "stockDBHandler.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
// #include <sys/syscall.h>


#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>




const int LINE_BUFFER_SIZE=512;

Stock** stockArrayBD=NULL;
size_t sizeStockArray=0;
void worker_createPipeChannels();
void worker_printDataMonthly(MonthlyStockData* stk,int month);
bool worker_checkPrintDataArguments(size_t argSize);
void worker_printDataYearly(YearlyStockData* stkYear);
void worker_sendStockDataToMain(const char* inputBuffer);
void worker_fecthStockData(const char* input_buffer);
void worker_sendFecthedStockListToMain();
void worker_sendFinishSignalToMain();
void worker_getStringInputFromMainProcess(char* string_input,int maxSizeString);
void worker_sendSiganlThroughPipeToMain(char signal);
void worker_exitAndNotify(int exitStatus);
void worker_errorSignalsHandler(int signum);
void worker_registerSignals();

int main()
{     
    
    worker_registerSignals();
    worker_createPipeChannels();
	worker_proccessUserReqeust();



    return EXIT_SUCCESS;
}
void worker_registerSignals()
{
    int signals[]={SIGKILL,SIGINT,SIGABRT,SIGBUS, SIGFPE, SIGILL, SIGSEGV,SIGTRAP,SIGSYS};
    int size=sizeof(signals)/sizeof(int);
    for(int sig=0;sig<size;sig++)
        signal(sig,worker_errorSignalsHandler);


}
void worker_createNamedPipe(const char* path)
{
    remove(path);
   int fifo_status=mkfifo(path,0666);
   if(fifo_status<0&&errno!=EEXIST )
   {
       	fprintf(stderr,"Runtime error:: '%s' named pipe\nError:%s\n", path,strerror(errno));
        worker_exitAndNotify(EXIT_FAILURE);
   }

}
 void worker_exitAndNotify(int exitStatus)
{
        if(exitStatus==EXIT_FAILURE)
            worker_sendSiganlThroughPipeToMain(ERROR_SIGNAL);
        freeStockArry(stockArrayBD,sizeStockArray);
        sleep(2);
        cleanup();
        exit(exitStatus);
}
void worker_sendSiganlThroughPipeToMain(char signal)
{
    if(worker_process_finish_signal_fd>0)
        write(worker_process_finish_signal_fd,&signal,sizeof(signal));     

}
void worker_createPipeChannels()
{
 
    worker_createNamedPipe(worker_IN_pipe);
    worker_createNamedPipe(worker_OUT_pipe);
    worker_createNamedPipe(worker_ERR_pipe);
    worker_createNamedPipe(worker_finish_signal_pipe);

	worker_process_IN_fd=open(worker_IN_pipe,O_RDONLY);
	if(worker_process_IN_fd < 0)
	{
		fprintf(stderr,"runtime error worker_process_IN_fd;path:%s error:%s\n",worker_IN_pipe ,strerror(errno));
		worker_exitAndNotify(EXIT_FAILURE);
	}
	worker_process_OUT_fd=open(worker_OUT_pipe,O_WRONLY);
	if(worker_process_OUT_fd < 0)
	{

	fprintf(stderr,"runtime error worker_process_OUT_fd;path:%s error:%s\n",worker_OUT_pipe ,strerror(errno));
	worker_exitAndNotify(EXIT_FAILURE);
	}
	worker_process_ERR_fd=open(worker_ERR_pipe,O_WRONLY);
	if(worker_process_ERR_fd < 0)
	{

		fprintf(stderr,"runtime error worker_process_ERR_fd;path:%s error:%s\n",worker_ERR_pipe ,strerror(errno));
		worker_exitAndNotify(EXIT_FAILURE);
	}
	worker_process_finish_signal_fd=open(worker_finish_signal_pipe,O_WRONLY);
	if(	worker_process_finish_signal_fd < 0)
	{
		fprintf(stderr,"runtime error worker_process_finish_signal_fd;path:%s error:%s\n",worker_finish_signal_pipe ,strerror(errno));
		worker_exitAndNotify(EXIT_FAILURE);
	}
    dup2(worker_process_OUT_fd, STDOUT_FILENO);
    dup2(worker_process_ERR_fd, STDERR_FILENO);
  
}
void worker_errorSignalsHandler(int signum)
{
    int sig=signum;
    char *msg="Signal error is flaged,runtime error in worker!\n\n";
    write(STDOUT_FILENO,msg,strlen(msg));
    worker_exitAndNotify(EXIT_FAILURE);

}
void worker_proccessUserReqeust()
{
	char opt;
    char input_buffer[LINE_BUFFER_SIZE];
    

  
   //first extract stock current database
    stockArrayBD=unZipStocksDatabase(&sizeStockArray);


    //read() is blocking I/O
	read(worker_process_IN_fd, &opt, 1);

    while(opt!=EXIT) 
    {
        switch (opt)
        {
        case FECTH_DATA:
            worker_getStringInputFromMainProcess(input_buffer,LINE_BUFFER_SIZE);
            worker_fecthStockData(input_buffer);
            break;
        case LIST_AVAILABE_STK:  
             worker_sendFecthedStockListToMain();
            break;
        case PRINT_DATA:
            worker_getStringInputFromMainProcess(input_buffer,LINE_BUFFER_SIZE);
            worker_sendStockDataToMain(input_buffer);
            break;
        case WRITE_CSV:
            zipStocksToDB(stockArrayBD,sizeStockArray);
            break;
        case EXIT:
            worker_exitAndNotify(EXIT_SUCCESS);
        default:
            break;
        }
        fflush(stdout);
        worker_sendSiganlThroughPipeToMain(FINISH_SIGNAL);
        //read() is blocking I/O ,so the loop will wait here for input form main and dont overload the system
        read(worker_process_IN_fd, &opt, 1);
			
    }

}

void worker_sendFecthedStockListToMain()
{

    printf("Available Stock: ");
    for(size_t ind=0;ind<sizeStockArray;ind++)
    printf("%s ",stockArrayBD[ind]->name);

    putchar('\n');
}


void worker_fecthStockData(const char* input_buffer)
{
    
    size_t sizeArg,pychSize=sizeStockArray;
    char** argumentsArry=worker_splitLineToArgumentsArry(input_buffer,&sizeArg);
    Stock *res;
    
    
    printf("Start Fecthing!\n");

    for(size_t argIndex=0;argIndex<sizeArg;argIndex++)
        {
            convertStringToUpperCase(argumentsArry[argIndex]);
            if(checkIfStockExist(argumentsArry[argIndex],stockArrayBD,sizeStockArray)==STOCK_NOT_FOUND)
                {
                    if(pychSize==sizeStockArray)
                        {
                            pychSize=pychSize*2+1;
                           
                            stockArrayBD =  reallocArray(stockArrayBD,sizeStockArray,pychSize);
                            check_allocation(stockArrayBD,"worker_fecthStockData-inner realloc stockArrayBD");
                        }
                    res=getUpdatedStockData(argumentsArry[argIndex]);
                if(res)
                    {stockArrayBD[sizeStockArray++]=res;
                    printf("Fecth %s is DONE!\n",argumentsArry[argIndex]);
                    }
          
                
                }
            else
                printf("%s is found on database,fecthed from local database!\n",argumentsArry[argIndex]);
          
         
        }

        //minimizing the array to real size
        if(pychSize!=sizeStockArray)
        {
            stockArrayBD =  reallocArray(stockArrayBD,pychSize,sizeStockArray);
            check_allocation(stockArrayBD,"worker_fecthStockData-minimizing realloc stockArrayBD");
        }
             for(size_t i=0;i<sizeArg;i++)
                 free(argumentsArry[i]);

}
void worker_getStringInputFromMainProcess(char* string_input,int maxSizeString)
{
    int byteRec;
    byteRec=read(worker_process_IN_fd,string_input,maxSizeString);
	if(byteRec<0)
		fprintf(stdout,"runtime error when reading input buffer process: %s\nplease try again later..", strerror(errno));
    else
        string_input[byteRec]='\0';

}


char** worker_splitLineToArgumentsArry(const char* argumentString,size_t *resSize)
{
    char** res;
    char *token;
    *resSize=1;
   // int str_len=strlen(argumentString);
    char* temp=strdup(argumentString);
    check_allocation(temp,"splitLineToArgumentsArry-temp");
    token=strtok(temp," ");
    while(strtok(NULL," ")!=NULL)
        (*resSize)++;
 
    res=(char**)malloc((*resSize)*sizeof(char*));
    check_allocation(res,"splitLineToArgumentsArry-res");
    free(temp);
    temp=strdup(argumentString);
    check_allocation(temp,"splitLineToArgumentsArry-temp");
    token=strtok(temp," ");
    res[0]=strdup(token);
    check_allocation(res[0],"splitLineToArgumentsArry-res");
    for(size_t i=1;i<*resSize;i++)   
    {
    token=strtok(NULL," ");
    res[i]=strdup(token);
    check_allocation(res[i],"splitLineToArgumentsArry-res");
    }
       free(temp);

    return res;

}
void worker_sendStockDataToMain(const char* inputBuffer)
{
    bool foundReqYear=false;
    size_t sizeArg,stockIndex;
    char** inputSplitArg=worker_splitLineToArgumentsArry(inputBuffer,&sizeArg);
    int y,reqYear;
    if(!worker_checkPrintDataArguments(sizeArg))
        return;
    convertStringToUpperCase(inputSplitArg[0]);
    stockIndex=checkIfStockExist(inputSplitArg[0],stockArrayBD,sizeStockArray);
    if(stockIndex!=STOCK_NOT_FOUND)//only in stock exist
        {
            reqYear=atoi(inputSplitArg[1]);
        for(y=0;y< stockArrayBD[stockIndex]->yearsSize&&!foundReqYear;y++) 
           {
               if(stockArrayBD[stockIndex]->years[y].year==reqYear)//print only the requested year
            {
                worker_printDataYearly((stockArrayBD[stockIndex]->years)+y);
                foundReqYear=true;
            } 
           }
            if(!foundReqYear)
            fprintf(stderr,"the year %d not found for %s in database!\n",reqYear,inputSplitArg[0]);
        }
       else   
            fprintf(stderr,"The stock %s  not found in database!\n",inputSplitArg[0]);

            //free array after used
      
        for(size_t i=0;i<sizeArg;i++)
            free(inputSplitArg[i]);
}

bool worker_checkPrintDataArguments(size_t argSize)
{   
        if (argSize < 2)//there are not arguments
            fprintf(stderr,"missing function arguments!\n"
            "Usage:<stock name> <year> \n try again!....\n");

        else if (argSize  > 2)
            fprintf(stderr,"too many function arguments!\n"
            "Usage: <stock name> <year> \n try again!....\n");


    return argSize==2;

}

void worker_printDataYearly(YearlyStockData* stkYear)
{
int i;
   for(i=MONTH_SIZE-1;i>=0;i--)
    if(stkYear->month[i].open>0)
        worker_printDataMonthly(stkYear->month,i);

}


void worker_printDataMonthly(MonthlyStockData* stk,int month)
{
    printf("month:%d; open: %.2f;high:%.2f; low:%.2f; close: %.2f; eps:%.2f; volume:%lu;\n", month+1,stk[month].open,
        stk[month].high, stk[month].low,
        stk[month].close, stk[month].eps,stk[month].volume);

}
