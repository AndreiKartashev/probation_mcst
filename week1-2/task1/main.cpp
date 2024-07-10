#include <ctime>
#include <iostream> 
#include <vector>
#include <numeric> 
#include <cmath> 


#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/poll.h> 
 
std::vector<int> v_deviders_num(int num) 
{ 
    std::vector<int> deviders;
    deviders.push_back(num); 

    for (int i = 2; i * i <= num; ++i) 
    { 
        if (num % i == 0) 
        { 
            deviders.push_back(i);
            deviders.push_back(num / i);        
        } 
    } 
    return deviders;
}

std::vector<int> v_deviders_range(int a, int b)
{
    std::vector <int> deviders;

    for (int i = a; i <= b; i++)
    {
        std::vector <int> deviders_i = v_deviders_num(i);

        deviders.insert(deviders.end(), deviders_i.begin(), deviders_i.end());
    }

    return deviders;
}

int main(int argc, char *argv[]) 
{ 
    if (argc < 3) 
    { 
        std::cerr << "Usage: " << argv[0] << " <N> <M> <K>\n"; 
        return 1; 
    }

    int n = atoi(argv[1]); 
    int m = atoi(argv[2]); 
    int k = atoi(argv[3]); //количество процессов
 
    int pipefds[k][2]; // Массив для хранения каналов для связи с порожденными процессами 
    struct pollfd fds[k]; // Массив структур pollfd для использования poll 
 

    // Создание каналов для связи
    for (int i = 0; i < k; i++) 
    {
        if (pipe(pipefds[i]) == -1) 
        { 
            std::cout << "pipe"; 
            return 1; 
        } 
    } 
    
    std::vector<std::vector<int>> matrix(k, std::vector <int>(0, 0));

    // Создание порожденных процессов 
    for (int i = 0; i < k; i++) 
    { 
        pid_t pid = fork(); 
        if (pid == -1) 
        { 
            std::cout << "fork";; 
            return 1; 
        }
        else if (pid == 0) 
        { 
            // Дочерний процесс 
            close(pipefds[i][0]); // Закрываем неиспользуемый конец канала 

            srand(time(0));

            //случайное число от 1 до M
            int p = (rand() ^ i) % m + 1;

            write (pipefds[i][1], &p, sizeof(p));

            for (int j = 1; j <= p; j++)
            {
                std::vector<int> deviders = v_deviders_range(2, n * j + i);

 
                // Вычисление произведения логарифмов всех элементов вектора 
                long double product_logs = std::accumulate(deviders.begin(), deviders.end(), 1.0, 
                    [](long double product, int value) { 
                        return product * std::log(value); 
                    }
                ); 
            

                long double g = pow(product_logs, 1.0 / deviders.size());

                write(pipefds[i][1], &g, sizeof(g));

            }

            close(pipefds[i][1]); // Закрываем конец канала
            return 0; 
        } 
        else 
        { // Главный процесс 

            close(pipefds[i][1]); // Закрываем неиспользуемый конец канала 
            // Настройка структуры pollfd для использования poll 
            fds[i].fd = pipefds[i][0]; 
            fds[i].events = POLLIN;

        } 
    } 
 


 //ожидание всех дочерних процессов    
    for (int i = 0; i < k; i++)
    {   
        int status;
        waitpid(-1, &status, 0);
            
    }
    
    std::vector<double> answer;

    // Использование poll для ожидания данных от порожденных процессов 
    for (int n = 0; n < k;) 
    {
        int ret = poll(fds, k, -1); 
        if (ret == -1) 
        { 
            perror("poll"); 
            return 1; 
        }
        
            for (int i = 0; i < k; i++) 
            { 
                
                if ((fds[i].revents & POLLIN) || (fds[i].revents & POLLHUP))
                { 
                    std::cout << "p = ";
                    int p;
                    read(pipefds[i][0], &p, sizeof(p));
                    std::cout << p << std::endl;

                    long double buf;
                    for (int j = 1; j <= p; j++)
                    {
                        read(pipefds[i][0], &buf, sizeof(buf));

                        std::cout << buf << ' ';
                        answer.push_back(buf);
                    }
                    std::cout << '\n'; 

                    close(pipefds[i][0]);
                    n++;
                } 
            } 
    }


    std::cout << '\n';

    //вычисление общего среднего геометрического
    long double g = std::accumulate(answer.begin(), answer.end(), 1.0, 
        [](long double product, double value) { 
                return product * value; 
        }
    );

    g = pow(g, 1.0 / answer.size());

    std::cout << "g = " << g << '\n';

    return 0; 
}