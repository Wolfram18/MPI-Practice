#define MSMPI_NO_DEPRECATE_20
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>
#include "mpi.h"

struct Complex
{
	int Re;
	int Im;
};

struct Complex MultiplicationComplex(struct Complex a, struct Complex b)
{
	Complex c;
	c.Re = a.Re * b.Re - a.Im * b.Im;
	c.Im = a.Re * b.Im + b.Re * a.Im;
	return c;
}

void MultiplicationMatrix(struct Complex* A, struct Complex* B, Complex* D, int N)
{
	Complex Sum, C;
	Sum.Re = 0;
	Sum.Im = 0;

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			Sum.Re = 0;
			Sum.Im = 0;
			for (int z = 0; z < N; z++)
			{
				C = MultiplicationComplex(A[i * N + z], B[z * N + j]);
				Sum.Re += C.Re;
				Sum.Im += C.Im;
			}
			D[i * N + j].Re = Sum.Re;
			D[i * N + j].Im = Sum.Im;
		}
}

int main(int argc, char* argv[])
{
	//Умножение комплексных матриц
	int N = 3, A = 10; // N - размер матрицы, А - кол-во матриц
	int pair = A / 2 + 1;
	bool even = 1;
	if (A % 2 == 1)
		even = 0;

	Complex* matrix1 = new Complex[N * N];
	Complex* matrix2 = new Complex[N * N];
	Complex* array = new Complex[N * N * pair];
	Complex* matrix3 = new Complex[N * N];

	int ProcNum, ProcRank;
	MPI_Status Status;
	//Инициализация среды
	MPI_Init(&argc, &argv);
	//Определение кол-ва процессов
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	//Определение ранга процесса
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

	if (ProcNum < pair)
	{
		MPI_Finalize();
		delete[] matrix1;
		delete[] matrix2;
		delete[] array;
		delete[] matrix3;
		return 0;
	}

	// Структурный способ конструирования производного типа
	// Кол-во блоков
	int const count = 2;
	// Кол-во элементов в каждом блоке
	int blocklens[count];
	blocklens[0] = 1;
	blocklens[1] = 1;
	// Смещение каждого блока от начала типа
	MPI_Aint indices[count];
	// Получение адреса переменных
	Complex Matrix;
	MPI_Address(&Matrix.Re, &indices[0]);
	MPI_Address(&Matrix.Im, &indices[1]);
	indices[1] = indices[1] - indices[0];
	indices[0] = 0;
	// Исходные типы
	MPI_Datatype old_types[count];
	old_types[0] = MPI_INT;
	old_types[1] = MPI_INT;
	// Новый тип
	MPI_Datatype complex_type;
	// Создание
	MPI_Type_struct(count, blocklens, indices, old_types, &complex_type);
	// Объявление
	MPI_Type_commit(&complex_type);

	// Получить группу, связанную с существующим коммуникатором
	MPI_Group group;
	MPI_Comm_group(MPI_COMM_WORLD, &group);
	// Создать новую группу из процессов с рангами ranks
	int* ranks = new int[pair];
	for (int i = 0; i < pair; i++)
		ranks[i] = i;
	MPI_Group complex_group;
	MPI_Group_incl(group, pair, ranks, &complex_group);
	// Создать новый коммуникатор
	MPI_Comm COMM;
	MPI_Comm_create(MPI_COMM_WORLD, complex_group, &COMM);

	// Заполнение матрицы и рассылка всем процессам
	if (ProcRank == 0)
	{
		printf("  Matrix %d*%d (%d)\n", N, N, A);
		for (int i = 0; i < N * N; i++)
		{
			matrix1[i].Re = 1 + rand() % 5;
			matrix1[i].Im = 1 + rand() % 5;
			printf("  Re,Im[%d] - (%d,%d)\n", i, matrix1[i].Re, matrix1[i].Im);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(matrix1, N * N, complex_type, 0, MPI_COMM_WORLD);

	// Подсчёт на процессах
	if (ProcRank >= 1 && ProcRank < pair)
	{
		MultiplicationMatrix(matrix1, matrix1, matrix2, N);
		printf("  Rank[%d] - multi done\n", ProcRank);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// Сбор результирующих матриц на 0м процессе
	if (ProcRank >= 0 && ProcRank < pair)
	{
		MPI_Gather(matrix2, N * N, complex_type, array, N * N, complex_type, 0, COMM);
	}

	if (ProcRank == 0)
	{
		// Если А - чётное
		if (even)
		{
			for (int i = 0; i < N * N; i++)
			{
				// Пропускаем матрицу, пришедшую с 0го процесса
				matrix1[i].Re = array[N * N + i].Re;
				matrix1[i].Im = array[N * N + i].Im;
			}
		}
		else // Добавляем нечётную
		{
			for (int i = 0; i < N * N; i++)
			{
				// Пришедшую с 0го процесса
				matrix1[i].Re = array[i].Re;
				matrix1[i].Im = array[i].Im;
			}
		}

		// Перемножаем результирующие матрицы
		for (int i = even + 1; i < pair; i++)
		{
			for (int j = 0; j < N * N; j++)
			{
				matrix2[j].Re = array[i * N * N + j].Re;
				matrix2[j].Im = array[i * N * N + j].Im;
			}
			MultiplicationMatrix(matrix1, matrix2, matrix3, N);
			for (int j = 0; j < N * N; j++)
			{
				matrix1[j].Re = matrix3[j].Re;
				matrix1[j].Im = matrix3[j].Im;
			}
		}

		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
				printf("  Re,Im[%d] - (%d,%d)\n", i * N + j, matrix1[i * N + j].Re, matrix1[i * N + j].Im);

		// Аннулирование производного типа
		MPI_Type_free(&complex_type);
		// Удаление групп
		MPI_Group_free(&group);
		MPI_Group_free(&complex_group);
		// Удаление коммуникатора
		MPI_Comm_free(&COMM);
	}

	// Завершение
	MPI_Finalize();
	delete[] matrix1;
	delete[] matrix2;
	delete[] array;
	delete[] matrix3;
	delete[] ranks;
	return 0;
}