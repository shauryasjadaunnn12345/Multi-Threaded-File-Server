#include "headFile.h"

int main(int argc, char *argv[])
{
    int N = stoi(argv[1]); // read args
    int L = stoi(argv[2]);
    float lamda = stoi(argv[3]);

    sem_unlink("child_cs"); // unlinking semaphores in case they are linked
    sem_unlink("signalParent");
    sem_unlink("signalChild");

    sem_t *child_cs = sem_open("child_cs", O_CREAT, SEM_PERMS, 1);         // create semaphore and initialize in 1
    sem_t *signalParent = sem_open("signalParent", O_CREAT, SEM_PERMS, 0); // create semaphore and initialize in 0
    sem_t *signalChild = sem_open("signalChild", O_CREAT, SEM_PERMS, 0);   // create semaphore and initialize in 0

    if (child_cs == SEM_FAILED) // Check if sems fail, then error and exit
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

    key_t key = ftok("shmfile", 65); // ftok to generate unique key

    int shmid = shmget(key, sizeof(requestInfo), 0666 | IPC_CREAT); // create main shared memory

    char *str = (char *)shmat(shmid, (void *)0, 0); // shmat to attach to shared memory

    for (int i = 0; i < N; i++) // create N child processes (N clients)
        if (fork() == 0)
        {
            sem_close(child_cs); // closing sems
            sem_close(signalParent);
            sem_close(signalChild);
            childProcess(str, i, L, 1);
            return 0;
        }

    for (int i = 0; i < N * L; i++)
    {
        sem_wait(signalParent); // waiting for child to signal

        requestInfo rInfo;
        memcpy(&rInfo, (void *)str, sizeof(rInfo)); // take request info from shared memory

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, &serverThread, (void *)&rInfo); // create thread and put data: request info in the thread
    }

    pid_t wpid;
    int status = 0;
    while ((wpid = wait(&status)) > 0)
        ; // this way, the father waits for all the child processes
}