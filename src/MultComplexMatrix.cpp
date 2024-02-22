#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <mpi.h>

#define DEFAULT_MATRIX_SIZE 2

void help()
{
	printf("\n Complex matrix multiplication.");
	printf("\n\n The matrix size (the \"N\" parameter)");
	printf("\n can be specified after the program name at startup.");
	printf("\n By default the matrix size is: %d.\n", DEFAULT_MATRIX_SIZE);
}

struct Complex
{
	int Re;
	int Im;
};

struct Complex MultiplicationComplex(struct Complex a, struct Complex b)
{
	struct Complex c;
	c.Re = a.Re * b.Re - a.Im * b.Im;
	c.Im = a.Re * b.Im + b.Re * a.Im;
	return c;
}

void MultiplicationMatrix(struct Complex* A, struct Complex* B, struct Complex* D, int N)
{
	struct Complex Sum, C;
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
	int N = (argc == 2) ? atoi(argv[1]) : DEFAULT_MATRIX_SIZE;

	struct Complex* matrix1 = malloc(sizeof(struct Complex)*(N * N));
	struct Complex* matrix2 = malloc(sizeof(struct Complex)*(N * N));
	struct Complex* matrix3 = malloc(sizeof(struct Complex)*(N * N));

	MPI_Status Status;

	// Initialize the MPI environment
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	struct Complex* array = malloc(sizeof(struct Complex)*(N * N * world_size));

	if (world_size < 1)
	{
		MPI_Finalize();
		free(matrix1);
		free(matrix2);
		free(matrix3);
		free(array);
		if (world_rank == 0)
			printf("\n Exit, because the number of processes is less than expected.");
		return 0;
	}

	// Структурный способ конструирования производного типа
	int const count = 2; // кол-во блоков
	int blocklens[count]; // кол-во элементов в каждом блоке
	blocklens[0] = 1;
	blocklens[1] = 1;
	MPI_Aint indices[count]; // смещение каждого блока от начала типа
	struct Complex Matrix; // получение адреса переменных
	MPI_Get_address(&Matrix.Re, &indices[0]);
	MPI_Get_address(&Matrix.Im, &indices[1]);
	indices[1] = indices[1] - indices[0];
	indices[0] = 0;
	MPI_Datatype old_types[count]; // исходные типы
	old_types[0] = MPI_INT;
	old_types[1] = MPI_INT;
	MPI_Datatype complex_type; // новый тип
	MPI_Type_struct(count, blocklens, indices, old_types, &complex_type); // создание
	MPI_Type_commit(&complex_type); // объявление

	MPI_Group group; // получить группу, связанную с существующим коммуникатором
	MPI_Comm_group(MPI_COMM_WORLD, &group);
	int* ranks = malloc(sizeof(int)*world_size); // создать новую группу из процессов с рангами ranks
	for (int i = 0; i < world_size; i++)
		ranks[i] = i;
	MPI_Group complex_group;
	MPI_Group_incl(group, world_size, ranks, &complex_group);
	MPI_Comm COMM; // создать новый коммуникатор
	MPI_Comm_create(MPI_COMM_WORLD, complex_group, &COMM); // Не работает

	// Broadcasts a message from the process with rank "root"
	// to all other processes of the communicator
	if (world_rank == 0)
	{
		help();
		printf("\n Matrix %d*%d (%d times):\n", N, N, world_size*2);
		for (int i = 0; i < N * N; i++)
		{
			matrix1[i].Re = 1 + rand() % 5;
			matrix1[i].Im = 1 + rand() % 5;
			printf(" Re,Im[%d] - (%d,%d)\n", i, matrix1[i].Re, matrix1[i].Im);
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(matrix1, N * N, complex_type, 0, MPI_COMM_WORLD);

	// Matrix multiplication on processes
	if (world_rank >= 1 && world_rank < world_size)
	{
		MultiplicationMatrix(matrix1, matrix1, matrix2, N);
		printf(" Multiplication from process [%d] completed\n", world_rank);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// Gathers together values from a group of processes
	if (world_rank >= 0 && world_rank < world_size)
	{
		MPI_Gather(matrix2, N * N, complex_type, array, N * N, complex_type, 0, COMM);
	}

	if (world_rank == 0)
	{
		for (int i = 0; i < N * N; i++)
		{
			matrix1[i].Re = array[N * N + i].Re;
			matrix1[i].Im = array[N * N + i].Im;
		}
		for (int i = 2; i < world_size; i++)
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
		printf("\n Result Matrix:\n");
		for (int i = 0; i < N; i++)
			for (int j = 0; j < N; j++)
				printf(" Re,Im[%d] - (%d,%d)\n", i * N + j, matrix1[i * N + j].Re, matrix1[i * N + j].Im);
		
		MPI_Type_free(&complex_type); // аннулирование производного типа
		MPI_Group_free(&group); // удаление групп
		MPI_Group_free(&complex_group);
		MPI_Comm_free(&COMM); // удаление коммуникатора
	}

	MPI_Finalize();
	free(matrix1);
	free(matrix2);
	free(array);
	free(matrix3);
	free(ranks);
	return 0;
}
