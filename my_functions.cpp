#include "headFile.h"

void childProcess(char *str, int N, int L, float lamda)
{

    srand(time(0));

    sem_t *child_cs = sem_open("child_cs", O_RDWR); // open sems
    sem_t *signalParent = sem_open("signalParent", O_RDWR);
    sem_t *signalChild = sem_open("signalChild", O_RDWR);

    string server_s = "server_wait_" + to_string(N); // use N to create unique sems for every client
    string child_s = "child_wait_" + to_string(N);

    sem_unlink(server_s.c_str());
    sem_unlink(child_s.c_str());

    sem_t *server_wait = sem_open(server_s.c_str(), O_CREAT, SEM_PERMS, 1); // create child specific sems
    sem_t *child_wait = sem_open(child_s.c_str(), O_CREAT, SEM_PERMS, 0);

    if (child_cs == SEM_FAILED) // check if sems fail, then error and exit
    {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    if (signalParent == SEM_FAILED)
    {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    if (signalChild == SEM_FAILED)
    {
        perror("sem_open(3) failed");
        exit(EXIT_FAILURE);
    }

    client_info cInfo; // create client info
    cInfo.lines = 0;
    cInfo.files = 0;

    struct timeval tval_after;
    vector<struct timeval> times; // vector to keep time between L requests - it'll have L-1 elements
    gettimeofday(&tval_after, NULL);

    for (int i = 0; i < L; i++) // process client's N requests
    {
        struct timeval tval_before, tval_result;
        gettimeofday(&tval_before, NULL);
        timersub(&tval_before, &tval_after, &tval_result);
        if (i != 0) // the first request won't be added because it doesn't have any request before
            times.push_back(tval_result);

        struct timeval tval_random;
        gettimeofday(&tval_random, NULL);
        srand(tval_random.tv_usec); // randomize seed

        //  shmget returns an identifier in shmid
        int shmid = shmget(N + 1, sizeof(char) * LINE_LENGTH, 0666 | IPC_CREAT); // create unique shared memory for every client
        if (shmid < 0)
            perror("shmid");

        char *str_local = (char *)shmat(shmid, (void *)0, 0); // shmat to attach to shared memory

        sem_wait(child_cs); // enter critical section

        // random file name , random lines
        requestInfo rInfo;

        DIR *dir;
        struct dirent *de;
        char *fileNames[100];
        int fileIndex = 0;

        dir = opendir(DIRECTORY); // choose a random file from the given directory
        while (dir)
        {
            de = readdir(dir);
            if (!de)
                break;
            if (de->d_type == 8)
            {
                fileNames[fileIndex] = de->d_name; // keep all the file names
                fileIndex++;
            }
        }
        closedir(dir);

        int random = rand() % fileIndex;           // randomize the selection
        strcpy(rInfo.fileName, fileNames[random]); // store selected file's name

        if (find(cInfo.fileNames.begin(), cInfo.fileNames.end(), rInfo.fileName) == cInfo.fileNames.end()) // check if the new selected file was already used in order to not to count it again
        {
            cInfo.fileNames.push_back(rInfo.fileName);
            cInfo.files++;
        }

        char file_[1000] = DIRECTORY; // find the file and count its line in order to set the upper limit for last line's random selection
        strcat(file_, rInfo.fileName);
        std::ifstream inFile(file_);
        int UPPER = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n') + 1; // upper line limit to read from a file - counts file lines

        // generate random first and last line to read
        rInfo.firstLine = (rand() % (UPPER - LOWER + 1)) + LOWER;

        if (rInfo.firstLine == UPPER)
            rInfo.lastLine = UPPER;
        else
        {
            int lower = rInfo.firstLine + 1;
            rInfo.lastLine = (rand() % (UPPER - lower + 1)) + lower;
        }

        rInfo.N = N;
        rInfo.L = i;

        memcpy(str, (void *)&rInfo, sizeof(requestInfo)); // copy request's info in shared memory
        sem_post(signalParent);
        sem_wait(signalChild);
        sem_post(child_cs); // exit critical section

        // Put data in file lines.txt
        FILE *file;
        file = fopen("lines.txt", "a");
        if (file == NULL)
        {
            printf("Error!");
            exit(1);
        }

        fprintf(file, "Client Number: %d\nRequest: %d\nReading from file: %s\nFirst Line: %d - Last Line: %d\n\n", rInfo.N, rInfo.L, rInfo.fileName, rInfo.firstLine, rInfo.lastLine);
        // reading data (lines) from shared memory
        for (int j = rInfo.firstLine; j <= rInfo.lastLine; j++)
        {
            sem_wait(child_wait);
            fprintf(file, "Line %d: %s", j, str_local);
            cInfo.lines++; // counts lines read by client
            sem_post(server_wait);
        }
        fprintf(file, "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n");
        fclose(file);

        if (shmdt(str_local) == -1)
        {
            perror("shmdt");
            exit(1);
        }

        gettimeofday(&tval_after, NULL); // find delay time
        timersub(&tval_after, &tval_before, &tval_result);
        request_delay_time rdt;
        rdt.requestID = rInfo.L;
        rdt.timev = tval_result;
        cInfo.rdtime.push_back(rdt); // store request's delay time with request's id

        // exponential distributed waiting time
        float p = (float)(rand() % 100) / (float)100; // random number p: 0<=p<1
        float exp_time = (-1) * lamda * log(1 - p);
        sleep(exp_time);
    }
    // create file stats.txt to present program's results
    FILE *file__;
    file__ = fopen("stats.txt", "a");
    if (file__ == NULL)
    {
        printf("Error!");
        exit(1);
    }
    fprintf(file__, "Client Number: %d\n", N);
    fprintf(file__, "READ %d lines\n", cInfo.lines);
    fprintf(file__, "From %d Files\n", cInfo.files);
    fprintf(file__, "File Names: ");
    for (int i = 0; i < cInfo.fileNames.size(); i++) // print the file names of the used files for every client
    {
        if (i != 0)
            fprintf(file__, ", %s", cInfo.fileNames[i].c_str());
        else
            fprintf(file__, "%s", cInfo.fileNames[i].c_str());
    }
    fprintf(file__, "\nTIME\n");
    for (int i = 0; i < cInfo.rdtime.size(); i++)
        fprintf(file__, "Request: %d  Time elapsed: %ld.%06ld\n", cInfo.rdtime[i].requestID, (long int)cInfo.rdtime[i].timev.tv_sec, (long int)cInfo.rdtime[i].timev.tv_usec);
    // calculate average time between L requests
    double time = 0;
    for (int i = 0; i < times.size(); i++)
        time = time + (double)times[i].tv_sec + (double)times[i].tv_usec / (double)1000000;

    time = time / (double)(L - 1);
    fprintf(file__, "Average time between requests: %06lf\n", time);

    fprintf(file__, "\n\n");
    fclose(file__);
}

void *serverThread(void *threadID)
{

    sem_t *signalChild = sem_open("signalChild", O_RDWR);
    requestInfo rInfo;
    memcpy(&rInfo, (void *)threadID, sizeof(rInfo)); // take request info from argument
    sem_post(signalChild);                           // signal child

    int shmid = shmget(rInfo.N + 1, sizeof(char) * LINE_LENGTH, 0666 | IPC_CREAT); // get child's shared memory
    if (shmid < 0)
    {
        perror("shmid");
    }

    char *str = (char *)shmat(shmid, (void *)0, 0); // shmat to attach to shared memory

    string server_s = "server_wait_" + to_string(rInfo.N); // use N to create unique sems for every client-child
    string child_s = "child_wait_" + to_string(rInfo.N);

    sem_t *server_wait = sem_open(server_s.c_str(), O_RDWR); // open child's sems
    sem_t *child_wait = sem_open(child_s.c_str(), O_RDWR);

    char pFile[1000] = DIRECTORY; // find the chosen file in order to open it
    strcat(pFile, rInfo.fileName);

    FILE *file = fopen(pFile, "r");
    if (file != NULL)
    {
        for (int j = rInfo.firstLine; j <= rInfo.lastLine; j++)
        {
            sem_wait(server_wait);
            char line[LINE_LENGTH];
            int count = 0;
            rewind(file);                                   // put file pointer in file's start
            while (fgets(line, sizeof(line), file) != NULL) // read a line
            {
                if (count == j - 1) // found the wanted line
                {
                    string temp = line;
                    memcpy(str, temp.c_str(), temp.length() + 1); // copy line in child's shared memory
                    sem_post(child_wait);
                    break;
                }
                else
                    count++;
            }
        }
    }
    else
        cout << "Error: Cannot read file " << rInfo.fileName << " !" << endl;

    fclose(file);
    return NULL;
}
