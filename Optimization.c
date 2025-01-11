#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <complex.h>

#define NUM_QUBITS 4  // Количество кубитов
#define NUM_ITERATIONS 100  // Количество итераций градиентного спуска
#define LEARNING_RATE 0.01  // Скорость обучения

// Целевая функция (например, квадратичная функция)
double target_function(double x) {
    return x * x;
}

// Производная целевой функции
double derivative(double x) {
    return 2 * x;
}

// Структура для хранения комплексного числа
typedef struct {
    double real;
    double imag;
} ComplexNumber;

// Функция для сложения комплексных чисел
ComplexNumber add_complex_numbers(ComplexNumber a, ComplexNumber b) {
    ComplexNumber result;
    result.real = a.real + b.real;
    result.imag = a.imag + b.imag;
    return result;
}

// Функция для умножения комплексных чисел
ComplexNumber multiply_complex_numbers(ComplexNumber a, ComplexNumber b) {
    ComplexNumber result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

// Структура для хранения квантового состояния
typedef struct {
    ComplexNumber amplitudes[1 << NUM_QUBITS];  // Амплитуды состояний
    pthread_mutex_t mutex;  // Мьютекс для синхронизации
} QuantumState;

// Инициализация квантового состояния
void initialize_quantum_state(QuantumState* state) {
    memset(state->amplitudes, 0, sizeof(ComplexNumber) * (1 << NUM_QUBITS));
    state->amplitudes[0].real = 1.0;  // Начальное состояние |0000>
    pthread_mutex_init(&state->mutex, NULL);
}

// Применение оператора Адамара
void apply_hadamard(QuantumState* state, int qubit_index) {
    pthread_mutex_lock(&state->mutex);
    for (int i = 0; i < (1 << NUM_QUBITS); ++i) {
        int bit = (i >> qubit_index) & 1;
        ComplexNumber new_amplitude = {
            .real = state->amplitudes[i].real / sqrt(2),
            .imag = state->amplitudes[i].imag / sqrt(2)
        };
        if (bit == 0) {
            state->amplitudes[i] = add_complex_numbers(new_amplitude, new_amplitude);
        } else {
            ComplexNumber one = {.real = 1, .imag = 0};
            ComplexNumber minus_one = {.real = -1, .imag = 0};
            state->amplitudes[i] = add_complex_numbers(
                multiply_complex_numbers(new_amplitude, one),
                multiply_complex_numbers(new_amplitude, minus_one)
            );
        }
    }
    pthread_mutex_unlock(&state->mutex);
}

// Применение контролируемого Z-вращения
void controlled_z_rotation(QuantumState* state, int control_qubit, int target_qubit, double angle) {
    pthread_mutex_lock(&state->mutex);
    for (int i = 0; i < (1 << NUM_QUBITS); ++i) {
        int control_bit = (i >> control_qubit) & 1;
        int target_bit = (i >> target_qubit) & 1;
        if (control_bit && target_bit) {
            ComplexNumber rotation_factor = {.real = cos(angle), .imag = sin(angle)};
            state->amplitudes[i] = multiply_complex_numbers(state->amplitudes[i], rotation_factor);
        }
    }
    pthread_mutex_unlock(&state->mutex);
}

// Оценка фазы
double estimate_phase(QuantumState* state, int qubit_index) {
    double phase_estimate = 0.0;
    for (int i = 0; i < (1 << NUM_QUBITS); ++i) {
        int bit = (i >> qubit_index) & 1;
        if (bit) {
            phase_estimate += atan2(state->amplitudes[i].imag, state->amplitudes[i].real);
        }
    }
    return phase_estimate / (1 << (NUM_QUBITS - 1));  // Нормализуем оценку фазы
}

// Структура для передачи аргументов потокам
typedef struct {
    QuantumState* state;
    double current_x;
    double learning_rate;
} StepArguments;

// Поточная функция для выполнения одного шага градиентного спуска
void* gradient_descent_step(void* arg) {
    StepArguments* args = (StepArguments*)arg;
    QuantumState* state = args->state;
    double current_x = args->current_x;
    double learning_rate = args->learning_rate;

    // Применяем операторы для создания суперпозиции
    for (int i = 0; i < NUM_QUBITS; ++i) {
        apply_hadamard(state, i);
    }

    // Применяем контролируемые Z-вращения для оценки фазы
    for (int i = 0; i < NUM_QUBITS; ++i) {
        controlled_z_rotation(state, i, i, derivative(current_x) * learning_rate);
    }

    // Оцениваем фазу
    double phase = estimate_phase(state, 0);

    // Обновляем параметры
    current_x -= phase * learning_rate;

    // Освобождаем память
    free(arg);

    return NULL;
}

// Основная функция для выполнения квантового градиентного спуска
void quantum_gradient_descent(double initial_x) {
    QuantumState state;
    initialize_quantum_state(&state);

    double current_x = initial_x;
    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        StepArguments* args = malloc(sizeof(StepArguments));
        args->state = &state;
        args->current_x = current_x;
        args->learning_rate = LEARNING_RATE;

        pthread_t thread;
        pthread_create(&thread, NULL, gradient_descent_step, args);
        pthread_join(thread, NULL);

        printf("Iteration %d: x = %.4lf\n", iter, current_x);
    }

    pthread_mutex_destroy(&state.mutex);
}

int main() {
    double initial_x = 10.0;  // Начальная точка
    quantum_gradient_descent(initial_x);
    return 0;
}