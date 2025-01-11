#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <complex.h>
#include <stdbool.h>

#define NUM_THREADS 4
#define ERROR_SIGSEGV 1397
#define PI 3.14159265358979323846

void signal_handler(int signo) {
    if (signo == SIGSEGV) {
        printf("Получено уведомление о завершении программы по причине ошибки SIGSEGV.\n");
        exit(EXIT_FAILURE);
    }
}

// Тип для комплексного числа
typedef double complex cplx;

// Размерность системы (число кубитов)
const int N = 16;

// Функция для вычисления модуля комплексного числа
double modulus(cplx z) {
    return cabs(z);
}

// Функция для инициализации начального состояния |0...0>
void initialize_state(cplx state[], int n) {
    for (int i = 0; i < (1 << n); ++i) {
        state[i] = 0.0;
    }
    state[0] = 1.0;
}

// Функция для применения оператора контролируемой фазы
void controlled_phase_shift(cplx state[], int control, int target, int n, double theta) {
    cplx temp[(1 << n)];
    for (int i = 0; i < (1 << n); ++i) {
        temp[i] = state[i];
    }
    for (int i = 0; i < (1 << n); ++i) {
        if ((i >> control) & 1) { // Если контрольный бит установлен
            int new_index = i ^ (1 << target);
            state[new_index] *= cexp(I * theta);
        }
    }
}

// Функция для применения оператора фазовой оценки
void quantum_phase_estimation(cplx state[], int n, int a, int N) {
    static double precomputed_phases[65536];
    static bool phases_computed = false;

    if (!phases_computed) {
        for (int k = 0; k < N; ++k) {
            for (int j = 0; j < k; ++j) {
                precomputed_phases[k * (1 << N) + j] = 2 * PI * pow(a, j) / pow(N, k + 1);
            }
        }
        phases_computed = true;
    }

    for (int k = 0; k < n; ++k) {
        for (int j = 0; j < k; ++j) {
            controlled_phase_shift(state, n - 1 - j, n - 1 - k, n, precomputed_phases[k * (1 << N) + j]);
        }
    }
}

// Функция для применения обратного преобразования Фурье
void inverse_qft(cplx state[], int n) {
    static cplx precomputed_factors[65536];
    static bool factors_computed = false;

    if (!factors_computed) {
        for (int j = 0; j < (1 << N); ++j) {
            precomputed_factors[j] = cexp(-I * 2 * PI * j / (1 << N)) / (1 << N);
        }
        factors_computed = true;
    }

    for (int i = 0; i < (1 << n); ++i) {
        cplx sum = 0.0;
        for (int j = 0; j < (1 << n); ++j) {
            sum += state[j] * precomputed_factors[i * j];
        }
        state[i] = sum;
    }
}

// Функция для измерения состояния
void measure_state(cplx state[], int n) {
    double probabilities[(1 << n)];
    for (int i = 0; i < (1 << n); ++i) {
        probabilities[i] = creal(state[i] * conj(state[i]));
    }

    double sum = 0.0;
    for (int i = 0; i < (1 << n); ++i) {
        sum += probabilities[i];
    }

    for (int i = 0; i < (1 << n); ++i) {
        probabilities[i] /= sum;
    }

    int result = 0;
    double rand_value = (double)rand() / RAND_MAX;
    double cumulative_probability = 0.0;
    for (int i = 0; i < (1 << n); ++i) {
        cumulative_probability += probabilities[i];
        if (rand_value < cumulative_probability) {
            result = i;
            break;
        }
    }

    printf("Результат измерения: %d\n", result);
}

// Структура для передачи параметров в поток
typedef struct {
    int start;
    int end;
    int a;
    int N;
    cplx *state;
} ThreadArgs;

// Функция потока для выполнения части вычислений
void *compute_thread(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *) args;
    int start = thread_args->start;
    int end = thread_args->end;
    int a = thread_args->a;
    int N = thread_args->N;
    cplx *state = thread_args->state;

    for (int i = start; i < end; ++i) {
        quantum_phase_estimation(state, N, a, N);
        inverse_qft(state, N);
        measure_state(state, N);
    }

    return NULL;
}

// Основная функция
int main() {
    signal(SIGSEGV, signal_handler);
    srand(time(NULL));

    cplx state[(1 << N)];
    initialize_state(state, N);

    int a = 7; // Пример значения для a
    int M = 15; // Пример значения для M

    // Разделение диапазона на потоки
    int chunk_size = M / NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_args[i].start = i * chunk_size;
        thread_args[i].end = (i + 1) * chunk_size;
        thread_args[i].a = a;
        thread_args[i].N = N;
        thread_args[i].state = state;

        int ret = pthread_create(&threads[i], NULL, compute_thread, &thread_args[i]);
        if (ret != 0) {
            perror("Ошибка создания потока");
            exit(EXIT_FAILURE);
        }
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}