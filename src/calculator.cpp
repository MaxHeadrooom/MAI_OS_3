#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <random>

std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

std::string reverse(std::string& s)
{
    std::string s1 = "";

    for (int i = 0; i < s.size(); i++)
    {
        s1 += s[s.size() - i - 1];
    }

    return s1;
}

int main(int argc, char** argv)
{
    FILE* f1 = fopen("output1.txt", "w");
    FILE* f2 = fopen("output2.txt", "w");

    int memoryd;
    memoryd = open("memory.txt", O_RDWR, 0666);
    char* buffer = (char*)mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, memoryd, 0);
    close(memoryd);

    sem_t* sem = sem_open(argv[1], O_CREAT, 0777, 0);  // Open semaphore from argument list

    if (sem == SEM_FAILED) 
    {
        perror("Could not open semaphore");
        return -1;
    }

    size_t i = 0;

    sem_wait(sem);  // Wait for semaphore to unlock and then lock it

    std::string s = "";

    while (buffer[i] != -1) 
    {
        if (buffer[i] != ' ' and buffer[i] != '\n') 
        {
            s += buffer[i];
        } else
            if (buffer[i] == '\n') 
            {
                s = reverse(s);
                s = s + '\n';
                if (rng() % 100 <= 80)
                {
                    fputs(s.c_str(), f1);
                }
                else
                {
                    fputs(s.c_str(), f2);
                }
                s = "";
            } 
        i++;
    }

    sem_close(sem);

    munmap(buffer, 1024);

    return 0;
}