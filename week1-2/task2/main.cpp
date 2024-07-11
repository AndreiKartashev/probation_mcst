#include <iostream>
#include <time.h> 

#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 

 
int main(int argc, char *argv[]) 
{ 
    if (argc < 2) 
    { 
        std::cout << "Usage: <program> [args...]\n"; 
        return 1; 
    } 

    struct timespec start, end; 
    clock_gettime(CLOCK_MONOTONIC, &start); 
 
    pid_t pid = fork(); 
 
    if (pid == -1) 
    { 
        // Ошибка при вызове fork 
        std::cout << "fork failed" << std::endl; 
        return 1; 
    } 
    else if (pid == 0) 
    { 
        // Дочерний процесс 
        execvp(argv[1], &argv[1]); 
        //Если execvp вернул управление, значит произошла ошибка 
        std::cout << "exec failed" << std::endl; 
        return 1; 
    } 
    else 
    { 
        // Родительский процесс 
        int status; 
        waitpid(pid, &status, 0); 
 
        clock_gettime(CLOCK_MONOTONIC, &end); 

        if (WIFEXITED(status)) 
        { 
            // Процесс завершился успешно 
            
            
            long long int elapsed_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000'000; 
            std::cout << elapsed_time_ms << " milliseconds have passed" << std::endl; 
        } 
        else 
        { 
            std::cout << "proc failed" << std::endl; 
        } 
    } 
 
    return 0; 
}