#include <iostream>
#include <time.h>
using namespace std;

float threshold = 0.0001;

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

void solve(int n, float **matrix)
{
	int done = 0;
	while (!done)
	{
		float diff = 0;
		for (int i = 1; i < n + 1; i++)
		{
			for (int j = 1; j < n + 1; j++)
			{
				float temp = matrix[i][j];
				matrix[i][j] = 0.2 * (matrix[i][j] + matrix[i][j - 1] + matrix[i - 1][j] +
					matrix[i][j + 1] + matrix[i + 1][j]);
				diff += abs(matrix[i][j] - temp);
			}
		}
		if (diff / (n * n) < threshold) done = 1;
	}

}

int main(int argc, char** argv)
{
	int n = 100;
	clock_t start, end;
	float time;
	float **matrix;

	matrix = initialize(n);

	start = clock();
	solve(n, matrix);
	end = clock();

	time = ((float)end - start) / CLOCKS_PER_SEC;

	printf("Matrix solved in %f s!\n", time);

	getchar();

	return 0;
}