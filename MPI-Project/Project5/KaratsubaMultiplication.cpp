#define MSMPI_NO_DEPRECATE_20
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#define BASE 10 // Система счисления
#define MIN_LENGTH_FOR_KARATSUBA 4 // Числа короче умножаются квадратичным алгоритмом

using namespace std;

int* sum(int* a, int size_a, int* b, int size_b) {
    // Функция для суммирования двух длинных чисел
    // Более длинное передется в качестве первого аргумента
    // Возвращает ненормализованное число
    int* s = new int[size_a + 1];
    s[size_a - 1] = a[size_a - 1];
    s[size_a] = 0;
    for (int i = 0; i < size_b; ++i)
        s[i] = a[i] + b[i];
    return s;
}

int* sub(int* a, int* b, int size_b) {
    // Функция для вычитания одного длинного числа из другого
    // Изменяет содержимое первого числа
    // Возвращает ненормализованное число
    for (int i = 0; i < size_b; ++i)
        a[i] -= b[i];
    return a;
}

void normalize(int* l, int size_l) {
    // Нормализация числа,
    // т.е. приведение каждого разряда в соответствие с системой счисления
    for (int i = 0; i < size_l - 1; ++i) {
        if (l[i] >= BASE) { // если число больше максимального, то организовавается перенос
            int carryover = l[i] / BASE;
            l[i + 1] += carryover;
            l[i] -= carryover * BASE;
        }
        else if (l[i] < 0) { // если меньше - заем
            int carryover = (l[i] + 1) / BASE - 1;
            l[i + 1] += carryover;
            l[i] -= carryover * BASE;
        }
    }
}

int* karatsuba(int* a, int size_a, int* b, int size_b) {
    // Результирующее произведение
    int* product = new int[size_a + size_b];

    // Если число короче то применяем наивное умножение
    if (size_a < MIN_LENGTH_FOR_KARATSUBA) {
        memset(product, 0, sizeof(int) * (size_a + size_b));
        for (int i = 0; i < size_a; ++i)
            for (int j = 0; j < size_b; ++j) {
                product[i + j] += a[i] * b[j];
            }
    }
    else { // Умножение методом Карацубы
        // Младшая часть числа a
        int* a_part1 = a;
        int a_part1_size = (size_a + 1) / 2;

        // Старшая часть числа a
        int* a_part2 = a + a_part1_size;
        int a_part2_size = (size_a) / 2;

        // Младшая часть числа b
        int* b_part1 = b;
        int b_part1_size = (size_b + 1) / 2;

        // Старшая часть числа b
        int* b_part2 = b + b_part1_size;
        int b_part2_size = (size_b) / 2;

        int* sum_of_a_parts = sum(a_part1, a_part1_size, a_part2, a_part2_size); // cумма частей числа a
        normalize(sum_of_a_parts, a_part1_size + 1);
        int* sum_of_b_parts = sum(b_part1, b_part1_size, b_part2, b_part2_size); // cумма частей числа b
        normalize(sum_of_b_parts, b_part1_size + 1);
        // Произведение сумм частей
        int* product_of_sums_of_parts = karatsuba(sum_of_a_parts, a_part1_size + 1, sum_of_b_parts, b_part1_size + 1);

        // Нахождение суммы средних членов
        int* product_of_first_parts = karatsuba(a_part1, a_part1_size, b_part1, b_part1_size); // младший член
        int* product_of_second_parts = karatsuba(a_part2, a_part2_size, b_part2, b_part2_size); // старший член
        int* sum_of_middle_terms = sub(sub(product_of_sums_of_parts, product_of_first_parts, a_part1_size + b_part1_size), product_of_second_parts, a_part2_size + b_part2_size);

        // Суммирование многочлена
        memcpy(product, product_of_first_parts, (a_part1_size + b_part1_size) * sizeof(int));
        memcpy(product + (a_part1_size + b_part1_size), product_of_second_parts, (a_part2_size + b_part2_size) * sizeof(int));
        for (int i = 0; i < (a_part1_size + 1 + b_part1_size + 1); ++i)
            product[a_part1_size + i] += sum_of_middle_terms[i];

        // Зачистка
        delete[] sum_of_a_parts;
        delete[] sum_of_b_parts;
        delete[] product_of_sums_of_parts;
        delete[] product_of_first_parts;
        delete[] product_of_second_parts;
    }

    normalize(product, size_a + size_b); // Конечная нормализация числа

    return product;
}

int main(int argc, char* argv[])
{
    // Умножение длинных чисел
    const int N = 4, A = 8; // длина и кол-во чисел
    const int pair = A / 2 + 1;
    bool even = 1;
    if (A % 2 == 1)
        even = 0;

    int array[N]; // число, которое будем рассылать
    int* result = new int[N + N]; // результат умножения на потоках
    int temp_res1[N]; // 1 половина для сбора на 0м процессе
    int temp_res2[N]; // 2 половина для сбора на 0м процессе
    int res_array1[N * pair]; // все 1 половины
    int res_array2[N * pair]; // все 2 половины

    int* temp1 = new int[N + N];
    int* temp2 = new int[N + N];
    int* temp3;

    int ProcNum, ProcRank;
    MPI_Status Status;
    // Инициализация среды
    MPI_Init(&argc, &argv);
    // Определение кол-ва процессов
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    // Определение ранга процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcNum < pair)
    {
        MPI_Finalize();
        delete[] result;
        delete[] temp1;
        delete[] temp2;
        return 0;
    }

    // Новый тип type
    MPI_Datatype type;
    MPI_Type_contiguous(N, MPI_INT, &type);
    MPI_Type_commit(&type);

    // Получить группу, связанную с существующим коммуникатором
    MPI_Group group;
    MPI_Comm_group(MPI_COMM_WORLD, &group);
    // Создать новую группу из процессов с рангами ranks
    int* ranks = new int[pair];
    for (int i = 0; i < pair; i++)
        ranks[i] = i;
    MPI_Group my_group;
    MPI_Group_incl(group, pair, ranks, &my_group);
    // Создать новый коммуникатор
    MPI_Comm COMM;
    MPI_Comm_create(MPI_COMM_WORLD, my_group, &COMM);

    // Заполнение числа и рассылка всем процессам
    if (ProcRank == 0)
    {
        for (int i = 0; i < N; i++)
            array[i] = 5; // + rand() % 5;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(array, 1, type, 0, MPI_COMM_WORLD);

    // Подсчёт на процессах
    if (ProcRank >= 1 && ProcRank < pair)
    {
        result = karatsuba(array, N, array, N);
        for (int i = 0; i < N; i++) {
            temp_res1[i] = result[i];
            printf("[%d]", result[i]);
        }
        for (int i = N; i < N + N; i++) {
            temp_res2[i - N] = result[i];
            printf("[%d]", result[i]);
        }
        printf("  Rank[%d] - multi done\n", ProcRank);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Сбор результирующих чисел на 0м процессе
    if (ProcRank >= 0 && ProcRank < pair)
    {
        MPI_Gather(temp_res1, 1, type, res_array1, 1, type, 0, COMM);
        MPI_Gather(temp_res2, 1, type, res_array2, 1, type, 0, COMM);
    }

    // Остаточный подсчёт на 0м процессе
    if (ProcRank == 0)
    {
        int size1 = N + N;
        if (even) // Если А - чётное
        {
            // Пропускаем число, пришедшее с 0го процесса
            for (int i = 0; i < N; i++)
                temp1[i] = res_array1[i + N];
            for (int i = N; i < N + N; i++)
                temp1[i] = res_array2[i];
            /*for (int i = 0; i < N + N; i++)
                printf("[%d]", temp1[i]);*/
        }
        else // Добавляем нечётное
        {
            for (int i = 0; i < N; i++)
                temp1[i] = array[i];
            for (int i = N; i < N + N; i++)
                temp1[i] = 0;
            /*for (int i = 0; i < N + N; i++)
                printf("[%d]", temp1[i]);*/
        }

        for (int i = 0; i < N; i++)
            temp2[i] = res_array1[i + N];
        for (int i = N; i < N + N; i++)
            temp2[i] = temp2[i] = res_array2[i];

        // Перемножаем результирующие числа
        for (int i = even + 1; i < pair; i++)
        {
            printf(" \n A:");
            for (int i = size1 - 1; i >= 0; i--)
                printf("[%d]", temp1[i]);

            // Дополняем нулями
            if (N + N < size1)
            {
                temp2 = new int[size1];
                for (int i = 0; i < N; i++)
                    temp2[i] = res_array1[i + N];
                for (int i = N; i < N + N; i++)
                    temp2[i] = res_array2[i];
                for (int i = N + N; i < size1; i++)
                    temp2[i] = 0;
            }

            printf(" \n B:");
            for (int i = size1 - 1; i >= 0; i--)
                printf("[%d]", temp2[i]);

            temp3 = new int[2 * size1];
            temp3 = karatsuba(temp1, size1, temp2, size1);
            temp1 = new int[2 * size1];
            for (int i = 0; i < 2 * size1; i++)
                temp1[i] = temp3[i];

            /*printf(" \n Z:");
            for (int i = 2 * size1 - 1; i >= 0; i--)
                printf("[%d]", temp1[i]);*/

            // Подчищаем нули
            while (temp3[2 * size1 - 1] == 0 && temp3[2 * size1 - 2] == 0)
            {
                size1--;
                temp1 = new int[2 * size1];
                for (int i = 0; i < 2 * size1; i++)
                    temp1[i] = temp3[i];
            }

            printf(" \n R:");
            for (int i = 2 * size1 - 1; i >= 0; i--)
                printf("[%d]", temp1[i]);
            printf("\n");

            size1 += size1;
        }

        // Аннулирование производного типа
        MPI_Type_free(&type);
        // Удаление групп
        MPI_Group_free(&group);
        MPI_Group_free(&my_group);
        // Удаление коммуникатора
        MPI_Comm_free(&COMM);
    }

    // Завершение
    delete[] ranks;
    delete[] result;
    delete[] temp1;
    delete[] temp2;
    MPI_Finalize();
    return 0;
}
