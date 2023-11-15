#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <string>

int main()
{
    int memoryd;
    memoryd = open("memory.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
    ftruncate(memoryd, 1024);
    char* buffer = (char*)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, memoryd, 0);
    close(memoryd);

    sem_t* sem = sem_open("mmap_sem", O_CREAT, 0777, 0);  // Open semaphore

    if (sem == SEM_FAILED) 
    {
        perror("Could not open semaphore");
        return -1;
    }

    // Forking the process
    int id = fork();

    if (id == -1) 
    {  // fork error
        return 2;
    }
    else
        if (id == 0) 
        {  // child process
            execl("./calculator", "./calculator", "mmap_sem", NULL);
            return 3;
        }
        else 
        {  // parent process
            char c;
            c = getchar();
            size_t i = 0;

            while (c != EOF) 
            {
                buffer[i++] = c;
                c = getchar();
            }
            buffer[i] = c;

            sem_post(sem);  // Unlock semaphore
            sem_close(sem);

            munmap(buffer, 1024);

            int status;
            waitpid(0, &status, 0);  // waiting for child process to finish

            if (status != 0)
                perror("Child process exited with an error");

            return status;
        }
}