#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <fstream>
using namespace std;

#define THREADS 100000

float threshold = 1.0 / 1000;
float diff = 0;
int p, n;
float **matrix;
std::mutex mtx;
LPSYNCHRONIZATION_BARRIER lpBarrier;

typedef struct in_data
{
	int pid;
	float **matrix;
}IN_DATA, *PIN_DATA;

float **initialize(int n)
{
	srand(time(NULL));

	float ** matrix = (float**)malloc((n + 2) * sizeof(float*));
	for (int i = 0; i < n + 2; i++)
	{
		matrix[i] = (float*)malloc((n * 2) * sizeof(float));
	}
	for (int i = 0; i < n + 2; i++)
	{
		for (int j = 0; j < n + 2; j++)
		{
			matrix[i][j] = rand() % 5;
		}
	}

	return matrix;
}

void printMat(int rows, int cols, float **mat)
{
	for (int i = 1; i <= rows; i++)
	{
		for (int j = 1; j <= cols; j++)
		{
			printf("%f ", mat[i][j]);
		}
		printf("\n");
	}
}

void solve(int pid)
{
	int done = 0;
	int sub = n / p;
	float temp, mydiff = 0;
	int mymin = 1 + (pid * (n / p));
	int mymax = mymin + (n / p) - 1;

	while (!done)
	{
		mydiff = diff = 0;
		for (int i = mymin; i <= mymax; i++)
		{
			for (int j = 1; j <= n; j++)
			{
				temp = matrix[i][j];
				matrix[i][j] = 0.2 * (matrix[i][j] + matrix[i][j - 1] + matrix[i - 1][j] +
					matrix[i][j + 1] + matrix[i + 1][j]);
				mydiff += abs(matrix[i][j] - temp);
			}
		}
		mtx.lock();
		diff += mydiff;
		mtx.unlock();
		EnterSynchronizationBarrier(lpBarrier, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
		if (diff / (n * n) < threshold) done = 1;
		EnterSynchronizationBarrier(lpBarrier, SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
	}
}

int main(int argc, char** argv)
{
	clock_t start, end;
	float time;
	int done = 0;
	std::thread ths[THREADS];

	InitializeSynchronizationBarrier(lpBarrier, p, -1);

	if (argc != 3)
	{
		printf("Usage: waves.exe <nr_of_processes> <size_of_matrix>\n");
		return -1;
	}

	n = atoi(argv[2]);
	p = atoi(argv[1]);

	if (n % p != 0)
	{
		printf("%d is not divisible by %d\n", n, p);
		return -1;
	}

	matrix = initialize(n);

	start = clock();
	for (int i = 0; i < p; i++)
	{
		ths[i] = std::thread(solve, i);
	}
	for (int i = 0; i < p; i++)
	{
		ths[i].join();
	}
	end = clock();

	time = ((float)end - start) / CLOCKS_PER_SEC;

	ofstream file;
	printMat(n, n, matrix);
	/*file.open("threadresults.txt", ios::app);
	file << time << endl;*/

	getchar();
	return 0;
}