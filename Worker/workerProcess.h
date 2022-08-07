#ifndef _WORKER_h_
#define _WORKER_h_

#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

static const char* MAIN_CONTAINER_NAME="maim_cont";
void worker_proccessUserReqeust();
void worker_closeUnusedPipe();
void worker_connectPipeChannel();

char** worker_splitLineToArgumentsArry(const char* argumentString,size_t *resSize);

#endif