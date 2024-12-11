#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> // Para a função sleep

#define MAX_TASKS 100

// Estrutura para uma tarefa
typedef struct {
    int input; // O input da tarefa
} Task;

// Variáveis globais para a fila de tarefas
Task task_queue[MAX_TASKS]; // -> fds
int task_count = 0;  // Número de tarefas na fila -> count
int task_index = 0;  // Índice da próxima tarefa a ser retirada -> index
int tasks_completed = 0; // Número de tarefas concluídas -> ???

// Variável de controle para encerrar as threads
int finished = 0; // -> ???

// Mutex para sincronizar o acesso à fila de tarefas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Função que realiza a tarefa
void process_task(Task* task) {
    printf("Thread %lu processando tarefa com input %d\n", pthread_self(), task->input);
    sleep(1); // Simula uma tarefa demorada
}

// Função executada pelas threads no pool
void* thread_function(void* arg) {
    while (1) {
        Task task;
        int has_task = 0;

        // Bloqueia o acesso à fila de tarefas
        pthread_mutex_lock(&mutex);

        // Verifica se há tarefas disponíveis
        if (task_index < task_count) {
            task = task_queue[task_index];
            task_index++;
            has_task = 1; // Marca que uma tarefa foi retirada
        } else if (tasks_completed >= task_count) {
            // Se todas as tarefas foram concluídas, sinaliza para sair
            finished = 1;
        }

        // Desbloqueia o acesso à fila de tarefas
        pthread_mutex_unlock(&mutex);

        // Se havia uma tarefa, processa-a
        if (has_task) {
            process_task(&task);

            // Incrementa o contador de tarefas concluídas
            pthread_mutex_lock(&mutex);
            tasks_completed++;
            pthread_mutex_unlock(&mutex);
        }

        // Sai do loop se todas as tarefas foram concluídas
        if (finished) {
            break;
        }
    }
    return NULL;
}

int main() {
    int M, N;

    // Entrada do número de inputs e threads
    printf("Digite o número de inputs (M): ");
    scanf("%d", &M);
    printf("Digite o número de threads (N): ");
    scanf("%d", &N);

    if (M > MAX_TASKS) {
        printf("Número máximo de tarefas é %d.\n", MAX_TASKS);
        return 1;
    }

    // Adiciona as tarefas à fila
    for (int i = 0; i < M; i++) {
        task_queue[i].input = i + 1;
    }
    task_count = M;

    // Cria o pool de threads
    pthread_t threads[N];
    for (int i = 0; i < N; i++) {
        if (pthread_create(&threads[i], NULL, thread_function, NULL) != 0) {
            perror("Erro ao criar thread");
            return 1;
        }
    }

    // Aguarda todas as threads concluírem
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Todas as tarefas foram processadas.\n");
    return 0;
}
