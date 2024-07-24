#include <iostream> 
#include <cstring>
#include <vector>
#include <map>


#include <time.h> 

#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/wait.h>   
#include <linux/perf_event.h> 
#include <sys/ioctl.h> 
#include <sys/syscall.h> 


// Таблица сопоставлений имен событий и их конфигураций
const std::map<std::string, __u64> EVENT_MAP = {
    {"cpu-cycles", PERF_COUNT_HW_CPU_CYCLES},
    {"instructions", PERF_COUNT_HW_INSTRUCTIONS},
    {"cache-references", PERF_COUNT_HW_CACHE_REFERENCES},
    {"cache-misses", PERF_COUNT_HW_CACHE_MISSES},
    {"branch-instructions", PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
    {"branch-misses", PERF_COUNT_HW_BRANCH_MISSES},
    {"bus-cycles", PERF_COUNT_HW_BUS_CYCLES},
    {"stalled-cycles-frontend", PERF_COUNT_HW_STALLED_CYCLES_FRONTEND},
    {"stalled-cycles-backend", PERF_COUNT_HW_STALLED_CYCLES_BACKEND},
    {"ref-cpu-cycles", PERF_COUNT_HW_REF_CPU_CYCLES}
};
 
int main(int argc, char *argv[]) 
{ 
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0]  << " -time <time> -count <event_name> programm [args...]";
        return 1;
    }


    struct timespec start, end;  
    int time_to_sleep = 0;

    char *event_name = "";
     
    

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
        else if (strcmp(argv[i], "-count") == 0)
        {
            if (i + 1 < argc)
            {
                event_name = argv[i + 1];
                i++;
            }
            else
            {
                std::cerr << "Usage: -count <event_name> \n";
                return -1;
            }
        }
        else
        {
            args.push_back(argv[i]); 
        }
    } 

    args.push_back(nullptr);


    // Поиск конфигурации события по имени
    auto it_event_name = EVENT_MAP.find(event_name);
    if (it_event_name == EVENT_MAP.end()) 
    {
        std::cerr << "Unsupported event: " << event_name << std::endl;
        return 1;
    }

    // Настройка атрибутов perf
    struct perf_event_attr pe; 
    memset(&pe, 0, sizeof(struct perf_event_attr)); 
    pe.type = PERF_TYPE_HARDWARE; 
    pe.config = it_event_name->second;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    

    int sv[2]; // Дескрипторы для socketpair

    // Создание socketpair 
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) 
    { 
        std::cerr << "socketpair eror" << '\n'; 
        return 1; 
    } 


    // Создание счетчика событий
    int fd = -1; 

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

        fd = syscall(__NR_perf_event_open, &pe, pid, -1, -1, 0);
        if (fd == -1) 
        {
            std::cerr << "Error opening perf event: " << strerror(errno) << std::endl;
            return 1;
        }
                    
        sleep(time_to_sleep); 
 
        // Отправка сигнала дочернему процессу 
        char msg = 'e'; 
        write(sv[0], &msg, sizeof(msg));
        write(sv[0], &fd, sizeof(fd));
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0); 
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
            
            read(sv[1], &fd, sizeof(fd));

            ioctl(fd, PERF_EVENT_IOC_RESET, 0); 


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

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);


    if (WIFEXITED(status)) 
    {
        long long count = -1; 
        read(fd, &count, sizeof(long long));   
        std::cout << "Counted " << event_name << ": " << count << std::endl;

        long long int elapsed_time_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000'000; 
        std::cout << elapsed_time_ms << " milliseconds have passed" << std::endl; 
    } 
    else 
    { 
        std::cout << "proc failed" << std::endl; 
    }


    close(fd);
    return 0; 
}