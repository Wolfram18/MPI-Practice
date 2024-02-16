#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <chrono>
#include <mpi.h>
using namespace std;

#define PI 3.141592653589793

double f(double x)
{
	return 1 / (1 + x * x);
}

int main(int argc, char* argv[])
{
	double pi, sum = 0, summ = 0, term, h, eps;
	int myrank, nprocs, n, i, a = 0, b = 1;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
	if (myrank == 0)
	{
		printf("Number of iterations = ");
		scanf("%d", &n);
	}

	auto start = chrono::system_clock::now();
	MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

	// формула средних прямоугольников
	/*h = 1.0 / n;
	for (i = myrank + 1; i <= n; i += nprocs)
		sum += f(a + h * (i - 0.5));
	term = 4 * h * sum;*/

	// формула трапеций
	/*h = 1.0 / n;
	for (i = myrank + 1; i < n; i += nprocs)
		summ += f(a + i * h);
	term = 4 * h * ((f(a) + f(b)) / 2 + summ);*/

	// формула Симпсона
	h = 1.0 / n;
	for (i = myrank + 1; i < n; i += nprocs)
	{
		sum += f(a + h * (i - 0.5));
		summ += f(a + i * h);
	}
	sum += f(a + h * (n - 0.5));
	term = 4 * (h / 3) * ((f(a) + f(b)) / 2 + (2 * sum) + summ);

	MPI_Reduce(&term, &pi, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	auto stop = chrono::system_clock::now();
	
	if (myrank == 0)
	{
		eps = abs(pi - PI);
		printf("Standard value of pi = %.16lg\n", PI);
		printf("Computed value of pi = %.16lg\n", pi);
		printf("Calculation accuracy of pi = %lg\n", eps);
		chrono::duration<double> elaps = (stop - start);
		printf("Calculation time of pi = %lg\n", elaps.count());
	}
	
	MPI_Finalize();
	return 0;
}