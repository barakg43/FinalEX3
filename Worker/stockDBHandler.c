#define _CRT_SECURE_NO_WARNINGS
#include "UtilityLib.h"
#include "sharedResource.h"
#include "stockDBHandler.h"
#include <errno.h>
#include <signal.h>
#include "workerProcess.h"  
//Func declarations
const size_t STOCK_NOT_FOUND=-1;
long findFileSize(FILE* fileToZip);
//returns an array of stocks extracted from zip, its size is stored in "size"
FILE* exportStockToCsv(Stock* stk);
Stock* importStockFromCSV(zip_file_t* currFileInArc, zip_uint64_t bytesToBeRead,const char* stockCsvFileName);
int readStrFromCSV(zip_file_t* currFileInArc, char* dest, int* phySize, zip_uint64_t* charRead, zip_uint64_t bytesToBeRead);

size_t checkIfStockExist(const char* stockName,Stock** stkArry,size_t size)
{
       
        size_t foundIndex=STOCK_NOT_FOUND;
        for(size_t index=0;(index<size)&&foundIndex==STOCK_NOT_FOUND;index++)
       { 
               if(strcmp(stkArry[index]->name,stockName)==0)
                    foundIndex=index;
        }


        return foundIndex;
}
void zipStocksToDB(Stock** stockList,size_t sizeList){//zipping all csv files available(are written in allCSV) to stks.zip
    zip_t* fileArc;
    zip_source_t* fileToZipSource; 
    FILE *fileToZip;
    char msg_Stdout[STDOUT_BUFFER_SIZE];
    msg_Stdout[0]='\0';
  
  

    //saving current working directory


    char* zipFileWithLocation=appd3Str(databasePath,databaseZipFile,"");
    //creating an archive
    fileArc = zip_open(zipFileWithLocation, ZIP_CREATE + ZIP_TRUNCATE, NULL);
    free(zipFileWithLocation);
    if(fileArc==NULL){
        strcat(msg_Stdout,"Error! could not create an archive:");
        strcat(msg_Stdout,strerror(errno));
        write(STDERR_FILENO,msg_Stdout,strlen(msg_Stdout));
         msg_Stdout[0]='\0';
        return;
    } 
       //adding each stock from list to archive
        for(size_t i=0;i<sizeList;i++)
        {

                fileToZip = exportStockToCsv(stockList[i]);
                fileToZipSource = zip_source_filep(fileArc, fileToZip, 0, 0);
                if(fileToZipSource==NULL){

                   // printf("Error! could not use file:\n%s\n", zip_strerror(fileArc));
                    strcat(msg_Stdout,"Error! could not use file:\n");
                    strcat(msg_Stdout,zip_strerror(fileArc));
                    strcat(msg_Stdout,"\n");
                    write(STDERR_FILENO,msg_Stdout,strlen(msg_Stdout));
                    msg_Stdout[0]='\0';
                    fclose(fileToZip);
                    deleteTempDatabaseFile(stockList[i]->name,".csv");  
                    continue;
                }
                char* fileNameCSV=appd3Str(stockList[i]->name,".csv","");
                
                if(zip_file_add(fileArc, fileNameCSV, fileToZipSource, 0)<0){
                   // printf("Error! could not add file:\n%s\n", zip_strerror(fileArc));
                    strcat(msg_Stdout,"Error! could not add file:\n");
                    strcat(msg_Stdout,zip_strerror(fileArc));
                    strcat(msg_Stdout,"\n");
                    write(STDERR_FILENO,msg_Stdout,strlen(msg_Stdout));
                    msg_Stdout[0]='\0';
                    zip_source_free(fileToZipSource);
                   
                }
                deleteTempDatabaseFile(stockList[i]->name,".csv");  
        }



    //closing archive
    if(zip_close(fileArc)<0){
            strcat(msg_Stdout,"Error! could not close archive:\n");
            strcat(msg_Stdout,zip_strerror(fileArc));
            strcat(msg_Stdout,"\n");
            write(STDERR_FILENO,msg_Stdout,strlen(msg_Stdout));
            msg_Stdout[0]='\0';
       
    }

    printf("Database saved in %s%s\n",databasePath,databaseZipFile);
}

//returns an array of stocks extracted from zip, its size is stored in "size"
Stock** unZipStocksDatabase(size_t* size){
    zip_stat_t sb;
    zip_t* fileArc;
	zip_file_t* currFileInArc;
    Stock** stksArry = NULL;
    *size=0;
    char* zipFileWithLocation=appd3Str(databasePath,databaseZipFile,"");
   
    int errorCode;
    fileArc = zip_open(zipFileWithLocation, ZIP_RDONLY, &errorCode);

    if(fileArc==NULL&&errorCode==ZIP_ER_NOENT)//archive not exist yet
            return NULL;
    if(fileArc==NULL){
        fprintf(stderr,"Error! could not open the archive:%m!\n");
        return NULL;
    }
    zip_uint64_t num_entries = zip_get_num_entries(fileArc, /*flags=*/0);
	//going through each file in archive
    stksArry=(Stock**)malloc(sizeof(Stock*)*num_entries);
    check_allocation(stksArry,"unZipStocks");
    
    for(zip_uint64_t i=0;i<num_entries;i++)
    {
        const char* stockCsvFileName = zip_get_name(fileArc, i, /*flags=*/0);
        if (stockCsvFileName == NULL) {
            fprintf(stderr,"error when gatting file entries name from zip:%m!");
        }
        else
            {
            currFileInArc = zip_fopen(fileArc, stockCsvFileName, 0);
            if(zip_stat(fileArc, stockCsvFileName, 0, &sb)!=0)
                {//getting total chars to be read from file
                    fprintf(stderr,"zip_stat failure\n");
                            continue;   
                }
           stksArry[(*size)++]= importStockFromCSV(currFileInArc,sb.size,stockCsvFileName);//storing current file's data

        
            }
    }

    //closing archive
    if(zip_close(fileArc)<0){
        printf("Error! could not close archive:\n%s\n", zip_strerror(fileArc));
    }

	//minimizing stks
    if((*size)!=num_entries)
	{stksArry = reallocArray(stksArry,num_entries,*size);
    check_allocation(stksArry,"unZipStocksDatabase-realloc stks");}
    return stksArry;
}

FILE* exportStockToCsv(Stock* stk) {
	FILE *csvOut;
	int y,m;
	char* fileName = appd3Str(databasePath,stk->name,".csv");
	

	csvOut = fopen(fileName, "w+");
	checkFileOpen(csvOut,fileName);
	fprintf(csvOut, "year, month, open, high, low, close, volume, eps\n");

	for(y=0;y<stk->yearsSize;y++){
		for(m=MONTH_SIZE-1;m>=0;m--){
			if(stk->years[y].month[m].open>0)
				fprintf(csvOut, "%d, %d, %f, %f, %f, %f, %lu, %f\n", stk->years[y].year,m+1,stk->years[y].month[m].open,
				stk->years[y].month[m].high,stk->years[y].month[m].low,stk->years[y].month[m].close,
				stk->years[y].month[m].volume,stk->years[y].month[m].eps);
		}
	}
	free(fileName);
	rewind(csvOut);
    return csvOut;
}

Stock* importStockFromCSV(zip_file_t* currFileInArc, zip_uint64_t bytesToBeRead,const char* stockCsvFileName){
	int currYear, currMonth, lastYear=0, i, sPhySize=256,status=0,numberOfYears=0;
    char *currStr; 
    size_t  charRead=0;
    Stock* stk= (Stock*)malloc(sizeof(Stock));
    check_allocation(stk, "importStockDataFromFile-res");;
    int currYearToday=0;

    
    initStockAttributes(stk);
    //allocating current string from file
    currStr = (char*)malloc(sizeof(char)*sPhySize);
    check_allocation(currStr,"importStockFromCSV");
    //initiallize stock name & years
    int lenWithoutExt=strlen(stockCsvFileName)-4;
    stk->name =(char*)malloc((lenWithoutExt+1)*sizeof(char));
    check_allocation(stk->name,"importStockFromCSV-stk->name");
    strncpy(stk->name,stockCsvFileName,lenWithoutExt);
    stk->name[lenWithoutExt]='\0';
    //skipping first row in csv file(8 words)
    for(i=0;i<8;i++){
        //NOTE: Zip_fseek was not working properly
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
    }

    
    while(status==0&&numberOfYears<stk->yearsSize){//keep reading until it's not possible
	    //reading dataflow according to csv format in exportToCSV: 
	    //year[int], month[int], open[float], high[float], low[float], 
        //close[float], volume[unsigned long], eps[float]

        //retrieving current year
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        currYear = atoi(currStr);
        if(!currYearToday)
            lastYear=currYearToday= stk->years[0].year=currYear;
        if(lastYear!=currYear){
			
            stk->years[currYearToday-currYear].year = currYear;
             numberOfYears++;
             lastYear=currYear;
		}

        //retrieving current month
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        currMonth = atoi(currStr);

        //retrieving current open
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].open = atof(currStr);

        //retrieving current high
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].high  = atof(currStr);

        //retrieving current low
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].low = atof(currStr);

        //retrieving current close
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].close = atof(currStr);

        //retrieving current volume
        readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].volume = atol(currStr);

        //retrieving current eps
        status = readStrFromCSV(currFileInArc,currStr,&sPhySize,&charRead,bytesToBeRead);
        stk->years[currYearToday-currYear].month[currMonth-1].eps = atof(currStr);
       
    }
    free(currStr);
    return stk;


}














int readStrFromCSV(zip_file_t* currFileInArc, char* dest, int* phySize,zip_uint64_t *charRead, zip_uint64_t bytesToBeRead){ //storing next available string from file stream in dest 
    char c=0;
    int i=0;

    //get char by char, until a space, and combine them into a string
    while(c!=' ' && c!='\n' && (*charRead)<bytesToBeRead){
        if(zip_fread(currFileInArc, &c, sizeof(char))<0){//reading a char
                printf("Error reading!\n");
                raise(SIGABRT);
        }
        (*charRead)++;
        if(c!=' ' && c!=',' && c!='\n'){//avoiding unecessary chars
            dest[i] = c;
            i++;
        }
        if(i==(*phySize)-2){//reallocating if needed
			(*phySize)*=2;
			dest = (char*)realloc(dest,(*phySize)*sizeof(char));
			if(dest==NULL){
				printf("Memory allocation failure\n");
				raise(SIGABRT);
			}
		}
    }
    dest[i]='\0';//sealing the string

    if((*charRead)==bytesToBeRead)//if we finished reading the entire file
        return 1;
    else
        return 0;
}









