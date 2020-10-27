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
	//��������� ����������� ������
	int N = 3, A = 10; // N - ������ �������, � - ���-�� ������
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
	//������������� �����
	MPI_Init(&argc, &argv);
	//����������� ���-�� ���������
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	//����������� ����� ��������
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

	// ����������� ������ ��������������� ������������ ����
	// ���-�� ������
	int const count = 2;
	// ���-�� ��������� � ������ �����
	int blocklens[count];
	blocklens[0] = 1;
	blocklens[1] = 1;
	// �������� ������� ����� �� ������ ����
	MPI_Aint indices[count];
	// ��������� ������ ����������
	Complex Matrix;
	MPI_Address(&Matrix.Re, &indices[0]);
	MPI_Address(&Matrix.Im, &indices[1]);
	indices[1] = indices[1] - indices[0];
	indices[0] = 0;
	// �������� ����
	MPI_Datatype old_types[count];
	old_types[0] = MPI_INT;
	old_types[1] = MPI_INT;
	// ����� ���
	MPI_Datatype complex_type;
	// ��������
	MPI_Type_struct(count, blocklens, indices, old_types, &complex_type);
	// ����������
	MPI_Type_commit(&complex_type);

	// �������� ������, ��������� � ������������ ��������������
	MPI_Group group;
	MPI_Comm_group(MPI_COMM_WORLD, &group);
	// ������� ����� ������ �� ��������� � ������� ranks
	int* ranks = new int[pair];
	for (int i = 0; i < pair; i++)
		ranks[i] = i;
	MPI_Group complex_group;
	MPI_Group_incl(group, pair, ranks, &complex_group);
	// ������� ����� ������������
	MPI_Comm COMM;
	MPI_Comm_create(MPI_COMM_WORLD, complex_group, &COMM);

	// ���������� ������� � �������� ���� ���������
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

	// ������� �� ���������
	if (ProcRank >= 1 && ProcRank < pair)
	{
		MultiplicationMatrix(matrix1, matrix1, matrix2, N);
		printf("  Rank[%d] - multi done\n", ProcRank);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	// ���� �������������� ������ �� 0� ��������
	if (ProcRank >= 0 && ProcRank < pair)
	{
		MPI_Gather(matrix2, N * N, complex_type, array, N * N, complex_type, 0, COMM);
	}

	if (ProcRank == 0)
	{
		// ���� � - ������
		if (even)
		{
			for (int i = 0; i < N * N; i++)
			{
				// ���������� �������, ��������� � 0�� ��������
				matrix1[i].Re = array[N * N + i].Re;
				matrix1[i].Im = array[N * N + i].Im;
			}
		}
		else // ��������� ��������
		{
			for (int i = 0; i < N * N; i++)
			{
				// ��������� � 0�� ��������
				matrix1[i].Re = array[i].Re;
				matrix1[i].Im = array[i].Im;
			}
		}

		// ����������� �������������� �������
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

		// ������������� ������������ ����
		MPI_Type_free(&complex_type);
		// �������� �����
		MPI_Group_free(&group);
		MPI_Group_free(&complex_group);
		// �������� �������������
		MPI_Comm_free(&COMM);
	}

	// ����������
	MPI_Finalize();
	delete[] matrix1;
	delete[] matrix2;
	delete[] array;
	delete[] matrix3;
	delete[] ranks;
	return 0;
}