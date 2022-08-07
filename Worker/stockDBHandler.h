#pragma once
#include "UtilityLib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zip.h>
#include <unistd.h>

const size_t STOCK_NOT_FOUND;
static const char* databaseZipFile="stocks_db.zip";
void zipStocksToDB(Stock** stockList, size_t sizeList);
Stock** unZipStocksDatabase(size_t* size);
size_t checkIfStockExist(const char* stockName,Stock** stkArry,size_t size);
