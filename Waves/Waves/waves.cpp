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

float threshold = 0.0001;
float diff = INT_MAX;
std::mutex mtx;

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
	float myDiff = 0;
	//printMat(inData->rows, inData->cols, inData->matrix);
	for (int i = 1; i < inData->rows + 1; i++)
	{
		for (int j = 1; j < inData->cols + 1; j++)
		{
			mtx.lock();
			float temp = inData->matrix[i][j];
			inData->matrix[i][j] = 0.2 * (inData->matrix[i][j] + inData->matrix[i][j - 1] + inData->matrix[i - 1][j] +
				inData->matrix[i][j + 1] + inData->matrix[i + 1][j]);
			diff += abs(inData->matrix[i][j] - temp);
			mtx.unlock();
		}
	}
	mtx.lock();
	diff += myDiff;
	mtx.unlock();
}

int main(int argc, char** argv)
{
	int n, p, sub;
	clock_t start, end;
	PIN_DATA inData = (PIN_DATA)malloc(sizeof(IN_DATA));
	float time;
	float **matrix;
	int done = 0;

	if (argc != 3)
	{
		printf("Usage: waves.exe <nr_of_processes> <size_of_matrix>\n");
		return -1;
	}

	n = atoi(argv[2]);
	p = atoi(argv[1]);
	std::thread ths[THREADS];

	if (n % p != 0)
	{
		printf("%d is not divisible by %d\n", n, p);
		return -1;
	}

	sub = n / p;

	matrix = initialize(n);

	inData->rows = sub;
	inData->cols = n;

	start = clock();
	while (!done)
	{
		diff = 0;
		for (int i = 0; i < p; i++)
		{
			float **submat = (float**)malloc(sizeof(float*) * (sub + 2));
			for (int j = 0; j < sub + 2; j++)
			{
				submat[j] = (float*)malloc(sizeof(float) * (n + 2));
				submat[j] = matrix[i * sub + j];
			}
			inData->matrix = submat;
			ths[i] = std::thread(solve, inData);
		}
		for (int i = 0; i < p; i++)
		{
			ths[i].join();
		}
		if (diff / (n * n) < threshold) done = 1;
	}
	end = clock();

	time = ((float)end - start) / CLOCKS_PER_SEC;

	/*ofstream file;
	file.open("threadresults.txt", ios::app);*/
	cout << time << endl;

	return 0;
}