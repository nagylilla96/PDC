// Histogram.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>

#define		MSG_MAX		2048
#define		COLORS		256

int* readFile(char* fileName, int* lns)
{
	FILE *f;
	int lines = 0;
	fopen_s(&f, fileName, "r");
	fscanf_s(f, "%d", &lines);
	*lns = lines;
	int *numbers = (int*)malloc(sizeof(int) * lines);
	for (int i = 0; i < lines; i++)
	{
		fscanf_s(f, "%d", &numbers[i]);
	}
	fclose(f);
	return numbers;
}

void createSubarray(int it, int *subarray, int *original, int n)
{
	int ind = 0;
	for (int j = (it - 1) * n; j < it * n; j++)
	{
		subarray[ind] = original[j];
		ind++;
	}
}

void initHist(int *hist, int n)
{
	for (int i = 0; i < n; i++)
	{
		hist[i] = 0;
	}
}

void createHist(int *hist, int *colors, int n)
{
	for (int i = 0; i < n; i++)
	{
		hist[colors[i]] += 1;
	}
}

void gatherHists(int *fullData, int *partialData, int n)
{
	for (int i = 0; i < n; i++)
	{
		fullData[i] += partialData[i];
	}
}

void printHist(int *hist, int n)
{
	printf("Results:\n");
	for (int i = 0; i < COLORS; i++)
	{
		printf("%d ", hist[i]);
	}
}

int main(int argc, char* argv[])
{
	char *fileName = (char*)malloc(MSG_MAX);

	if (argc != 2)
	{
		printf("Usage: mpiexec -n <nr_of_slaves> Histogram.exe <input_file>\n");
		return -1;
	}
	strcpy_s(fileName, MSG_MAX, argv[1]);
	int colors[COLORS] = { 0 };

	MPI_Init(&argc, &argv);

	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	int rank;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0)
	{
		int lines = 0;
		int *numbers = readFile(fileName, &lines);
		printf("lines = %d\n", lines);
		int procElements = lines / (world_size - 1);
		for (int i = 1; i <= world_size - 1; i++)
		{
			MPI_Send(&procElements, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			int *subarray = (int*)malloc(sizeof(int) * procElements);
			if (subarray == NULL)
			{
				printf("Malloc failed\n");
				return -1;
			}
			createSubarray(i, subarray, numbers, procElements);
			int recvNumber = 0;
			MPI_Send(subarray, procElements, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		for (int i = 1; i < world_size - 1; i++) {
			int *histogram = (int*)malloc(sizeof(int) * COLORS);
			if (histogram == NULL)
			{
				printf("Malloc failed\n");
				return -1;
			}
			MPI_Recv(histogram, COLORS, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			gatherHists(colors, histogram, COLORS);
			free(histogram);
		}
		printHist(colors, COLORS);
	}
	else
	{
		int procElements = 0;
		MPI_Recv(&procElements, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		int *subarray = (int*)malloc(sizeof(int) * procElements);
		if (subarray == NULL)
		{
			printf("Malloc failed\n");
			return -1;
		}
		MPI_Recv(subarray, procElements, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		int *histogram = (int*)malloc(sizeof(int) * COLORS);
		if (histogram == NULL)
		{
			printf("Malloc failed\n");
			return -1;
		}
		initHist(histogram, COLORS);
		createHist(histogram, subarray, procElements);
		MPI_Send(histogram, COLORS, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize();

	free(fileName);

	return 0;
}