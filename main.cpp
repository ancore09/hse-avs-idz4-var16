#include <fstream>
#include <iostream>
#include <pthread.h>
#include <string>
#include <unistd.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_mutex_t mutex_console;

// размер матрицы сада
int size = 10;
// количество микросекунд, которое садовник будет тратить на работу над участком
int work_time = 5000;
// количество микросекунд, которое садовник будет тратить на перемещение по пустому участку
int skip_time = 2000;
std::string file_name = "output.txt";

// вывод матрицы сада в консоль
void printArray(int **array) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            std::cout << array[i][j] << "\t";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void printArray(int **array, std::ofstream &file) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << array[i][j] << "\t";
        }
        file << std::endl;
    }
    file << std::endl;
}

// синхронизированный вывод в консоль
void printConsole(std::string str, int **array) {
    pthread_mutex_lock(&mutex_console);
    std::cout << str << std::endl;
    printArray(array);
    pthread_mutex_unlock(&mutex_console);
}

void printFile(std::string str, int **array) {
    pthread_mutex_lock(&mutex_console);
    std::ofstream file;
    file.open(file_name, std::ios::app);
    file << str << std::endl;
    printArray(array, file);
    file.close();
    pthread_mutex_unlock(&mutex_console);
}

// садовник 1
void handleCell1(int **array, int i, int j) {
    // если участок не может быть обработан, то садовник пропускает его
    if (array[i][j] == -1) {
        // лочим мьютекс, чтобы другой садовник не начал обрабатывать этот участок
        pthread_mutex_lock(&mutex1);
        // 2 - метка первого садовника (не очень логично, но вот так)
        array[i][j] = 2;
        // разлочиваем мьютекс, чтобы другой садовник мог пройти по этому участку
        pthread_mutex_unlock(&mutex1);

        printConsole("Thread 1: cant handle cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 1: cant handle cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);

        // садовник тратит время на пропуск участка
        usleep(skip_time);

        // лочим мьютекс чтобы восстановить метку участка без конфликтов
        pthread_mutex_lock(&mutex1);
        array[i][j] = -1;
        pthread_mutex_unlock(&mutex1);
    }
    // если участок может быть обработан, то садовник обрабатывает его
    else if (array[i][j] == 0) {
        // лочим мьютекс, чтобы другой садовник не начал обрабатывать этот участок
        pthread_mutex_lock(&mutex1);
        // 2 - метка первого садовника (не очень логично, но вот так)
        array[i][j] = 2;
        printConsole("Thread 1: handling cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 1: handling cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);

        // садовник тратит время на обработку участка
        usleep(work_time);
        // 1 - метка обработанного участка
        array[i][j] = 1;
        // разлочиваем мьютекс, чтобы другой садовник мог пройти по этому участку
        pthread_mutex_unlock(&mutex1);
        // ждем 10 микросекунд, чтобы другой поток успел отреагировать на изменение мьютекса
        usleep(10);
    }
    // если участик уже обработан, то садовник пропускает его
    else if (array[i][j] == 1) {
        // лочим мьютекс, чтобы другой садовник не начал обрабатывать этот участок
        pthread_mutex_lock(&mutex1);
        // 2 - метка первого садовника (не очень логично, но вот так)
        array[i][j] = 2;
        printConsole("Thread 1: cell is already cleared [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 1: cell is already cleared [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);

        pthread_mutex_unlock(&mutex1);
        // садовник тратит время на пропуск участка
        usleep(skip_time);

        pthread_mutex_lock(&mutex1);
        array[i][j] = 1;
        pthread_mutex_unlock(&mutex1);
    }
    // если на клетке, куда переходит садовник, стоит другой, то садовник ждет, пока клетка освободится.
    else {
        printConsole("Thread 1: waiting for Thread 2 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 1: waiting for Thread 2 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);

        // смотрим на мьютекс другого садовника и ждем пока он освободится
        pthread_mutex_lock(&mutex2);
        printConsole("Thread 1: DONE waiting for Thread 2 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 1: DONE waiting for Thread 2 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        pthread_mutex_unlock(&mutex2);

        // мьютекс свободен, значит садовник может скипать этот участок
        usleep(skip_time);

    }
}

// садовник 2
void handleCell2(int **array, int i, int j) {
    if (array[i][j] == -1) {
        pthread_mutex_lock(&mutex2);
        array[i][j] = 3;
        pthread_mutex_unlock(&mutex2);

        printConsole("Thread 2: cant handle cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 2: cant handle cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);


        usleep(skip_time);

        pthread_mutex_lock(&mutex2);
        array[i][j] = -1;
        pthread_mutex_unlock(&mutex2);
    } else if (array[i][j] == 0) {
        pthread_mutex_lock(&mutex2);
        array[i][j] = 3;
        printConsole("Thread 2: handling cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 2: handling cell [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        usleep(work_time);
        array[i][j] = 1;
        pthread_mutex_unlock(&mutex2);
        usleep(10);
    } else if (array[i][j] == 1) {
        pthread_mutex_lock(&mutex2);
        array[i][j] = 3;
        pthread_mutex_unlock(&mutex2);
        printConsole("Thread 2: cell is already cleared [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 2: cell is already cleared [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        usleep(skip_time);
        pthread_mutex_lock(&mutex2);
        array[i][j] = 1;
        pthread_mutex_unlock(&mutex2);
    } else {
        printConsole("Thread 2: waiting for Thread 1 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 2: waiting for Thread 1 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);

        pthread_mutex_lock(&mutex1);
        printConsole("Thread 2: DONE waiting for Thread 1 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        printFile("Thread 2: DONE waiting for Thread 1 [" + std::to_string(i) + "][" + std::to_string(j) + "]", array);
        pthread_mutex_unlock(&mutex1);
        usleep(skip_time);

    }
}

// обход садовника 1
void firstThread(int **array) {
    for (int i = 0; i < size; ++i) {
        if (i % 2 == 0) {
            for (int j = 0; j < size; ++j) {
                handleCell1(array, i, j);
            }
        } else {
            for (int j = size-1; j >= 0; --j) {
                handleCell1(array, i, j);
            }
        }
    }
}

// обход садовника 2
void secondThread(int **array) {
    for (int j = size-1; j >= 0; --j) {
        if (j % 2 != 0) {
            for (int i = size - 1; i >= 0; --i) {
                handleCell2(array, i, j);
            }
        } else {
            for (int i = 0; i < size; ++i) {
                handleCell2(array, i, j);
            }
        }
    }
}

// создание случайной матриы сада
int **createArray(int N) {
    srand(time(nullptr));
    int **array = new int*[N];
    for (int i = 0; i < 10; ++i) {
        array[i] = new int[N];
    }

    double amount = 0;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if ((amount / (N*N) < 0.1 || amount / (N*N) > 0.3) && rand() % 7 == 0) {
                array[i][j] = -1;
                amount++;
            } else {
                array[i][j] = 0;
            }
        }
    }

    array[0][0] = 0;
    array[N-1][N-1] = 0;

    // printConsole array
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << array[i][j] << "\t";
        }
        std::cout << std::endl;
    }

    return array;
}

// задание констант
void setUserConstantsConsole() {
    std::cout << "Enter size of array: ";
    std::cin >> size;
    std::cout << "Enter work time: ";
    std::cin >> work_time;
    std::cout << "Enter skip time: ";
    std::cin >> skip_time;
    std::cout << "Enter file name: ";
    std::cin >> file_name;
}

void setUserConstantsCmd(int argc, char *argv[]) {
    size = atoi(argv[2]);
    work_time = atoi(argv[3]);
    skip_time = atoi(argv[4]);
    file_name = argv[5];
}

void setUserConstantsFile(std::string filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        file >> size;
        file >> work_time;
        file >> skip_time;
        file >> file_name;
    }
    file.close();
}

int main(int argc, char *argv[]) {
    switch (argv[1][0]) {
        case 'c':
            setUserConstantsConsole();
            break;
        case 'f':
            setUserConstantsFile(argv[2]);
            break;
        case 't':
            setUserConstantsCmd(argc, argv);
            break;
        default:
            std::cout << "Wrong argument" << std::endl;
            return 0;
    }

    int **array = createArray(size);

    pthread_t thread1, thread2;

    pthread_mutex_init(&mutex1, nullptr);
    pthread_mutex_init(&mutex2, nullptr);
    pthread_mutex_init(&mutex_console, nullptr);

    pthread_create(&thread1, nullptr, (void *(*)(void *)) firstThread, array);
    pthread_create(&thread2, nullptr, (void *(*)(void *)) secondThread, array);

    pthread_join(thread1, nullptr);
    pthread_join(thread2, nullptr);

    pthread_mutex_destroy(&mutex2);
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex_console);


    return 0;
}
