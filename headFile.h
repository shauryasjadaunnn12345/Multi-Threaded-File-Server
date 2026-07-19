#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <time.h>
#include <dirent.h>
#include <algorithm>
#include <math.h>
#include <sys/time.h>

#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define INITIAL_VALUE 1
#define LINE_LENGTH 1000
#define LOWER 1              // lower line limit to read from a file
#define DIRECTORY "./FILES/" // file's to read directory

using namespace std;

void childProcess(char *str, int N, int L, float lamba);
void *serverThread(void *threadID);

typedef struct request_info requestInfo;
struct request_info
{
    int N; // client's id
    int L; // request's id
    char fileName[100];
    int firstLine;
    int lastLine;
};

typedef struct request_delay_time_ request_delay_time;
struct request_delay_time_
{
    timeval timev; // reuest's delay time
    int requestID;
};

typedef struct client_info clientInfo;
struct client_info
{
    int lines;
    int files;
    vector<string> fileNames;
    vector<request_delay_time> rdtime; // request id - delay time for every request
    struct timeval average_time;       // average time between requests
};
