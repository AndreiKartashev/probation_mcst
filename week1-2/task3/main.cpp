#include <iostream> 
#include <cstring>
#include <vector>
#include <time.h> 

#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/wait.h>   
 
int main(int argc, char *argv[]) 
{ 
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0]  << " -time <time> programm [args...]";
        return 1;
    }

    struct timespec start, end;  
    int time_to_sleep = 0;
    std::vector<char*> args; 
 
    // Разбор командной строки 
    for (int i = 1; i < argc; ++i) 
    { 
        if (strcmp(argv[i], "-time") == 0)
        {
            if (i + 1 < argc)
            {
                time_to_sleep = std::atoi(argv[i + 1]);
                i++;
            }
            else
            {
                std::cerr << "Usage: -time <time> \n";
                return -1;
            }
        } 
        else
        {
            args.push_back(argv[i]); 
        }
    } 

    args.push_back(nullptr);


    int sv[2]; // Дескрипторы для socketpair

    // Создание socketpair 
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) 
    { 
        std::cerr << "socketpair eror" << '\n'; 
        return 1; 
    } 
 

    clock_gettime(CLOCK_MONOTONIC, &start); 

    
    pid_t pid = fork(); 
    if (pid == -1) 
    { 
        std::cerr << "fork error" << '\n'; 
        return 1; 
    } 
    else if (pid > 0) 
    { 
        // Родительский процесс 
        close(sv[1]); // Закрытие неиспользуемого конца 
 
        sleep(time_to_sleep); 
 
        // Отправка сигнала дочернему процессу 
        char msg = 'e'; 
        write(sv[0], &msg, sizeof(msg));  
    } 
    else 
    { 
        // Дочерний процесс 
        close(sv[0]); // Закрытие неиспользуемого конца 
 
         
        char buf; 
        read(sv[1], &buf, sizeof(buf)); 
 
        // Проверка полученного сигнала и выполнение exec 
        if (buf == 'e') 
        {  
            char* c_args[args.size()];
            std::copy(args.begin(), args.end(), c_args);


            std::cout << "exec start\n";

            execvp(c_args[0], &c_args[0]);

            for (int i = 0; i < args.size(); i++)
            {
                std::cout << c_args[i] << ' ';
            }

            std::cout << "exec failed" << '\n'; 
            return 1; 
        } 
    } 
 
    int status; 
    waitpid(pid, &status, 0);

    clock_gettime(CLOCK_MONOTONIC, &end);


    if (WIFEXITED(status)) 
    {
            
        long long int elapsed_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000'000; 
        std::cout << elapsed_time_ms << " milliseconds have passed" << std::endl; 
    } 
    else 
    { 
        std::cout << "proc failed" << std::endl; 
    }

    return 0; 
}