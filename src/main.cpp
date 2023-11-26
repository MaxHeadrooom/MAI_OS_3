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

    // First fork
    int id = fork();

    if (id == -1) 
    {  // fork error
        return 2;
    }
    else if (id == 0) 
    {  // First child process
        // Second fork
        int child_id = fork();

        if (child_id == -1) 
        {  // fork error
            return 2;
        }
        else if (child_id == 0) 
        {  // Grandchild process
            execl("./calculator", "./calculator", "mmap_sem", NULL);
            return 3;
        }
        else 
        {  // First child process
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
            waitpid(child_id, &status, 0);  // Waiting for the grandchild process to finish

            if (status != 0)
                perror("Grandchild process exited with an error");

            return status;
        }
    }
    else 
    {  // Parent process
        // Second fork
        int child_id = fork();

        if (child_id == -1) 
        {  // fork error
            return 2;
        }
        else if (child_id == 0) 
        {  // Second child process
            execl("./calculator", "./calculator", "mmap_sem", NULL);
            return 3;
        }
        else 
        {  // Parent process
            waitpid(0, NULL, 0);  // Waiting for the first child process to finish
            waitpid(child_id, NULL, 0);  // Waiting for the second child process to finish
            sem_close(sem);

            return 0;
        }
    }
}
