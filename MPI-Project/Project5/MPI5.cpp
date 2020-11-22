#define MSMPI_NO_DEPRECATE_20
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"

#define BASE 10 // ������� ���������
#define MIN_LENGTH_FOR_KARATSUBA 4 // ����� ������ ���������� ������������ ����������

using namespace std;

int* sum(int* a, int size_a, int* b, int size_b) {
    // ������� ��� ������������ ���� ������� �����
    // ����� ������� ��������� � �������� ������� ���������
    // ���������� ����������������� �����
    int* s = new int[size_a + 1];
    s[size_a - 1] = a[size_a - 1];
    s[size_a] = 0;
    for (int i = 0; i < size_b; ++i)
        s[i] = a[i] + b[i];
    return s;
}

int* sub(int* a, int* b, int size_b) {
    // ������� ��� ��������� ������ �������� ����� �� �������
    // �������� ���������� ������� �����
    // ���������� ����������������� �����
    for (int i = 0; i < size_b; ++i)
        a[i] -= b[i];
    return a;
}

void normalize(int* l, int size_l) {
    // ������������ �����,
    // �.�. ���������� ������� ������� � ������������ � �������� ���������
    for (int i = 0; i < size_l - 1; ++i) {
        if (l[i] >= BASE) { // ���� ����� ������ �������������, �� ���������������� �������
            int carryover = l[i] / BASE;
            l[i + 1] += carryover;
            l[i] -= carryover * BASE;
        }
        else if (l[i] < 0) { // ���� ������ - ����
            int carryover = (l[i] + 1) / BASE - 1;
            l[i + 1] += carryover;
            l[i] -= carryover * BASE;
        }
    }
}

int* karatsuba(int* a, int size_a, int* b, int size_b) {
    // �������������� ������������
    int* product = new int[size_a + size_b];

    // ���� ����� ������ �� ��������� ������� ���������
    if (size_a < MIN_LENGTH_FOR_KARATSUBA) {
        memset(product, 0, sizeof(int) * (size_a + size_b));
        for (int i = 0; i < size_a; ++i)
            for (int j = 0; j < size_b; ++j) {
                product[i + j] += a[i] * b[j];
            }
    }
    else { // ��������� ������� ��������
        // ������� ����� ����� a
        int* a_part1 = a;
        int a_part1_size = (size_a + 1) / 2;

        // ������� ����� ����� a
        int* a_part2 = a + a_part1_size;
        int a_part2_size = (size_a) / 2;

        // ������� ����� ����� b
        int* b_part1 = b;
        int b_part1_size = (size_b + 1) / 2;

        // ������� ����� ����� b
        int* b_part2 = b + b_part1_size;
        int b_part2_size = (size_b) / 2;

        int* sum_of_a_parts = sum(a_part1, a_part1_size, a_part2, a_part2_size); // c���� ������ ����� a
        normalize(sum_of_a_parts, a_part1_size + 1);
        int* sum_of_b_parts = sum(b_part1, b_part1_size, b_part2, b_part2_size); // c���� ������ ����� b
        normalize(sum_of_b_parts, b_part1_size + 1);
        // ������������ ���� ������
        int* product_of_sums_of_parts = karatsuba(sum_of_a_parts, a_part1_size + 1, sum_of_b_parts, b_part1_size + 1);

        // ���������� ����� ������� ������
        int* product_of_first_parts = karatsuba(a_part1, a_part1_size, b_part1, b_part1_size); // ������� ����
        int* product_of_second_parts = karatsuba(a_part2, a_part2_size, b_part2, b_part2_size); // ������� ����
        int* sum_of_middle_terms = sub(sub(product_of_sums_of_parts, product_of_first_parts, a_part1_size + b_part1_size), product_of_second_parts, a_part2_size + b_part2_size);

        // ������������ ����������
        memcpy(product, product_of_first_parts, (a_part1_size + b_part1_size) * sizeof(int));
        memcpy(product + (a_part1_size + b_part1_size), product_of_second_parts, (a_part2_size + b_part2_size) * sizeof(int));
        for (int i = 0; i < (a_part1_size + 1 + b_part1_size + 1); ++i)
            product[a_part1_size + i] += sum_of_middle_terms[i];

        // ��������
        delete[] sum_of_a_parts;
        delete[] sum_of_b_parts;
        delete[] product_of_sums_of_parts;
        delete[] product_of_first_parts;
        delete[] product_of_second_parts;
    }

    normalize(product, size_a + size_b); // �������� ������������ �����

    return product;
}

int main(int argc, char* argv[])
{
    //��������� ������� �����
    const int N = 4, A = 8; // ����� � ���-�� �����
    const int pair = A / 2 + 1;
    bool even = 1;
    if (A % 2 == 1)
        even = 0;

    int array[N]; // �����, ������� ����� ���������
    int* result = new int[N + N]; // ��������� ��������� �� �������
    int temp_res1[N]; // 1 �������� ��� ����� �� 0� ��������
    int temp_res2[N]; // 2 �������� ��� ����� �� 0� ��������
    int res_array1[N * pair]; // ��� 1 ��������
    int res_array2[N * pair]; // ��� 2 ��������

    int* temp1 = new int[N + N];
    int* temp2 = new int[N + N];
    int* temp3;

    int ProcNum, ProcRank;
    MPI_Status Status;
    //������������� �����
    MPI_Init(&argc, &argv);
    //����������� ���-�� ���������
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    //����������� ����� ��������
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    if (ProcNum < pair)
    {
        MPI_Finalize();
        delete[] result;
        delete[] temp1;
        delete[] temp2;
        return 0;
    }

    // ����� ��� type
    MPI_Datatype type;
    MPI_Type_contiguous(N, MPI_INT, &type);
    MPI_Type_commit(&type);

    // �������� ������, ��������� � ������������ ��������������
    MPI_Group group;
    MPI_Comm_group(MPI_COMM_WORLD, &group);
    // ������� ����� ������ �� ��������� � ������� ranks
    int* ranks = new int[pair];
    for (int i = 0; i < pair; i++)
        ranks[i] = i;
    MPI_Group my_group;
    MPI_Group_incl(group, pair, ranks, &my_group);
    // ������� ����� ������������
    MPI_Comm COMM;
    MPI_Comm_create(MPI_COMM_WORLD, my_group, &COMM);

    // ���������� ����� � �������� ���� ���������
    if (ProcRank == 0)
    {
        for (int i = 0; i < N; i++)
            array[i] = 5; // + rand() % 5;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(array, 1, type, 0, MPI_COMM_WORLD);

    // ������� �� ���������
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

    // ���� �������������� ����� �� 0� ��������
    if (ProcRank >= 0 && ProcRank < pair)
    {
        MPI_Gather(temp_res1, 1, type, res_array1, 1, type, 0, COMM);
        MPI_Gather(temp_res2, 1, type, res_array2, 1, type, 0, COMM);
    }

    // ���������� ������� �� 0� ��������
    if (ProcRank == 0)
    {
        int size1 = N + N;
        if (even) // ���� � - ������
        {
            // ���������� �����, ��������� � 0�� ��������
            for (int i = 0; i < N; i++)
                temp1[i] = res_array1[i + N];
            for (int i = N; i < N + N; i++)
                temp1[i] = res_array2[i];
            /*for (int i = 0; i < N + N; i++)
                printf("[%d]", temp1[i]);*/
        }
        else // ��������� ��������
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

        // ����������� �������������� �����
        for (int i = even + 1; i < pair; i++)
        {
            printf(" \n A:");
            for (int i = size1 - 1; i >= 0; i--)
                printf("[%d]", temp1[i]);

            // ��������� ������
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

            // ��������� ����
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

        // ������������� ������������ ����
        MPI_Type_free(&type);
        // �������� �����
        MPI_Group_free(&group);
        MPI_Group_free(&my_group);
        // �������� �������������
        MPI_Comm_free(&COMM);
    }

    // ����������
    delete[] ranks;
    delete[] result;
    delete[] temp1;
    delete[] temp2;
    MPI_Finalize();
    return 0;
}