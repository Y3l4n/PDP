#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#define m 5 // number of rows
#define n 5 // number of cols

#define U 100 // Upper value for top row values (C[0][x])
#define D 100 // Down value for bottom row values (C[m - 1][x])
#define L 0 // Left value for leftmost column values (C[x][0])
#define R 0 // Right value for rightmost column values (C[x][n - 1])




void Write2File(float *C, char name[]) // write matrix to file
{
    // char path[50] = "./results/Linear/";
    // strcat(path, name);

    FILE *result = fopen(name, "a");
    int i, j;

    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            fprintf(result, "%lf\t", *(C + i * n + j));
        }
        fprintf(result, "\n");
    }

    fclose(result);
}

void Initialize(float *C){ // initialize values of a matrix size m x n

    int i, j;
    for (i = 0; i < m; i++){ // iterating over rows
        for (j = 0; j < n; j++){ // iterating over columns
            if ((m / 2 - 20) <= i && i <= (m / 2 + 20) 
            && (n / 2 - 20) <= j && j <= (n / 2 + 20)){ // check whether index of element (i, j) belongs to the 40x40 pixel square in the middle (range [-20, 20])
                
                // 1D access of 2D array (see under)
                *(C + i * n + j) = 80.0;   
            } else {
                *(C + i * n + j) = 25.0;
            }
        }
    }
}

// Goal: initialize matrix n x n with values
// Update all values in matrix using red-black approach (checker board)
// Checker board has red and black blocks 
// Values corresponding to red blocks get redUpdate
// (values with sum of col. index and row index is even)
// Ex: C[0][0], C[0][2],...,C[1][1], C[1][3],...,C[2][0], C[2][2],...
// Values corresponding to black blocks get blackUpdate
// (values with sum of col.index and row index is odd)
// Ex: C[0][1]), C[0][3],...,C[1][0], C[1][3],...,C[2][1], C[2][3],...


// How each values are updated (for both red and black)
// After each iteration, each values will take
// the average of its neighbors (upper, down, left, right values (4 values))
// In Gauss-Seidel, values are updated after one another within single iteration
// Divide into red-black to avoid direct conflict

// Note: Accessing values stored in 1D array representation for 2D matrix
// 2D matrix: 
// C[0][0], C[0][1],....,C[0][n-1]
// C[1][0], C[1][1],....,C[1][n-1]
// ....
// C[m-1][0], C[m-1][1],.....C[m-1][n-1]
// 1D representation:
// C[0][0], C[0][1],....,C[0][n-1], C[1][0], C[1][1],....,C[1][n-1],...,C[m-1][0], C[m-1][1],.....C[m-1][n-1]

// Access C[i][j] (2D) = C[i * n + j] (1D)
// Access up, down, left, right for C[i][j] in 1D
// up: C[i - 1][j] = C[(i - 1) * n + j]
// down: C[i + 1][j] = C[(i + 1) * n + j]
// left: C[i][j - 1] = C[i * n + (j - 1)]
// right: C[i][j + 1] = C[i * n + (j + 1)]

// problem formulation
// https://www.youtube.com/watch?v=73GKju_OX0I

void redUpdate(float *C){
    int i, j; // i (row), j (column) index of values in matrix
    // values for updating edge values
    // u (up): upper value for top row (no more top values)
    // d (down): down value for bottom row (no more bottom values)
    // l (left): left value for leftmost column (no more left values)
    // r (right): right value for right column (no more right values)
    float u, d, l ,r;
    for (i = 0; i < n; i++){ // iterating over rows

        // iterating over columns:
        // for i even - i % 2 = 0, j starts from 0 with += 2 - j even
        // for i odd - i % 2 = 1, j start from 1 with += 2 - j odd
        // sum i and j always even
        for (j = i % 2; j < m; j += 2){

            if (i == 0){ // top row values
                u = U; // set default up U
                d = *(C + (i + 1) * n + j); // get down value

            } else if (i == m - 1){ // bottom row values
                u = *(C + (i - 1) * n + j); // get up value
                d = D; // set default down D

            } else {
                // get up, down
                u = *(C + (i - 1) * n + j);
                d = *(C + (i + 1) * n + j); 
            }
            
            if (j == 0){ // leftmost column values
                l = L; // set default left L
                r = *(C + i * n + j + 1); // get right

            } else if (j == n - 1){ // rightmost column values
                l = *(C + i * n + j - 1); // get left
                r = R; // set default right R
            
            } else {
                // get left, right
                l = *(C + i * n + j - 1);
                r = *(C + i * n + j + 1);
            }
            // Update C[i][j] = average of up down left right
            *(C + i * n + j) = (u + d + l + r) / 4;
        }
    }
}


// similar to redUpdate, but sum index = odd gets this
void blackUpdate(float *C){
    int i, j; 
    float u, d, l ,r;
    for (i = 0; i < n; i++){ // iterating over rows

        // iterating over columns:
        // for i even - (i + 1) % 2 = 1, j starts from 1 with += 2 - j odd
        // for i odd - (i + 1) % 2 = 0, j start from 0 with += 2 - j even
        // sum i and j always odd
        for (j = (i + 1) % 2; j < m; j += 2){

            // similar update to red
            if (i == 0){ // top row values
                u = U; // set default up U
                d = *(C + (i + 1) * n + j); // get down value

            } else if (i == m - 1){ // bottom row values
                u = *(C + (i - 1) * n + j); // get up value
                d = D; // set default down D

            } else {
                // get up, down
                u = *(C + (i - 1) * n + j);
                d = *(C + (i + 1) * n + j); 
            }
            
            if (j == 0){ // leftmost column values
                l = L; // set default left L
                r = *(C + i * n + j + 1); // get right

            } else if (j == n - 1){ // rightmost column values
                l = *(C + i * n + j - 1); // get left
                r = R; // set default right R
            
            } else {
                // get left, right
                l = *(C + i * n + j - 1);
                r = *(C + i * n + j + 1);
            }
            // Update C[i][j] = average of up down left right
            *(C + i * n + j) = (u + d + l + r) / 4;
        }
    }
}

int main(int argc, char** argv){
    float *C;
    C = (float *)malloc((m * n) * sizeof(float)); // allocate memory to matrix m x n
    Initialize(C);
    Write2File(C, "matran_test.txt");

    int i, j;
    float u, d, l, r;
    int k;
    for (k = 0; k < 5; k++){ // run 10000 iterations
        redUpdate(C);
        blackUpdate(C);
    }
    Write2File(C, "ketqua_linear_test.txt");
}