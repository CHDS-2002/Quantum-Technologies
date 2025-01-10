#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define N 8 // Размер массива данных (должно быть степенью двойки)

// Структура для представления комплексного числа
typedef struct {
    double real;
    double imag;
} Complex;

// Структура для представления вектора состояний
typedef struct {
    Complex states[N];
} StateVector;

// Функция для инициализации комплексного числа
Complex init_complex(double real, double imag) {
    Complex c;
    c.real = real;
    c.imag = imag;
    return c;
}

// Функция для умножения двух комплексных чисел
Complex multiply_complex(Complex a, Complex b) {
    Complex c;
    c.real = a.real * b.real - a.imag * b.imag;
    c.imag = a.real * b.imag + a.imag * b.real;
    return c;
}

// Функция для нормализации вектора состояний
void normalize(StateVector* sv) {
    double norm = 0.0;
    for (int i = 0; i < N; ++i) {
        norm += sv->states[i].real * sv->states[i].real +
                sv->states[i].imag * sv->states[i].imag;
    }
    norm = sqrt(norm);
    for (int i = 0; i < N; ++i) {
        sv->states[i].real /= norm;
        sv->states[i].imag /= norm;
    }
}

// Функция для создания равномерной суперпозиции всех состояний
void create_superposition(StateVector* sv) {
    double amp = 1.0 / sqrt(N);
    for (int i = 0; i < N; ++i) {
        sv->states[i] = init_complex(amp, 0.0);
    }
}

// Функция для инвертирования фазы целевого состояния
void invert_phase(StateVector* sv, int target_index) {
    sv->states[target_index].real *= -1.0;
}

// Функция для применения оператора диффузии
void apply_diffusion(StateVector* sv) {
    Complex sum = init_complex(0.0, 0.0);
    for (int i = 0; i < N; ++i) {
        sum.real += sv->states[i].real;
        sum.imag += sv->states[i].imag;
    }
    for (int i = 0; i < N; ++i) {
        sv->states[i].real = 2.0 * sum.real / N - sv->states[i].real;
        sv->states[i].imag = 2.0 * sum.imag / N - sv->states[i].imag;
    }
}

// Функция для выполнения шага алгоритма Гровера
void grover_iteration(StateVector* sv, int target_index) {
    invert_phase(sv, target_index);
    apply_diffusion(sv);
}

// Функция для генерации случайного целевого индекса
int generate_target_index() {
    srand(time(NULL));
    return rand() % N;
}

// Функция для измерения состояния и получения результата
int measure_state(StateVector* sv) {
    double max_probability = 0.0;
    int max_index = 0;
    for (int i = 0; i < N; ++i) {
        double probability = sv->states[i].real * sv->states[i].real +
                             sv->states[i].imag * sv->states[i].imag;
        if (probability > max_probability) {
            max_probability = probability;
            max_index = i;
        }
    }
    return max_index;
}

int main() {
    StateVector sv;
    int target_index = generate_target_index();

    // Инициализация суперпозиции
    create_superposition(&sv);

    // Выполнение шагов алгоритма Гровера
    int num_iterations = (int)(M_PI / 4.0 * sqrt(N)) - 1;
    for (int i = 0; i < num_iterations; ++i) {
        grover_iteration(&sv, target_index);
    }

    // Измерение конечного состояния
    int found_index = measure_state(&sv);

    printf("Target index: %d\n", target_index);
    printf("Found index: %d\n", found_index);

    return 0;
}