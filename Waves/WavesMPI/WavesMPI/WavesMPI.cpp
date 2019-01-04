// WavesMPI.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mpi.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <time.h>
using namespace std;

#define THREADS 100000

float threshold = (float) 0.0001;
float diff = (float) INT_MAX;

typedef struct in_data
{
	int rows, cols;
	float **matrix;
}IN_DATA, *PIN_DATA;

float **initialize(int n)
{
	int x, y;
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
			matrix[i][j] = 1;
		}
	}

	x = rand() % (n + 1) + 1;
	y = rand() % (n + 1) + 1;

	matrix[x][y] = 5;

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

void solve(PIN_DATA inData)
{

}

int main(int argc, char** argv)
{
	int n, p, sub, rank;
	clock_t start, end;
	float time, mydiff, tempdiff;
	int done = 0;

	if (argc != 2)
	{
		printf("Usage: mpiexec -n <nr_of_processes> waves.exe <size_of_matrix>\n");
		return -1;
	}

	MPI_Init(&argc, &argv);

	n = atoi(argv[1]);

	MPI_Comm_size(MPI_COMM_WORLD, &p);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (n % p != 0)
	{
		printf("%d is not divisible by %d\n", n, p);
		return -1;
	}

	sub = n / p;

	float **submat = (float**)malloc(sizeof(float*) * (sub + 2));
	for (int i = 0; i < sub + 2; i++)
	{
		submat[i] = (float*)malloc(sizeof(float) * (n + 2));
		for (int j = 0; j < n + 2; j++)
		{
			submat[i][j] = (float) (rand() % 5 + 1);
		}
	}

	start = clock();
	while (!done)
	{
		cout << "Started" << rank << endl;

		mydiff = 0;
		if (rank != 0)
		{
			printf("Sending %s from %d to %d\n", "[1][0]", rank, rank - 1);
			MPI_Send(&submat[1][0], n + 2, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD);
		}
		if (rank != p - 1)
		{
			printf("Sending [%d][%d] from %d to %d\n", sub, 0, rank, rank + 1);
			MPI_Send(&submat[sub][0], n + 2, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD);
		}
		if (rank != 0)
		{
			printf("Sending [%d][%d] from %d to %d\n", 0, 0, rank, rank - 1);
			MPI_Recv(&submat[0][0], n + 2, MPI_FLOAT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		if (rank != p - 1)
		{
			printf("Sending [%d][%d] from %d to %d\n", sub + 1, 0, rank, rank + 1);
			MPI_Recv(&submat[sub + 1][0], n + 2, MPI_FLOAT, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		for (int i = 1; i < sub + 1; i++)
		{
			for (int j = 1; j < n + 1; j++)
			{
				float temp = submat[i][j];
				submat[i][j] = (float) 0.2 * (submat[i][j] + submat[i][j - 1] + submat[i - 1][j] +
					submat[i][j + 1] + submat[i + 1][j]);
				mydiff += abs(submat[i][j] - temp);
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		if (rank != 0)
		{
			printf("Sending mydiff = %f from %d to %d\n", mydiff, rank, 0);
			MPI_Send(&mydiff, 1, MPI_FLOAT, 0, 2, MPI_COMM_WORLD);
			MPI_Recv(&mydiff, 1, MPI_FLOAT, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else
		{
			for (int i = 1; i < p; i++)
			{
				printf("Process 0 is waiting for tempdiff from %d\n", i);
				MPI_Recv(&tempdiff, 1, MPI_FLOAT, i, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				mydiff += tempdiff;
			}
			for (int i = 1; i < p; i++)
			{
				printf("Sending mydiff = %f from %d to %d\n", mydiff, rank, i);
				MPI_Send(&mydiff, 1, MPI_FLOAT, i, 1, MPI_COMM_WORLD);
			}
		}
		if (mydiff / (n * n) < threshold) done = 1;
	}
	end = clock();

	time = ((float)end - start) / CLOCKS_PER_SEC;

	printf("Matrix solved in %f s!\n", time);
	printMat(n, n, submat);

	MPI_Finalize();

	return 0;
}

