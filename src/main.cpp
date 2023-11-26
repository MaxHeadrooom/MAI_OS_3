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
    memoryd = open(
        "memory.txt",      // Имя файла
        O_RDWR | O_CREAT | O_TRUNC,  // Флаги открытия файла (чтение и запись, создание файла, обрезать его до нулевой длины)
        0666               // Режим доступа к файлу (права доступа)
    );

    ftruncate(memoryd, 1024);//обрезание memory.txt до 1024 байт

    char* buffer = (char*)mmap(
        NULL,        // Адрес начала отображения (NULL означает, что ядро само выбирает адрес)
        1024,        // Размер отображаемой области в байтах
        PROT_READ | PROT_WRITE,  // Разрешения на чтение и запись
        MAP_SHARED,  // Общий режим отображения (изменения видны другим процессам)
        memoryd,     // Файловый дескриптор, с которым связано отображение
        0            // Смещение в файле (0 означает отображение с начала файла)
    );

    close(memoryd);

    sem_t* sem = sem_open("mmap_sem", O_CREAT, 0777, 0);  // Open semaphore
    //"mmap_sem": Это строка, представляющая имя семафора. В данном случае, семафор называется "mmap_sem".
    //O_CREAT: Флаг, указывающий, что семафор должен быть создан, если его нет. Если семафор существует, он будет открыт.
    //0777: Это восьмеричное число, представляющее режим доступа к семафору. Здесь 0777 означает, что семафор доступен для чтения, записи и выполнения для всех пользователей.
    //0: Это начальное значение семафора. В данном случае, семафор инициализируется значением 0.

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
            // "./calculator": Это имя исполняемого файла, который будет запущен с помощью execl.
            // "./calculator": Это аргумент, передаваемый в запускаемую программу в качестве аргумента командной строки.
            // "mmap_sem": Это второй аргумент, передаваемый в запускаемую программу в качестве аргумента командной строки.
            // NULL: Это последний аргумент для execl, который указывает на конец списка аргументов.
            return 3;
        }
        else 
        {  // First child process
            char c;
            c = getchar();
            int i = 0;

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
