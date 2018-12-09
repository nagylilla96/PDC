#include "stdafx.h"
#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include <iostream>

int** readMatrix(char* fname, int *size)
{
	FILE *f;
	int n;

	fopen_s(&f, fname, "r");
	fscanf_s(f, "%d\n", &n);
	*size = n;

	int **matrix = (int**)calloc(n, sizeof(int*));
	for (int i = 0; i < n; i++)
	{
		matrix[i] = (int*)calloc(n, sizeof(int));
		for (int j = 0; j < n; j++)
		{
			fscanf_s(f, "%d", &matrix[i][j]);
		}
	}

	fclose(f);
	return matrix;
}

// Arranges the matrix to an array of the subarrays in the correct order
// or vice versa based on mode
// If mode == 0 : mat -> array
// If mode == 1 : array -> mat
void matConv(int **mat, int *arr, int p, int subsize, int mode)
{
	int proc_id = 0, row_ind = 0, col_ind = 0, ind = 0;
	for (int i = 0; i < p; i++) {
		for (int j = 0; j < p; j++) {
			proc_id = i * p + j;
			for (int ii = 0; ii < subsize; ii++) {
				row_ind = i * subsize + ii;
				for (int jj = 0; jj < subsize; jj++) {
					ind = (proc_id * subsize * subsize) + (ii * subsize) + jj;
					col_ind = j * subsize + jj;
					if (mode == 0)
					{
						arr[ind] = mat[row_ind][col_ind];
					}
					else
					{
						if (mode == 1)
						{
							mat[row_ind][col_ind] = arr[ind];
						}
					}
				}
			}
		}
	}
}

void printMatrix(int **matrix, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			printf("%d ", matrix[i][j]);
		}
		printf("\n");
	}
}

int main(int argc, char *argv[])
{

	int p, size, rank, row, col, sizematA, sizematB, sizeA, sizeB;
	int subsizeA, subsizeB, source, destination, send_tag, recv_tag;
	int sizes[2], periodic[2], dimensions[2], coords[2], select[2];

	int **matA, **matB, **matC;
	int *subarrA, *subarrB, *subarrC, *arrA, *arrB, *arrC;

	MPI_Status status;
	MPI_Comm communicator, row_comm, col_comm;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &(size));
	MPI_Comm_rank(MPI_COMM_WORLD, &(rank));

	/* Calculate the number of processors */
	p = (int)sqrt((double)size);
	if (p * p != size) {
		MPI_Finalize();
		if (rank == 0) {
			printf("Number of processors should be perfect square\n");
		}
		return -1;
	}

	dimensions[0] = p;
	dimensions[1] = p;

	periodic[0] = 1;
	periodic[1] = 1;

	// Create a new Cartesian topology of 2 dimensions, each having p processes
	MPI_Cart_create(MPI_COMM_WORLD, 2, dimensions, periodic, 1, &communicator);
	MPI_Cart_coords(communicator, rank, 2, coords);

	row = coords[0];
	col = coords[1];

	select[0] = 0;
	select[1] = 1;

	// Select the column communicator of processes
	MPI_Cart_sub(communicator, select, &row_comm);

	select[0] = 1;
	select[1] = 0;

	// Select the row communicator of processes
	MPI_Cart_sub(communicator, select, &col_comm);

	if (rank == 0) {
		matA = readMatrix("matrixA.txt", &sizeA);
		sizes[0] = sizeA;

		matB = readMatrix("matrixB.txt", &sizeB);
		sizes[1] = sizeB;
	}

	// Waits for all the processes to reach this
	MPI_Barrier(communicator);

	// We have to send the size of the matrix to all subprocesses
	MPI_Bcast(sizes, 2, MPI_INT, 0, communicator);

	sizeA = sizes[0];
	sizeB = sizes[1];

	if (sizeA != sizeB) {
		MPI_Finalize();
		if (rank == 0) {
			printf("Matrices aren't of the same size!\n");
		}
		return -1;
	}

	if (sizeA % p != 0 || sizeB % p != 0) {
		MPI_Finalize();
		if (rank == 0) {
			printf("Matrices can't be divided among processors equally!\n");
		}
		return -1;
	}

	subsizeA = sizeA / p;
	subsizeB = sizeB / p;

	sizematA = subsizeA * subsizeA;
	sizematB = subsizeB * subsizeB;

	// Allocate memory for subarrays
	subarrA = (int *)malloc(sizematA * sizeof(int));
	subarrB = (int *)malloc(sizematB * sizeof(int));

	// Allocate memory for the 1d array for the original matrices to be sent via Scatter
	arrA = (int *)malloc(sizeof(int) * sizeA * sizeA);
	arrB = (int *)malloc(sizeof(int) * sizeB * sizeB);

	// Convert matrices to arrays
	if (rank == 0) {
		matConv(matA, arrA, p, subsizeA, 0);
		matConv(matB, arrB, p, subsizeB, 0);
	}

	MPI_Barrier(communicator);

	// Scatter matrices to all subprocesses
	MPI_Scatter(arrA, sizematA, MPI_INT, subarrA, sizematA, MPI_INT, 0, communicator);
	MPI_Scatter(arrB, sizematB, MPI_INT, subarrB, sizematB, MPI_INT, 0, communicator);

	// Cannon's algorithm

	// Initial matrix shifting
	// The source is the subprocess rank
	// Shift A
	if (row != 0) {
		source = (col + row) % p;
		destination = (col + p - row) % p;
		recv_tag = 0;
		send_tag = 0;
		MPI_Sendrecv_replace(subarrA, sizematA, MPI_INT, destination, send_tag, source, recv_tag, row_comm, &status);
	}

	// Shift B
	if (col != 0) {
		source = (row + col) % p;
		destination = (row + p - col) % p;
		recv_tag = 0;
		send_tag = 0;
		MPI_Sendrecv_replace(subarrB, sizematB, MPI_INT, destination, send_tag, source, recv_tag, col_comm, &status);
	}

	subarrC = (int *)calloc(subsizeA * subsizeB, sizeof(int));

	send_tag = 0;
	recv_tag = 0;

	for (int step = 0; step < p; step++) {
		int index = 0;

		// Matrix multiplication
		for (int i = 0; i < subsizeA; i++) {
			for (int j = 0; j < subsizeB; j++) {
				for (int k = 0; k < subsizeA; k++) {
					subarrC[index] += subarrA[i * subsizeA + k] * subarrB[k * subsizeB + j];
				}
				index++;
			}
		}

		// Move A to the left
		source = (col + 1) % p;
		destination = (col + p - 1) % p;
		MPI_Sendrecv_replace(subarrA, sizematA, MPI_INT, destination, send_tag, source, recv_tag, row_comm, &status);

		// Move B up
		source = (row + 1) % p;
		destination = (row + p - 1) % p;
		MPI_Sendrecv_replace(subarrB, sizematB, MPI_INT, destination, send_tag, source, recv_tag, col_comm, &status);
	}


	if (rank == 0)
	{
		arrC = (int *)malloc(sizeof(int) * sizeA * sizeB);
	}

	MPI_Barrier(communicator);

	MPI_Gather(subarrC, subsizeA * subsizeB, MPI_INT, arrC, subsizeA * subsizeB, MPI_INT, 0, communicator);

	if (rank == 0) {
		matC = (int **)malloc(sizeA * sizeof(int *));
		for (int i = 0; i < sizeA; i++)
		{
			matC[i] = (int *)malloc(sizeB * sizeof(int));
		}

		matConv(matC, arrC, p, subsizeA, 1);
		printf("Matrix A:\n", rank, sizeA, sizeA);
		printMatrix(matA, sizeA);
		printf("\n");

		printf("Matrix B:\n", rank, sizeB, sizeB);
		printMatrix(matB, sizeB);
		printf("\n");

		printf("Result:\n", rank, sizeA, sizeB);
		printMatrix(matC, sizeA);
	}
	MPI_Finalize();
	return 0;
}