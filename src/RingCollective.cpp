#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include "mpi.h"

void help()
{
	printf("\n Function \"Ring\": 0 --> 1, 1-->2, 2-->3 and etc. --> 1");
	printf("\n using the Collective communication pattern.");
        printf("\n It repeats optional amount of times");
        printf("\n and prints final values on threads and then sum them up.");
        printf("\n\n Waiting for the number of repetitions to be entered.");
	printf("\n By default the Ring function is repeated once (1).");
}

int main(int argc, char* argv[])
{
	help();
	int count = (argc == 2) ? atoi(argv[1]) : 1;
	int from, to;
	MPI_Status Status;
	//char mess[] = "openmpi";
	//char recv[10];
	int mess = 0, recv = 0, sum = 0;
	int num[10];

	// Initialize the MPI environment
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	if (world_rank == 0)
	{
		MPI_Send(&mess, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
		printf("\n Start send from root:  %d -> %d", world_rank, 1);
	}
	MPI_Barrier(MPI_COMM_WORLD);

	if (world_rank != 0)
	{
		for (int i = 0; i < count; i++)
		{
			if (i > 0 && world_rank == 1)
				from = world_size - 1;
			else
				from = world_rank - 1;
			MPI_Recv(&recv, 1, MPI_INT, from, 0, MPI_COMM_WORLD, &Status);
			recv++;
			printf("\n Recv (round %d):  %d -> %d, %d+1", i, from, world_rank, recv - 1);

			if (world_rank == world_size - 1)
				to = 1;
			else
				to = world_rank + 1;
			MPI_Send(&recv, 1, MPI_INT, to, 0, MPI_COMM_WORLD);
			printf("\n Send (round %d):  %d -> %d", i, world_rank, to);
		}
	}

	// Gathers together values from a group of processes
	MPI_Gather(&recv, 1, MPI_INT, &num, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if (world_rank == 0)
	{
		printf("\n Result: ");
		for (int i = 0; i < world_size; i++)
			printf("%d, ", num[i]);
	}

	// Reduces values on all processes to a single value
	MPI_Reduce(&recv, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Barrier(MPI_COMM_WORLD);

	if (world_rank == 0)
		printf("\n Result = %d", sum);

	MPI_Finalize();
	return 0;
}
