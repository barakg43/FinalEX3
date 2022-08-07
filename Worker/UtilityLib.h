#ifndef _UtilityLib_h_
#define _UtilityLib_h_

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define YEARS_RANGE 10
#define MONTH_SIZE 12
#define CURRENT_YEAR 2022
#define FILE_OPENING_ERROR -10
#define MEMORY_ALLOCATION_FAILED -11
typedef struct json_object jsonObj;
typedef struct MonthlystockData {
  float open, high, low, close, eps;
   unsigned long int volume;//Note to self: I deleted the month attribute because it is already represented as col section
} MonthlyStockData;

typedef struct yearlyStockData {
    int year;
    MonthlyStockData month[12];
} YearlyStockData;

typedef struct stock {
    char* name;
    int yearsSize;
    YearlyStockData years[YEARS_RANGE+1];
} Stock;

static const char* scrptLocation= "./getdata.sh ";
static const char* databasePath = "./db/";
Stock* getUpdatedStockData(char* name);
void freeStockData(Stock* stk);
char* appd3Str(const char* firstStr, const char* secStr, const char* thrdStr);//append  thrdStr,secStr to firstStr
void checkFileOpen(FILE* fp, char* fileName);
void check_allocation(void* arr, char* func_name);
void convertStringToUpperCase(char* str);
void printAllStock(Stock* stk);
void freeStockArry(Stock** arry,size_t arrySize);
void deleteTempDatabaseFile(const char* stockName,const char* extension);
void initStockAttributes(Stock* stk);
Stock** reallocArray(Stock** arry,size_t oldSize,size_t newSize);
#endif /* _UtilityLib_h_ */
 