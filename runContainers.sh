#!/bin/bash


#containers running
sudo docker pull barakg43/stock_fetcher:main
sudo docker pull barakg43/stock_fetcher:worker
echo 'stating worker container in background'
sudo docker run --name worker_stocks -v /var/tmp/pipes:/pipes/ barakg43/stock_fetcher:worker & # '&' making the command to run in background without blocking current bash window
echo 'stating main container'
sleep 2
clear
sudo docker run -it --name main_stocks -v /var/tmp/pipes:/pipes/ barakg43/stock_fetcher:main

