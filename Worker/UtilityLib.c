#include "UtilityLib.h"
#include <unistd.h>
#include <ctype.h>
#include <json-c/json.h>
#include <zip.h>
#include <signal.h>
// #include "workerProcess.h"
#include "sharedResource.h"
#include <errno.h>
/*function that not reveal to user,inner function*/
void getEpsValueFromJson(jsonObj* yearlyEPSdb,size_t arraySize,int currentStockYear,int* currentYearEps,float *epsValue,size_t *jsonArrayIndex);
void getStockDataFromWeb(char* name);
Stock* importStockDataFromFile(const char* StockName);
bool getStockAndEPSjsonDB(jsonObj** stockDB,jsonObj** epsDB,const char* stockName);
void setMonthAttributesFromJson(MonthlyStockData* month, const jsonObj* mounthJsonData);
void getYearAndMonthFromString(const char* date,int* year,int* month);
void getYearValueFromEpsAttribute(jsonObj* yearly_array_obj,int* year);

//debug func
void printDataYearly(YearlyStockData* stkYear);
void printDataMonthly(MonthlyStockData* stk,int month);



char* appd3Str(const char* firstStr, const char* secStr, const char* thrdStr)//append secStr to firstStr
{
    size_t resLen = strlen(firstStr) + strlen(secStr)+strlen(thrdStr);
    char* res = (char*)malloc((resLen+1) * sizeof(char));
    check_allocation(res, "appdStr-res");
    res[0] = '\0';
  
    strcpy(res, firstStr);
    strcat(res, secStr);
    strcat(res, thrdStr);
    //printf("res is %s\n",res);
    return res;
}


void getYearAndMonthFromString(const char* date,int* year,int* month)
{
    char* temp=strdup(date);
    check_allocation(temp,"getYearAndMonthFromString");
    char* yearStr=strtok(temp,"-");
    char* monthStr=strtok(NULL,"-");

    if(yearStr)
        *year=atoi(yearStr);
    if(monthStr)
        *month=atoi(monthStr);
    free(temp);
   
}
void getStockDataFromWeb(char* name) //uses bash script to get .stock file
{
 
    char* scptArg = appd3Str(scrptLocation,name,"");
    system(scptArg);
    free(scptArg);
    
}

Stock** reallocArray(Stock** arry,size_t oldSize,size_t newSize)
{

    Stock**newArr = (Stock**)malloc(newSize * sizeof(Stock*));
    size_t i;

    // check if memory allocation succeeded
    if (newArr)
    {
        // copy arr into newArr
        for (i = 0; i < oldSize && i < newSize; i++)
            newArr[i] = arry[i];

        //the new size is greater then old
        for (i = oldSize; i < newSize; i++)
                newArr[i]=NULL;
            
        //the new size is smaller then old
         for (i = newSize; i < oldSize; i++)
           freeStockData(arry[i]);

    
    }

    return newArr;
}


void getEpsValueFromJson(jsonObj* yearlyEPSdb,size_t arraySize,int currentStockYear,int* currentYearEps,float *epsValue,size_t *jsonArrayIndex)
{
   
    

     jsonObj* yearly_array_obj , *eps_array_obj;
        // get the name attribute in the i-th object
    if(currentStockYear<*currentYearEps&&(++*jsonArrayIndex)<arraySize)
    {
    
        yearly_array_obj = json_object_array_get_idx(yearlyEPSdb, *jsonArrayIndex);
        getYearValueFromEpsAttribute(yearly_array_obj,currentYearEps);
        if(json_object_object_get_ex(yearly_array_obj,"reportedEPS",&eps_array_obj))
            *epsValue= json_object_get_double(eps_array_obj);
       // printf("eps is:%s",json_object_get_string(yearly_array_obj));
        while (currentStockYear<*currentYearEps&&(*jsonArrayIndex++)<arraySize)
        {
             yearly_array_obj= json_object_array_get_idx(yearlyEPSdb, *jsonArrayIndex);
             getYearValueFromEpsAttribute(yearly_array_obj,currentYearEps);
            if(json_object_object_get_ex(yearly_array_obj,"reportedEPS",&eps_array_obj))
                *epsValue= json_object_get_double(eps_array_obj);

        }

    }
      

}




void getYearValueFromEpsAttribute(jsonObj* yearly_array_obj,int* year)
{
      jsonObj *date_array_obj;
      int dummy;
    if(json_object_object_get_ex(yearly_array_obj,"fiscalDateEnding",&date_array_obj))
    {
        const char* dateString=json_object_get_string(date_array_obj);
        getYearAndMonthFromString(dateString,year,&dummy);
    }

}


void setMonthAttributesFromJson(MonthlyStockData* month, const jsonObj* mounthJsonData)
{
    jsonObj* currentLineData;
    if(json_object_object_get_ex(mounthJsonData, "1. open",&currentLineData)) 
        month->open=json_object_get_double(currentLineData);
    if(month->open==0)
            return;
    if(json_object_object_get_ex(mounthJsonData, "2. high",&currentLineData)) 
        month->high=json_object_get_double(currentLineData);
    if(json_object_object_get_ex(mounthJsonData, "3. low",&currentLineData)) 
        month->low=json_object_get_double(currentLineData);
    if(json_object_object_get_ex(mounthJsonData, "4. close",&currentLineData)) 
        month->close=json_object_get_double(currentLineData);
    if(json_object_object_get_ex(mounthJsonData, "5. volume",&currentLineData)) 
        month->volume=json_object_get_double(currentLineData);


}
bool getStockAndEPSjsonDB(jsonObj** stockDB,jsonObj** epsDB,const char* stockName)
{

    char* stockFileNameWithLocation = appd3Str(databasePath,stockName, ".stock");
    char* espFileNameWithLocation=NULL;
  
    *stockDB=json_object_from_file(stockFileNameWithLocation);
    
    free(stockFileNameWithLocation);
    if(*stockDB==NULL)
        fprintf(stderr,"ERROR! %s when loading stock database %s ,please check the %s.log file\n\n", json_util_get_last_err(),stockName,stockName);
    else
        { 
        espFileNameWithLocation=appd3Str(databasePath,stockName, ".esp");
        *epsDB=json_object_from_file(espFileNameWithLocation);
        free(espFileNameWithLocation);
        if(epsDB==NULL)
            {
            json_object_put(*stockDB);
            fprintf(stderr,"ERROR! %s when loading stock EPS %s ...\nplease check the %s.log file\n\n", json_util_get_last_err(),stockName,stockName);
            } 
        }

    return (*epsDB)&&(*stockDB);
}
void initStockAttributes(Stock* stk){

    stk->name=NULL;
    stk->yearsSize=YEARS_RANGE+1;
        //init loop
    for (int i = 0; i < stk->yearsSize; i++) {
        //initiailizing year's attributes
        stk->years[i].year = 0;
        for (int y = 0; y < MONTH_SIZE; y++) {
            //initiailizing month's attributes
            stk->years[i].month[y].open = stk->years[i].month[y].high = stk->years[i].month[y].low =
                stk->years[i].month[y].close = stk->years[i].month[y].volume = stk->years[i].month[y].eps = 0;
        }
    }
   
}

Stock* importStockDataFromFile(const char* StockName)
{
    jsonObj* stockDBjsonFormat=NULL,*epsDBjsonFormat=NULL,*allMonthDBStock=NULL,*epsJsonArry=NULL;
    Stock* res = NULL;
    size_t epsArraySize=0,jsonArrayIndex=-1;
    float epsValue=0;
    int currMonthIndex, currYearIndex, currYearEps,currnetYearToday;
    //float tempEps=0, dummy;

   if(!getStockAndEPSjsonDB(&stockDBjsonFormat,&epsDBjsonFormat,StockName))
        return NULL;
  
    res = (Stock*)malloc(sizeof(Stock));
    check_allocation(res, "importStockDataFromFile-res");
   
    initStockAttributes(res);
    //each cell represents a year(index=2022-year) col = month-1
    epsJsonArry=json_object_object_get(epsDBjsonFormat,"annualEarnings");
    if(epsJsonArry!=NULL)
        epsArraySize=json_object_array_length(epsJsonArry);

    
    res->name = strdup(StockName);
    check_allocation(res->name, "getAllStockDataFromFiles- res->name ");

 
    allMonthDBStock=json_object_object_get(stockDBjsonFormat,"Monthly Time Series");
    if(allMonthDBStock==NULL)
    {
        fprintf(stderr,"error reading json file %s\n",StockName);
         return NULL;
    }
       
    struct json_object_iterator jsonMonthItr=json_object_iter_begin(allMonthDBStock);
    struct json_object_iterator endOfStockDB=json_object_iter_end(allMonthDBStock);
    
    const char* dateString=json_object_iter_peek_name(&jsonMonthItr);
    getYearAndMonthFromString(dateString,&currYearIndex,&currMonthIndex);
    currnetYearToday=currYearIndex;
    currYearEps=currYearIndex+1;//for make 'getEpsValueFromJson' get the first value of eps, aka  currYearIndex<currYearEps
    while(!json_object_iter_equal(&jsonMonthItr,&endOfStockDB)&&(currnetYearToday-currYearIndex) < res->yearsSize)
    {
        
        getEpsValueFromJson(epsJsonArry,epsArraySize,currYearIndex,&currYearEps,&epsValue,&jsonArrayIndex);
        res->years[currnetYearToday-currYearIndex].year =currYearIndex ;
        jsonObj* currentMonthJsonData=json_object_iter_peek_value(&jsonMonthItr);
       //printf("data is:%s",json_object_get_string(currentMonthJsonData));
      
        setMonthAttributesFromJson(&res->years[currnetYearToday-currYearIndex].month[currMonthIndex - 1], currentMonthJsonData);
        if(currYearIndex==currYearEps)
            res->years[currnetYearToday-currYearIndex].month[currMonthIndex - 1].eps=epsValue;
        json_object_iter_next(&jsonMonthItr);
        if(!json_object_iter_equal(&jsonMonthItr,&endOfStockDB))
       {
        const char* dateString=json_object_iter_peek_name(&jsonMonthItr);
        getYearAndMonthFromString(dateString,&currYearIndex,&currMonthIndex);
       } 
       
    }
 //  printAllStock(res);
  json_object_put(stockDBjsonFormat);
  json_object_put(epsDBjsonFormat);
        return res;

}

Stock* getUpdatedStockData(char* name){
    Stock *res;

    getStockDataFromWeb(name);
    res=importStockDataFromFile(name);
    deleteTempDatabaseFile(name,".stock");
    deleteTempDatabaseFile(name,".esp");
    return res;
}
 void convertStringToUpperCase(char* str)
{
  
    for(int i=0;str[i]!='\0';i++)  
        str[i]=toupper(str[i]);
}
void deleteTempDatabaseFile(const char* stockName,const char* extension)
{
     char* fileNameWithLocation=appd3Str(databasePath,stockName,extension);
    remove(fileNameWithLocation);
    free(fileNameWithLocation);


}
/*this function check if memory allocation failed
output:memory allocation failed*/
void checkFileOpen(FILE* fp, char* fileName)
{

    if (!fp)
    {
        
        fprintf(stderr, "Error:'%m' when trying to opening the file:%s\n", fileName);
        raise(SIGABRT);
    }
}



/*this function check if memory allocation failed
output:memory allocation failed*/
void check_allocation(void* arr, char* func_name)
{

    if (!arr)
    {
        fprintf(stderr, "Memomy allocation falied in:%s....\nEXITING!\n", func_name);
        raise(SIGABRT);

    }

 }

void freeStockData(Stock* stk)
{
    if(stk)
        {
            free(stk->name);
            free(stk);
            stk = NULL;
        } 
}


void freeStockArry(Stock** arry,size_t arrySize){

    for(size_t i=0;i<arrySize;i++)
         freeStockData(arry[i]);
}
/*********************debuging func***************************/
void printAllStock(Stock* stk)
{
    if(stk)
    {
        printf("########## stock name:%s #########################\n",stk->name);
        for(int i=0;i<stk->yearsSize;i++)
            {
            printf("year is:%d\n",stk->years[i].year);
            printDataYearly(stk->years+i);
            }

    }



}

void printDataYearly(YearlyStockData* stkYear)
{
int i;
   for(i=MONTH_SIZE-1;i>=0;i--)
    if(stkYear->month[i].open>0)
        printDataMonthly(stkYear->month,i);

}


void printDataMonthly(MonthlyStockData* stk,int month)
{
    printf(" month:%d; open: %.2f;high:%.2f; low:%.2f; close: %.2f; eps:%.2f; volume:%lu;\n", month+1,stk[month].open,
        stk[month].high, stk[month].low,
        stk[month].close, stk[month].eps,stk[month].volume);
}
