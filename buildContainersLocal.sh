#!/bin/bash
#containers creation
cd ./Main
mkdir -p bin
gcc -Wno-discarded-qualifiers-g -o0 -og *.c  -o ./bin/main.o
sudo docker build -t main_stocks:1.0 -f Dockerfile.build .
cd ..

cd ./Worker
mkdir -p bin
gcc -Wno-discarded-qualifiers-g -o0 -og *.c   -lzip libjson-c.a  -o ./bin/workerProcess.o
sudo docker build -t worker_stocks:1.0  -f Dockerfile.build .


