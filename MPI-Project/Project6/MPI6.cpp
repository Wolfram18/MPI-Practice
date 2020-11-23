#define MSMPI_NO_DEPRECATE_20
#include <complex>
#include <vector>
#include <cmath>
#include <stdio.h>;
#include "mpi.h";
using namespace std;

const double pi = acos(-1);

#define veccom vector<complex<double>>
#define vecint vector<int>

int reverse_bits(int a, int n)
{
    int res = 0;
    for (int i = 0; i < n; ++i)
    {
        res <<= 1;
        res |= a & 1;
        a >>= 1;
    }
    return res;
}

int logn(int a)
{
    int cnt = 0;
    do
    {
        ++cnt;
        a >>= 1;
    } while (a);
    return cnt;
}

int topow2(int a)
{
    for (int i = 0; i < 32; ++i)
        if ((1 << i) >= a)
            return 1 << i;
    return -1;
}

void swap_revbits(veccom& a)
{
    int l = logn(a.size()) - 1;
    veccom res(a.size());
    for (int i = 0; i < a.size(); ++i)
        res[i] = a[reverse_bits(i, l)];
    a = res;
}

void fft(veccom& a, bool back)
{
    swap_revbits(a);
    int n = a.size();
    double t = (back ? -1 : 1);

    for (int m = 2; m <= n; m *= 2)
    {
        complex <double> wm(cos(t * 2 * pi / (double)m), sin(t * 2 * pi / (double)m));

        for (int k = 0; k < n; k += m)
        {
            complex <double> w(1);
            for (int j = 0; j < m / 2; ++j)
            {
                complex <double> a0 = a[k + j];
                complex <double> w_a1 = w * a[k + j + m / 2];
                a[k + j] = a0 + w_a1;
                a[k + j + m / 2] = a0 - w_a1;

                if (back)
                {
                    a[k + j] /= 2.0;
                    a[k + j + m / 2] /= 2.0;
                }
                w *= wm;
            }
        }
    }
}

vecint fft_mul(vecint a, vecint b)
{
    int n = topow2(max(a.size(), b.size())) * 2;
    a.resize(n);
    b.resize(n);

    veccom ac(a.begin(), a.end());
    veccom bc(b.begin(), b.end());

    fft(ac, false);
    fft(bc, false);

    veccom cc(n);
    for (int i = 0; i < n; ++i)
        cc[i] = ac[i] * bc[i];
    fft(cc, true);

    vecint c(n, 0);
    for (int i = 0; i < n; ++i)
        c[i] += (int)(cc[i].real() + 0.5);
    for (int i = 0; i < n - 1; ++i)
        if (c[i] > 9)
        {
            c[i + 1] += c[i] / 10;
            c[i] %= 10;
        }
    while (c[c.size() - 1] > 9)
    {
        c.push_back(c[c.size() - 1] / 10);
        c[c.size() - 2] %= 10;
    }

    while (c.back() == 0 && c.size() > 1)
        c.pop_back();

    return c;
}

int main(int argc, char* argv[])
{
    // Умножение длинных чисел
    const int N = 4, A = 9; // длина и кол-во чисел
    const int pair = A / 2;
    bool even = 1;
    if (A % 2 == 1)
        even = 0;

    int* array = new int[N]; // число, которое будем рассылать
    vecint a, b, c; // для промежуточных вычислений
    int* result1 = new int[N]; // на 1м потоке
    int* result2 = new int[N]; // на 2м потоке
    int size1 = 0, size2 = 0;

    int ProcNum, ProcRank;
    MPI_Status Status;
    // Инициализация среды
    MPI_Init(&argc, &argv);
    // Определение кол-ва процессов
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    // Определение ранга процесса
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    // Создание двухмерной решетки 2x2
    MPI_Comm GridComm;
    const int ndims = 2;
    int dims[ndims], periodic[ndims], reorder = 1, maxdims = 2;
    dims[0] = dims[1] = 2;
    periodic[0] = periodic[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periodic, reorder, &GridComm);
    
    int coords[ndims * ndims][2];
    if (ProcRank == 0) // (0,0)
    {
        printf("Topology: \n");
        for (int rank = 0; rank < ndims*ndims; rank++) {
            MPI_Cart_coords(GridComm, rank, ndims, coords[rank]);
            printf("#%d - (%d, %d)\n", rank, coords[rank][0], coords[rank][1]);
        }

        // Заполнение числа и рассылка процессам
        for (int i = 0; i < N; i++)
            array[i] = 5; // + rand() % 5;
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(array, N, MPI_INT, 0, MPI_COMM_WORLD);

    if (ProcRank == 1 || ProcRank == 2) // (0,1) и (1,0)
    {
        size1 = N;

        for (int i = 0; i < size1; i++)
            a.push_back(array[i]);
        for (int i = 0; i < size1; i++)
            b.push_back(array[i]);

        printf("#%d - multy\n", ProcRank);
        for (int i = 0; i < pair - 1; i++)
        {
            c = fft_mul(a, b);
            for (int i = c.size() - 1; i >= 0; i--)
                printf("[%d]", c[i]);
            printf("\n");
            size1 = c.size();
            a = c;
        }

        if (ProcRank == 1) // (0,1)
        {
            result1 = new int[size1];
            for (int i = 0; i < size1; i++)
                result1[i] = c.at(i);

            MPI_Send(&size1, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            MPI_Send(result1, size1, MPI_INT, 3, 0, MPI_COMM_WORLD);
        }
        
        if (ProcRank == 2) // (1,0)
        {
            if (!even) // нечётная
            {
                c = fft_mul(a, b);
                for (int i = c.size() - 1; i >= 0; i--)
                    printf("[%d]", c[i]);
                printf("\n");
                size2 = c.size();

            }
            result2 = new int[size2];
            for (int i = 0; i < size2; i++)
                result2[i] = c.at(i);

            MPI_Send(&size2, 1, MPI_INT, 3, 0, MPI_COMM_WORLD);
            MPI_Send(result2, size2, MPI_INT, 3, 0, MPI_COMM_WORLD);
        }
    }
    
    if (ProcRank == 3) // (1,1)
    {
        printf("#%d - result multy", ProcRank);
        
        // С (0,1)
        MPI_Recv(&size1, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &Status);
        result1 = new int[size1];
        MPI_Recv(result1, size1, MPI_INT, 1, 0, MPI_COMM_WORLD, &Status);
        printf("\nA: ");
        for (int i = size1 - 1; i >= 0; i--)
            printf("[%d]", result1[i]);

        // С (1,0)
        MPI_Recv(&size2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD, &Status);
        result2 = new int[size2];
        MPI_Recv(result2, size2, MPI_INT, 2, 0, MPI_COMM_WORLD, &Status);
        printf("\nB: ");
        for (int i = size2 - 1; i >= 0; i--)
            printf("[%d]", result2[i]);

        for (int i = 0; i < size1; i++)
            a.push_back(result1[i]);
        for (int i = 0; i < size2; i++)
            b.push_back(result2[i]);

        c = fft_mul(a, b);
        printf("\nR: ");
        for (int i = c.size() - 1; i >= 0; i--)
            printf("[%d]", c[i]);
        printf("\n");

        // Удаление коммуникатора
        MPI_Comm_free(&GridComm);
    }

    delete[] array;
    delete[] result1;
    delete[] result2;
    MPI_Finalize();
    return 1;
}
