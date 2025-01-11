#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>  // Добавили включение stdint.h для типа intptr_t

// Операция Адамара
void hadamard(int *qubit) {
    // В данном случае просто меняем значение qubit'а на противоположное
    *qubit ^= 1;
}

// Оракул U_f
void oracle(int *input_qubit, int *output_qubit, bool (*f)(int)) {
    if (f(*input_qubit)) {
        *output_qubit ^= 1;
    }
}

// Измерение состояния кубита
bool measure(int qubit) {
    return qubit == 1;
}

// Функции для тестирования
bool constant_function(int x) { return true; }
bool balanced_function(int x) { return x == 0 ? false : true; }

typedef struct {
    pthread_t thread_id;
    int input_qubit;
    int output_qubit;
    bool (*f)(int);
} ThreadArgs;

void* execute_algorithm(void* args) {
    ThreadArgs* ta = (ThreadArgs*)args;
    
    // Шаг 1: Начальные состояния |0>|1>
    int input_qubit = ta->input_qubit;
    int output_qubit = ta->output_qubit;
    
    // Шаг 2: Применение операции Адамара
    hadamard(&input_qubit);
    hadamard(&output_qubit);
    
    // Шаг 3: Вызов оракула
    oracle(&input_qubit, &output_qubit, ta->f);
    
    // Шаг 4: Еще одна операция Адамара
    hadamard(&input_qubit);
    
    // Шаг 5: Измерение первого кубита
    bool result = measure(input_qubit);
    
    free(args);  // Освобождаем память после завершения потока
    
    return (void*)(intptr_t)(result ? 1 : 0);
}

int main() {
    // Тестирование с постоянной функцией
    printf("Testing with a constant function:\n");
    ThreadArgs* const_args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
    const_args->input_qubit = 0;
    const_args->output_qubit = 1;
    const_args->f = constant_function;
    
    pthread_t const_thread_id;
    pthread_create(&const_thread_id, NULL, execute_algorithm, const_args);
    void* const_result;
    pthread_join(const_thread_id, &const_result);
    printf("Constant Function Result: %s\n", (intptr_t)const_result == 1 ? "CONSTANT" : "BALANCED");
    
    // Тестирование со сбалансированной функцией
    printf("\nTesting with a balanced function:\n");
    ThreadArgs* bal_args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
    bal_args->input_qubit = 0;
    bal_args->output_qubit = 1;
    bal_args->f = balanced_function;
    
    pthread_t bal_thread_id;
    pthread_create(&bal_thread_id, NULL, execute_algorithm, bal_args);
    void* bal_result;
    pthread_join(bal_thread_id, &bal_result);
    printf("Balanced Function Result: %s\n", (intptr_t)bal_result == 1 ? "CONSTANT" : "BALANCED");
    
    return 0;
}