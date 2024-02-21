#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include "mpi.h"

#define _countof(a) (sizeof(a)/sizeof(*(a)))

void help()
{
	printf("\n Function \"Ring\": 0 --> 1, 1-->2, 2-->3 and etc. --> 1");
	printf("\n using the Point-To-Point communication pattern.");
        printf("\n It repeats optional amount of times.");
        printf("\n\n Waiting for the number of repetitions to be entered.");
	printf("\n By default the Ring function is repeated once (1).");
}

int main(int argc, char* argv[])
{
	help();
	int count = (argc == 2) ? atoi(argv[1]) : 1;
	int from, to;
	MPI_Status Status;
	char mess[] = "openmpi";
	char recv[10];
	//int mess = 5, recv = 0;

	// Initialize the MPI environment
	MPI_Init(&argc, &argv);

	// Get the number of processes
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	// Get the name of the processor
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int name_len;
	MPI_Get_processor_name(processor_name, &name_len);

	printf("\n Hello world from processor %s, rank %d"
		" out of %d processors",
		processor_name, world_rank, world_size);
	MPI_Barrier(MPI_COMM_WORLD);

	if (world_rank == 0)
	{
		MPI_Send(&mess, _countof(mess), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
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
			MPI_Recv(&recv, _countof(recv), MPI_CHAR, from, 0, MPI_COMM_WORLD, &Status);
			printf("\n Recv (round %d):  %d -> %d, %s", i, from, world_rank, recv);

			if (world_rank == world_size - 1)
				to = 1;
			else
				to = world_rank + 1;
			MPI_Send(&recv, _countof(recv), MPI_CHAR, to, 0, MPI_COMM_WORLD);
			printf("\n Send (round %d):  %d -> %d", i, world_rank, to);
		}
	}

	MPI_Finalize();
	return 0;
}
