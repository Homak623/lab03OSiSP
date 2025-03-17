#define _GNU_SOURCE
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define CAPACITY 8

typedef struct {
    pid_t pid;
    char name[CAPACITY * 2];
    bool is_running;  // Флаг для отслеживания состояния процесса
} ProcessInfo;

size_t child_processes_size = 0;
size_t child_processes_capacity = CAPACITY;
ProcessInfo *child_processes = NULL;
const char *child_name = "./child";

void init_signals();
void handle_signal(int signo, siginfo_t *info, void *context);
void handle_child_exit(int signo);
void create_child();
void delete_last_child();
void list_children();
void delete_all_children();
void cleanup_and_exit();
void wait_for_children();
void print_menu();
void start_child(int index);
void stop_child(int index);

int main() {
    srand(time(NULL));
    init_signals();
    child_processes = (ProcessInfo *)calloc(child_processes_capacity, sizeof(ProcessInfo));
    if (!child_processes) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    print_menu();
    char input[CAPACITY];
    while (true) {
        printf("> ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL) continue;

        char option = input[0];
        int index = -1;
        if (strlen(input) > 1 && (input[1] >= '0' && input[1] <= '9')) {
            index = input[1] - '0';  // Получаем индекс процесса
        }

        switch (option) {
            case 'm': print_menu(); break;
            case '+': create_child(); break;
            case '-': delete_last_child(); break;
            case 'l': list_children(); break;
            case 'k': delete_all_children(); break;
            case 's': stop_child(index); break;
            case 'g': start_child(index); break;
            case 'q': cleanup_and_exit(); break;
            default: printf("Invalid option. Type 'm' for menu.\n"); break;
        }
    }
    return 0;
}

void start_child(int index) {
    if (index < 0 || index >= child_processes_size) {
        printf("Invalid index\n");
        return;
    }
    pid_t pid = child_processes[index].pid;
    union sigval info;
    info.sival_int = 0;
    sigqueue(pid, SIGUSR2, info);  // Разрешаем вывод
    child_processes[index].is_running = true;
    printf("Started child %s with PID %d\n", child_processes[index].name, pid);
}

void stop_child(int index) {
    if (index < 0 || index >= child_processes_size) {
        printf("Invalid index\n");
        return;
    }
    pid_t pid = child_processes[index].pid;
    union sigval info;
    info.sival_int = 0;
    sigqueue(pid, SIGUSR1, info);  // Останавливаем вывод
    child_processes[index].is_running = false;
    printf("Stopped child %s with PID %d\n", child_processes[index].name, pid);
}

void wait_for_children() {
    while (child_processes_size > 0) {
        pid_t pid = child_processes[child_processes_size - 1].pid;
        int status;
        waitpid(pid, &status, 0);  // Ожидание завершения дочернего процесса
        printf("Child %s with PID %d has exited\n", child_processes[child_processes_size - 1].name, pid);
        child_processes_size--;
    }
}

void init_signals() {
    struct sigaction action = {0};
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGCHLD);

    action.sa_flags = SA_SIGINFO;
    action.sa_mask = set;
    action.sa_sigaction = handle_signal;

    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);

    // Обработка SIGCHLD для завершения дочерних процессов
    action.sa_handler = handle_child_exit;
    sigaction(SIGCHLD, &action, NULL);
}

void handle_signal(int signo, siginfo_t *info, void *context) {
    if (signo == SIGUSR1) {
        // Получен сигнал от дочернего процесса с запросом на вывод
        pid_t child_pid = info->si_value.sival_int;
        printf("Parent: Received SIGUSR1 from child %d\n", child_pid);

        // Разрешаем вывод данных
        union sigval info_resume;
        info_resume.sival_int = 0;
        sigqueue(child_pid, SIGUSR2, info_resume);  // Отправляем SIGUSR2 дочернему процессу
    } else if (signo == SIGUSR2) {
        // Дочерний процесс завершил вывод
        pid_t child_pid = info->si_value.sival_int;
        printf("Parent: Child %d has finished output\n", child_pid);
    }
}

void handle_child_exit(int signo) {
    // Обработка завершения дочерних процессов
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (size_t i = 0; i < child_processes_size; i++) {
            if (child_processes[i].pid == pid) {
                printf("Child %s with PID %d has exited\n", child_processes[i].name, pid);
                // Удаляем процесс из массива
                for (size_t j = i; j < child_processes_size - 1; j++) {
                    child_processes[j] = child_processes[j + 1];
                }
                child_processes_size--;
                break;
            }
        }
    }
}

void create_child() {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        return;
    }
    if (pid == 0) {
        execl(child_name, child_name, NULL);
        perror("Failed to exec");
        exit(EXIT_FAILURE);
    } else {
        if (child_processes_size >= child_processes_capacity) {
            child_processes_capacity *= 2;
            ProcessInfo *tmp = (ProcessInfo *)realloc(child_processes, child_processes_capacity * sizeof(ProcessInfo));
            if (!tmp) {
                perror("Failed to reallocate memory");
                exit(EXIT_FAILURE);
            }
            child_processes = tmp;
        }
        snprintf(child_processes[child_processes_size].name, CAPACITY * 2, "C_%02d", (int)child_processes_size);
        child_processes[child_processes_size].pid = pid;
        child_processes[child_processes_size].is_running = false;  // По умолчанию процесс остановлен
        child_processes_size++;
        printf("Created child %s with PID %d\n", child_processes[child_processes_size - 1].name, pid);
    }
}

void delete_last_child() {
    if (child_processes_size == 0) {
        printf("No children to delete\n");
        return;
    }
    pid_t pid = child_processes[child_processes_size - 1].pid;
    kill(pid, SIGTERM);
    printf("Deleted child %s with PID %d\n", child_processes[child_processes_size - 1].name, pid);
    child_processes_size--;
}

void list_children() {
    printf("Parent PID: %d\n", getpid());
    if (child_processes_size == 0) {
        printf("No children running.\n");
    } else {
        for (size_t i = 0; i < child_processes_size; i++) {
            printf("Child %s with PID %d is %s\n", child_processes[i].name, child_processes[i].pid,
                   child_processes[i].is_running ? "running" : "stopped");
        }
    }
}

void delete_all_children() {
    while (child_processes_size > 0) {
        delete_last_child();
    }
    printf("All children deleted\n");
}

void cleanup_and_exit() {
    delete_all_children();
    wait_for_children();
    free(child_processes);
    printf("Exiting...\n");
    exit(EXIT_SUCCESS);
}

void print_menu() {
    printf("\nOptions:\n");
    printf("+ - Create new child\n");
    printf("- - Delete last child\n");
    printf("l - List all children\n");
    printf("k - Delete all children\n");
    printf("s<num> - Stop child at index <num>\n");
    printf("g<num> - Start child at index <num>\n");
    printf("q - Quit\n");
    printf("m - Show this menu\n");
}
