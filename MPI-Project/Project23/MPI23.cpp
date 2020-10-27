#define MSMPI_NO_DEPRECATE_20
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clocale>
#include "mpi.h"

int main(int argc, char* argv[])
{
	//Функция Ring, проходит ring раз
	//Выводятся итоговые значения на потоках
	//Потом суммируются 
	int ring = 2; //Число повторов
	int ProcNum, ProcRank, ProcFrom, ProcTo;
	MPI_Status Status;
	//char mess[] = "abcdef";
	//char recv[10];
	int mess = 0, recv = 0, sum = 0;
	int num[10];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
	MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

	if (ProcRank == 0)
	{
		//Действия, выполняемые только процессом с рангом 0
		MPI_Send(&mess, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("\n Start send:  %d -> %d", ProcRank, 1);
	}
	else //Сообщение, отправляемое всеми процессами, кроме процесса с рангом 0
	{
		for (int i = 0; i < ring; i++)
		{
			if (i > 0 && ProcRank == 1)
				ProcFrom = ProcNum - 1;
			else
				ProcFrom = ProcRank - 1;
			MPI_Recv(&recv, 1, MPI_INT, ProcFrom, 0, MPI_COMM_WORLD, &Status);
			recv++;
			printf("\n Recv[%d]:  %d -> %d, %d+1", i, ProcFrom, ProcRank, recv - 1);

			if (ProcRank == ProcNum - 1)
				ProcTo = 1;
			else
				ProcTo = ProcRank + 1;
			MPI_Send(&recv, 1, MPI_INT, ProcTo, 0, MPI_COMM_WORLD);
			printf("\n Send[%d]:  %d -> %d", i, ProcRank, ProcTo);
		}
	}

	//Собираем последние результаты на процессах
	MPI_Gather(&recv, 1, MPI_INT, &num, 1, MPI_INT, 0, MPI_COMM_WORLD);
	//Ждем все процессы
	MPI_Barrier(MPI_COMM_WORLD);
	printf("\n Result: ");
	if (ProcRank == 0)
		for (int i = 0; i < ProcNum; i++)
			printf("%d, ", num[i]);

	//Суммируем последние результаты на процессах
	MPI_Reduce(&recv, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);
	if (ProcRank == 0)
		printf("\n Result = %d", sum);

	MPI_Finalize();
	return 0;
}
