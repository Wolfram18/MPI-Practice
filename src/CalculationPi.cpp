#include <stdio.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

#define DEFAULT_N 100
#define DEFAULT_METHOD 1
#define PI 3.1415926535897932384626433832795

void help()
{
    printf("\n Approximate calculation of Pi");
    printf("\n using the definite integral method.");
    printf("\n\n The number of iterations (the \"N\" parameter)");
    printf("\n can be specified after the program name at startup.");
    printf("\n By default the number of iterations is: %d.", DEFAULT_N);
    printf("\n\n The definite integral method (the \"method\" parameter)");
    printf("\n can be specified after the program name at startup:");
    printf("\n 1. Rectangle Method");
    printf("\n 2. Trapezoidal Method");
    printf("\n 3. Simpson Method");
    printf("\n By default the method is: %d.\n", DEFAULT_METHOD);
}

double f(double x)
{
    return 1/(1+x*x);
}

double rectangle_method(int N, int world_size, int world_rank) 
{
	double h, sum = 0;
	int a = 0, b = 1;
	h = 1.0 / N;
	for (int i = world_rank + 1; i <= N; i += world_size)
		sum += f(a + h * (i - 0.5));
	return 4 * h * sum;
}

double trapezoidal_method(int N, int world_size, int world_rank) 
{
	double h, sum = 0;
	int a = 0, b = 1;
	h = 1.0 / N;
	for (int i = world_rank + 1; i < N; i += world_size)
		sum += f(a + i * h);
	return 4 * h * ((f(a) + f(b)) / 2 + sum);
}

double simpson_method(int N, int world_size, int world_rank) 
{
	double h, sum = 0, summ = 0;
	int a = 0, b = 1;
	h = 1.0 / N;
	for (int i = world_rank + 1; i < N; i += world_size)
	{
		sum += f(a + h * (i - 0.5));
		summ += f(a + i * h);
	}
	sum += f(a + h * (N - 0.5));
	return 4 * (h / 3) * ((f(a) + f(b)) / 2 + (2 * sum) + summ);
}

int main(int argc, char *argv[])
{
    int N = (argc > 1) ? atoi(argv[1]) : DEFAULT_N;
    int method = (argc > 1) ? atoi(argv[2]) : DEFAULT_METHOD;
    double MyPi, term;
    clock_t StartClock, EndClock;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    StartClock = clock();
    // Broadcasts a message from the process with rank "root"
    // to all other processes of the communicator
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (method == 1)
        term = rectangle_method(N, world_size, world_rank);
    else if (method == 2)
        term = trapezoidal_method(N, world_size, world_rank);
    else
        term = simpson_method(N, world_size, world_rank);

    // Reduces values on all processes to a single value
    MPI_Reduce(&term, &MyPi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0)
    {
        EndClock = clock();
        help();
        printf("\n Number of iterations = %d", N);
        printf("\n Computed value of Pi = %3.20f", MyPi);
        printf("\n Reference value of Pi = %3.20f", PI);
        printf("\n Error = %3.20f", fabs(MyPi - PI));
        double TotalTime;
        TotalTime = (double)(EndClock - StartClock) / CLOCKS_PER_SEC;
        printf("\n Computing time = %f sec\n", TotalTime);
    }

    MPI_Finalize();
    return 0;
}
