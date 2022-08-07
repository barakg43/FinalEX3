#!/bin/bash



mkdir -p db
cd db

#Iterate every stock in input stream
for stock_name in $@ ;do
	

	#retrieving stock's info
	wget  "https://www.alphavantage.co/query?function=TIME_SERIES_MONTHLY&symbol=$stock_name&apikey=ZCRAYKTE65BME9G6"  --output-document=${stock_name}.stock  2>&1 >/dev/null | grep   "unable to resolve host address" >${stock_name}.stock

	 
	#Validating whether stock actually exists
	if  grep -q "Error Message" ${stock_name}.stock ;then
	
		echo  "`date +'%m/%d/%Y %R'`:could not fetched $stock_name monthly,stock name not found on server database " >>$stock_name.log
		echo "`date +'%m/%d/%Y %R'`:could not fetched $stock_name monthly. " >$stock_name.stock
		rm -f ${stock_name}_temp.stock
		continue
	elif  grep -q "unable to resolve host address" ${stock_name}.stock ;then	
		echo "`date +'%m/%d/%Y %R'`:could not fetched $stock_name monthly. cannot connect to Database server,check the internet connection" >>$stock_name.log
		echo "`date +'%m/%d/%Y %R'`:could not fetched $stock_name monthly. " >$stock_name.stock
		rm -f $stock_name.stock
		continue
	fi

	#retrieving eps data
		wget  "https://www.alphavantage.co/query?function=EARNINGS&symbol=$stock_name&apikey=UJYJ7WP99XT18KA6"  --output-document=$stock_name.esp &>/dev/null  
	touch $stock_name.log

	echo  "`date +'%m/%d/%Y %R'`:Fetched $stock_name monthly ">>$stock_name.log
	echo  "`date +'%m/%d/%Y %R'`:Fetched $stock_name ESP" >>$stock_name.log
	
done
