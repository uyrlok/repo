// hash matrix1 = 478897
// hash matrix2 = 699516
// hash matrix3 = 600491

#include "stdafx.h"
#include <fstream>
#include <string>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <mpi.h>

int length;

using namespace std;
typedef struct limits {
	int start;
	int stop;
} LIMITS;

int** open_file(string filename)
{
	ifstream fin;
	string temp;
	int j = 0,k=0;
	fin.open(filename);
	fin >> temp;
	length = temp.length();
	if (length > 0)
	{
		int **matrix;
		matrix = new int *[length];
		for (int i = 0; i < length; i++)
		{
			matrix[i] = new int[length];
		}
		
		for (int i = 0; i < length; i++)
		{
			for (int j = 0; j < length; j++)
			{
				if (temp[j] != NULL)
				{

					matrix[i][j] = temp[j] - '0';
				}
				else
				{
					matrix[i][j] = 0;
				}
			}
			fin >> temp;
			k++;
		}
		fin.close();
		return matrix;
	}
	else
	{
		return 0;
	}
}

int check_control_sum(int **matrix,int len)
{
	int mod = 1e6 + 7;
	string temp;
	int hash=0;
		for(int i=0;i<len;i++)
		for (int j = 0; j < len; j++)
		{
			temp.append(to_string(matrix[i][j]));
		}
	
	for (int i = 0; i < temp.size(); i++)
	hash = (hash + (temp[i] * (unsigned int)pow(31, i) % mod)) % mod;
	return hash;
	
}

int main(int argc, char **argv)
{
	
	string pathtofile1 = "matrix1.txt";
	string pathtofile2 = "matrix2.txt";
	int **matrix1;
	matrix1 = open_file(pathtofile1);
	int length = ::length;
	int **matrix2;
	matrix2 = open_file(pathtofile2);
	//check control sum
	int hash1 = check_control_sum(matrix1,length);
	int hash2 = check_control_sum(matrix2, length);
	//result matrix
	int **matrix3 = new int*[length];
	for (int i = 0; i < length; i++)
		matrix3[i] = new int[length];
	
	

	int rank;
	MPI_Init(&argc, &argv);
	int num_tasks, num_workers;
	MPI_Status stat;


	MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int d = length / 2;
	
	int start, finish;
	int threads = num_tasks;
	int* numToDo = (int*)malloc(threads * sizeof(int));
	int len = length;

	if (len % threads == 0) {
		for (int i = 0; i<threads; i++) *(numToDo + i) = len / threads;
	}
	else {
		for (int i = 0; i<threads; i++) *(numToDo + i) = len / threads;
		for (int i = 0; i<len % threads; i++) *(numToDo + i) = *(numToDo + i) + 1;
	}

	LIMITS* variableIndex = (LIMITS*)malloc(threads * sizeof(LIMITS));
	start = 0;
	for (int i = 0; i<threads; i++) {
		(variableIndex + i)->start = start;
		(variableIndex + i)->stop = start + *(numToDo + i);
		start = (variableIndex + i)->stop;
	}

	if (rank == 0)
	{
		double start_time = MPI_Wtime();
		for (int i = 1; i < threads; i++)
		{
			printf("Send matrix1 to process %i...", i);
			MPI_Send(matrix1, len*len, MPI_INT, i, 0, MPI_COMM_WORLD);
			printf("Complete\n");
		}

		MPI_Barrier(MPI_COMM_WORLD);

		start = (variableIndex + rank)->start;
		finish = (variableIndex + rank)->stop;
		int numRows = (finish - start);

		int *tmp = (int *)malloc(numRows * sizeof(int));
		for (int i = 0; i < numRows; i++)
			tmp[i] = 0;

		for (int i = start; i < finish; i++)
		{
			tmp[i] = matrix1[i][0] * matrix1[i][1];
			for (int j = 1; j < finish / 2; j++)
			{
				tmp[i] = tmp[i] + matrix1[i][2 * j] * matrix1[i][2 * j + 1];
			}
		}
		
		int * rowfactor = new int[length];

		for (int i = start; i < finish; i++)
		{
			rowfactor[i] = tmp[i];
		}
		MPI_Barrier(MPI_COMM_WORLD);

		for (int k = 1; k < threads; k++)
		{
			start = (variableIndex + k)->start;
			finish = (variableIndex + k)->stop;
			int numRows = (finish - start);

			int *tmp = (int *)malloc(numRows* sizeof(int));

			MPI_Recv(tmp, numRows, MPI_INT, k, 0, MPI_COMM_WORLD, &stat);
			for (int i = start; i < finish; i++)
			{
				rowfactor[i] = tmp[i - start];
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		for (int i = 1; i < threads; i++)
		{
			printf("Send matrix2 to process %i...", i);
			MPI_Send(matrix2, len*len, MPI_INT, i, 0, MPI_COMM_WORLD);
			printf("Complete\n");
		}

		MPI_Barrier(MPI_COMM_WORLD);

		for (int i = 0; i < numRows; i++)
			tmp[i] = 0;

		for (int i = start; i < finish; i++)
		{
			tmp[i] = matrix2[0][i] * matrix2[1][i];
			for (int j = 1; j < finish / 2; j++)
			{
				tmp[i] = tmp[i] + matrix2[2 * j][i] * matrix2[2 * j + 1][i];
			}
		}

		int * columnfactor = new int[length];
		for (int i = start; i < finish; i++)
		{
			columnfactor[i] = tmp[i];
		}

		MPI_Barrier(MPI_COMM_WORLD);

		for (int k = 1; k < threads; k++)
		{
			start = (variableIndex + k)->start;
			finish = (variableIndex + k)->stop;
			int numRows = (finish - start);

			MPI_Recv(tmp, numRows, MPI_INT, k, 0, MPI_COMM_WORLD, &stat);
			for (int i = start; i < finish; i++)
			{
				columnfactor[i] = tmp[i - start];
			}
		}
		free(tmp);

		MPI_Barrier(MPI_COMM_WORLD);

		double stop_time = MPI_Wtime();

		for (int i = 0; i < length; i++)
		{
			for (int j = 0; j < length; j++)
			{
				matrix3[i][j] = -rowfactor[i] - columnfactor[j];
				for (int k = 0; k < d; k++)
				{
					matrix3[i][j] = matrix3[i][j] + (matrix1[i][2 * k] + matrix2[2 * k + 1][j])*(matrix1[i][2 * k + 1] + matrix2[2 * k][j]);
				}
			}
		}

		//printf("%d\n",stop_time - start_time);
	}
	else
	{
		MPI_Recv(matrix1, len*len, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

		MPI_Barrier(MPI_COMM_WORLD);

		start = (variableIndex + rank)->start;
		finish = (variableIndex + rank)->stop;
		int numRows = (finish - start);

		int *tmp = (int *)malloc(len*(numRows) * sizeof(int));
		for (int i = 0; i < len*numRows; i++)
			tmp[i] = 0;

		for (int i = start; i < finish; i++)
		{
			tmp[i-start] = matrix1[i][0] * matrix1[i][1];
			for (int j = 1; j < finish / 2; j++)
			{
				tmp[i-start] = tmp[i-start] + matrix1[i][2 * j] * matrix1[i][2 * j + 1];
			}
		}
		MPI_Barrier(MPI_COMM_WORLD);

		printf("Worker %d sending rowFactor to Master\n", rank);
		MPI_Send(tmp, numRows, MPI_INT, 0, 0, MPI_COMM_WORLD);
		printf("Worker %d finished sending\n", rank);

		MPI_Barrier(MPI_COMM_WORLD);

		MPI_Recv(matrix2, len*len, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

		MPI_Barrier(MPI_COMM_WORLD);

		for (int i = 0; i < len*numRows; i++)
			tmp[i] = 0;

		for (int i = start; i < finish; i++)
		{
			tmp[i - start] = matrix2[0][i] * matrix2[1][i];
			for (int j = 1; j < finish / 2; j++)
			{
				tmp[i - start] = tmp[i - start] + matrix2[2*j][i] * matrix2[2*j+1][i];
			}
		}
		
		MPI_Barrier(MPI_COMM_WORLD);

		printf("Worker %d sending columnFactor to Master\n", rank);
		MPI_Send(tmp, numRows, MPI_INT, 0, 0, MPI_COMM_WORLD);
		printf("Worker %d finished sending\n", rank);
		free(tmp);

		MPI_Barrier(MPI_COMM_WORLD);

	}
	/* Алгоритм винограда для одного процесса (закомментить в конце MPI_FINALIZE();)
	int * rowfactor = new int[length];
	int * columnfactor = new int[length];
	for (int i = 0; i < length; i++)
	{
	rowfactor[i] = matrix1[i][0] * matrix1[i][1];
	for (int j = 1; j < d; j++)
	{
	rowfactor[i] = rowfactor[i] + matrix1[i][2 * j] * matrix1[i][2 * j + 1];
	}
	}
	
	//разделить выполнение на каждом
	for (int i = 0; i < length; i++)
	{
		columnfactor[i] = matrix2[0][i] * matrix2[1][i];
		for (int j = 1; j < d; j++)
		{
			columnfactor[i] = columnfactor[i] + matrix2[2 * j][i] * matrix2[2 * j+1][i];
		}
	}

	//разделить выполнение на каждом
	for (int i = 0; i < length; i++)
	{
		for (int j = 0; j < length; j++)
		{
			matrix3[i][j] = -rowfactor[i] - columnfactor[j];
			for (int k = 0; k < d; k++)
			{
				matrix3[i][j] = matrix3[i][j] + (matrix1[i][2 * k] + matrix2[2 * k+1][j])*(matrix1[i][2 * k+1] + matrix2[2 * k][j]);
			}
		}
	}*/
	/*
	for (int i = 0; i < length; i++)
		for (int j = 0; j < length; j++) {
			matrix3[i][j] = 0;
			for (int k = 0; k < length; k++)
				matrix3[i][j] += matrix1[i][k] * matrix2[k][j];
		}
		*/

	MPI_Finalize();
	int hash3 = check_control_sum(matrix3, length);

	system("pause");
	
	return 0;
}



