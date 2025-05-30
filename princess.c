#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>

#define M 256
#define N 256

#define U 100
#define D 100
#define L 0
#define R 0

#define Delta 0.001

void Initialize(float *C);
void Write2File(float *C, char name[]);
void redUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size);
void blackUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size);


int main(int argc, char *argv[]){
    int rank, size;

    // initialize mpi with -np processors, assign ranks to processors and number of ranks
    MPI_Init(&argc, &argv);
    // rank start from 0, for example 3 processors, ranks: (0, 1, 2)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // rank = rank of each processor
    MPI_Comm_size(MPI_COMM_WORLD, &size); // size = number of processors

    int m = M / size; // Divide matrix along rows (horizontal slice) across processors
    // for example, matrix of 100 x 100, 5 processors
    // each processor will have a 20 x 100 matrix
    // to do: try changing it to vertical 

    float *C_local, *C;
    // Alloc memory to matrix and sub-matrices
    C = (float *) malloc((M * N) * sizeof(float)); // main matrix
    // sub-matrix at each processors (size m x N)
	C_local = (float *) malloc((m*N) * sizeof(float));

    // processor 1 will initialize matrix value
    if (rank == 1){
        Initialize(C);
    }
    
	// Ma trận to: M x N, ma trận con (chia cho các processor): m x N
	// m = M / số processor
    // Distribute sub-matrices from root processor to all processors
    // params: 
    // MPI_Scatter(
    // void* send_data, // data in root process - C: main matrix
    // int send_count, // how many values will be sent to each process - m * N values for each sub-matrix
    // MPI_Datatype send_datatype, // data type of the sent values - float (values in matrix)
    // void* recv_data, // buffer of data (place to hold the send_data) - C_local: local sub-matrix in each process
    // int recv_count, // how many values the recv_data receives - m * N values
    // MPI_Datatype recv_datatype, // what data type is the received data - float (values in matrix)
    // int root, // root process that is distributing data - 1: process 1 send 
    // MPI_Comm communicator) // communicator that the processes are in // WORLD
    MPI_Scatter(C, m*N, MPI_FLOAT, C_local, m*N, MPI_FLOAT, 1, MPI_COMM_WORLD);

    // Up and down boundary
    // Horizontal slice
    // First processor (rank = 0) receives first M / size rows. 
    // It has the 1st row, which doesn't have an up value
    // Similar to last processor (rank = size - 1) receives last M / size rows
    // It has the last row, which doesn't have a down value

    int i;
    // Initialize matrix of size (1 x N) for default Up and Down values for top and bottom rows
	float *Up, *Down;
	Up = (float *) malloc(N * sizeof(float));
	Down = (float *) malloc(N * sizeof(float));

	// Initialize up and down boundaries for first and last processor
	if (rank == 0) {
		for (i = 0; i < N; i++){
			Up[i] = U;
		}
	} else if (rank == size - 1){
		for (i = 0; i < N; i ++){
			Down[i] = D;
		}
	}
    

    // Save results in progress variables
	int k = 1;
  	char *iter_name;
  	// iter_name = (char *)malloc(20*sizeof(char));

    	
	// Initialize values for local and global delta
	float delta, delta_glob;



    // How the communication works:
    // For updating the local sub-matrices
    // All processors need "ghost" Up and Down matrix (1 x N)
    // For updating "edge" rows locally

    // When communicating, for processor 0 (top submatrix)
    // It only haves the local "Up" matrix (initialized Up for rank = 0)
    // for updating its top row locally
    // Since there are no higher matrices
    // It doesn't receive any rows into its "Up" matrix 
    // From other processors

    // For processor size - 1 (bottom submatrix)
    // It only haves the local "Down" matrix (initialized Down for rank = size - 1)
    // for updating its bottom row locally
    // Since there are no lower matrices
    // It doesn't receive any rows into its "Down" matrix
    // from other processors


    // For all middle processors
    // It sends the first row to prev processor (Send C_local - index of first row)
    // To act as new "Down" row for prev processor 
    // (Recv Down - receive C_local - index of first row in this processor)
    // (as "Down" row - index of "ghost" Down row in prev processor)
    // It sends the last row to next processor (Send C_local + (m-1)*N - index of last row)
    // To act as new "Up" row for next processor
    // (Recv Up - receive C_local + (m-1)*N - index of last row in this processor)
    // (as "Up" row - index of "ghost" Up row in next processor)


    // MPI_Send(
    //     void* data, // pointer to the first value in data buffer
    //     int count, // number of values
    //     MPI_Datatype datatype, // value data type
    //     int destination, // source
    //     int tag, // send down (1), send up (2)
            // tag helps MPI ensures the sender / receiver calls the correc type
    //     MPI_Comm communicator,)

    // MPI_Recv(
    //     void* data, // pointer to the first value in data buffer
    //     int count, // number of values
    //     MPI_Datatype datatype, // value data type
    //     int source, // source
    //     int tag, // receive up (1), receive down (2)
            // tag helps MPI ensures the sender / receiver calls the correc type
    //     MPI_Comm communicator,
    //     MPI_Status* status)


    do {
        i += 1;
        // initialize delta, refresh delta after each iteration
        // for fresh local maximum change
        delta = 0; 

		// Hàng đầu tiên của process dưới là biên dưới của process trên
		// Hàng cuối cùng của process trên là biên trên của process dưới

        //Communication 
		if (rank == 0){

			// Process trên cùng
			// Gửi hàng cuối cùng thânh biên trên của process 1
			// Nhận hàng đầu tiên của process 1 thành biên dưới của nó

			//Send down to next processor
			MPI_Send(C_local + (m-1)*N, N, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD);
			
			//Receive Down from next processor
			MPI_Recv(Down, N, MPI_FLOAT, rank + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);	
		} else if (rank == size - 1) {

			// Process dưới cùng
			// Gửi hàng đầu tiên thành biên dưới của process dưới cùng - 1
			// Nhận hàng cuối cùng của process dưới cùng - 1 thành biên trên của nó
			//Send Up to previous processor
			MPI_Send(C_local, N, MPI_FLOAT, rank - 1, 2, MPI_COMM_WORLD);
			
			//Receive Up from previous processor
			MPI_Recv(Up, N, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		
		} else {
			//Send Up to previous processor
			MPI_Send(C_local, N, MPI_FLOAT, rank - 1, 2, MPI_COMM_WORLD);
			//Send down to next processor
			MPI_Send(C_local + (m-1)*N, N, MPI_FLOAT, rank + 1, 1, MPI_COMM_WORLD);
			
			//Receive Up from previous processor
			MPI_Recv(Up, N, MPI_FLOAT, rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//Receive Down from next processor
			MPI_Recv(Down, N, MPI_FLOAT, rank + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		

        // Red black update
        redUpdate(C_local, Up, Down, &delta, m, N, rank, size);
        blackUpdate(C_local, Up, Down, &delta, m, N, rank, size);

        // All reduce: check stopping condition
        // Take all local deltas and set global delta based on
        // Maximum local delta
        MPI_Allreduce(&delta, &delta_glob, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
        
        //save intermidiate states every 100 iterrations
	    if (k % 100 == 0) {
	    	MPI_Gather(C_local, m*N, MPI_FLOAT, C, m*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
	     	
	     	if (rank == 0) { // wirte iteration time as file.txt
			 	iter_name = (char *)malloc(50*sizeof(char));
			 	sprintf(iter_name, "%05d", k);
			 	strcat(iter_name, ".txt");

			 	Write2File(C, iter_name);
	     	}
	    } 
	    k += 1;
        
    } while (delta_glob > Delta);
    printf("Delta %f \n", delta);

    //Gather heat grid to processor 0
	MPI_Gather(C_local, m*N, MPI_FLOAT, C, m*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
    
    if (rank == 0){ // Write final iteration (sufficient tolerance)
		//save final result
		iter_name = (char *)malloc(50*sizeof(char));
		sprintf(iter_name, "%05d", k);
		strcat(iter_name, ".txt");
		Write2File(C, iter_name);
	}

    MPI_Finalize();
}


// redUpdate and blackUpdate now are kind of similar
// both of them will be responsible for keeping the checkerboard
// because of alignment, both of them can "shift" red and black
// keeping the pattern
// the local update for values in each submatrix in each process
// is similar to the linear method

// why need align

// for sub-matrices with even heights (h % 2 == 0)
// the last row will be brbrbrbr
// the start row:       rbrbrbrb (match red-black update)

// however sub-matrices with odd heights (h % 2 == 1)
// start row (even processor matrix): rbrbrbrb - submatrix 0 proc 0
// last row (even processor matrix):  rbrbrbrb
// start row (odd processor matrix):  rbrbrbrb - submatrix 1 proc 1
// invalid
// align: for odd processor matrix, add in red_align (rank % 2 == 1)
// so that the supposedly red cells will be black cells (shift right 1)

// even height: for both odd and even rank processors
// redUpdate and blackUpdate
// odd height: for odd processors:
// redUpdate becomes blackUpdate, blackUpdate becomes redUpdate
// (align)


void redUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size) {
		int red_align, i, j;
		if (h % 2 == 0) {
			red_align = 0;
		} else {
			red_align = id %2;
		}
		
		//neighboring points
		float u, d, l, r;
		
		//changing of value
		float c;
		
		for (i = 0; i < h; i ++) {
			for (j = (i + red_align) % 2; j < w; j += 2) {
				//check boundary condition dim x
				if (i == 0) {
					u = Up[j];
					d = C_local[(i+1)*w + j];
				} else if (i == h - 1) {
					u = C_local[(i-1)*w +j];
					d = Down[j];
				} else {
					u = C_local[(i-1)*w +j];
					d = C_local[(i+1)*w + j];
				}
				
				//check boundary condition dim y
				if (j == 0) {
					r = C_local[i*w + j + 1];
					l = L;
				} else if (j == w - 1) {
					r = R;
					l = C_local[i*w + j - 1];
				} else {
					l = C_local[i*w + j - 1];
					r = C_local[i*w + j + 1];
				}
				
                // measure how much a cell change in
                // a single iteration
                // because delta is reset after each iteration
                // every process after red-black update will
                // contribute local delta to Allreduce
                // to calculate global delta
                // to check for tolerance rate

				c = C_local[i*w + j] - (u + d + l + r)/4;
				if (c > *delta) {
					*delta = c;
				}
				
				//Update center value
				C_local[i*w + j] = (u + d + l + r)/4;
			}
		}
}

void blackUpdate(float *C_local, float *Up, float *Down, float *delta, int h, int w, int id, int size) {
		int black_align, i, j;
		if (h % 2 == 0) {
			black_align = 0;
		} else {
			black_align = id %2;
		}
		
		//neighboring points
		float u, d, l, r;
		
		//changing of value
		float c;
		
		for (i = 0; i < h; i ++) {
			for (j = (i + black_align + 1) % 2; j < w; j += 2) {
				//check boundary condition vertically
				if (i == 0) {
					u = Up[j];
					d = C_local[(i+1)*w + j];
				} else if (i == h - 1) {
					u = C_local[(i-1)*w +j];
					d = Down[j];
				} else {
					u = C_local[(i-1)*w +j];
					d = C_local[(i+1)*w + j];
				}
				
				//check boundary condition horizontally
				if (j == 0) {
					r = C_local[i*w + j + 1];
					l = L;
				} else if (j == w-1) {
					r = R;
					l = C_local[i*w + j - 1];
				} else {
					l = C_local[i*w + j - 1];
					r = C_local[i*w + j + 1];
				}
								
				c = C_local[i*w + j] - (u + d + l + r)/4;
				if (c > *delta) {
					*delta = c;
				}

				//Update center value
				C_local[i*w + j] = (u + d + l + r)/4;
			}
		}		
}


void Write2File(float *C, char name[]) // write matrix to file
{
    // char path[50] = "./results/Linear/";
    // strcat(path, name);

    FILE *result = fopen(name, "w");
    int i, j;

    for (i = 0; i < M; i++)
    {
        for (j = 0; j < N; j++)
        {
            fprintf(result, "%lf\t", *(C + i * N + j));
        }
        fprintf(result, "\n");
    }

    fclose(result);
}

void Initialize(float *C){ // initialize values of a matrix size M x n

    int i, j;
    for (i = 0; i < M; i++){ // iterating over rows
        for (j = 0; j < N; j++){ // iterating over columns
            if ((M / 2 - 20) <= i && i <= (M / 2 + 20) 
            && (N / 2 - 20) <= j && j <= (N / 2 + 20)){ // check whether index of element (i, j) belongs to the 40x40 pixel square in the middle (range [-20, 20])
                
                // 1D access of 2D array (see under)
                *(C + i * N + j) = 80.0;   
            } else {
                *(C + i * N + j) = 25.0;
            }
        }
    }
}