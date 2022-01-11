#include "mercury.h"
#include "matrix.h"

void M_SetCamera(CloseData *close, Matrix *world, Matrix *skew) {

    Matrix     *camera;
    float       cx, cy, cz;

    camera = &(close->fcamskewmatrix);

    close->fcamx = (world)->mat[3][0];
    close->fcamy = (world)->mat[3][1];
    close->fcamz = (world)->mat[3][2];

    cx = -(close->fcamx * (world)->mat[0][0])
         -(close->fcamy * (world)->mat[0][1])
         -(close->fcamz * (world)->mat[0][2]);

    cy = -(close->fcamx * (world)->mat[1][0])
         -(close->fcamy * (world)->mat[1][1])
         -(close->fcamz * (world)->mat[1][2]);

    cz = -(close->fcamx * (world)->mat[2][0])
         -(close->fcamy * (world)->mat[2][1])
         -(close->fcamz * (world)->mat[2][2]);



    (camera)->mat[0][0] = (world)->mat[0][0] * (skew)->mat[0][0]+
                          (world)->mat[1][0] * (skew)->mat[1][0]+
                          (world)->mat[2][0] * (skew)->mat[2][0];

    (camera)->mat[0][1] = (world)->mat[0][0] * (skew)->mat[0][1]+
                          (world)->mat[1][0] * (skew)->mat[1][1]+
                          (world)->mat[2][0] * (skew)->mat[2][1];

    (camera)->mat[0][2] = (world)->mat[0][0] * (skew)->mat[0][2]+
                          (world)->mat[1][0] * (skew)->mat[1][2]+
                          (world)->mat[2][0] * (skew)->mat[2][2];


    (camera)->mat[1][0] = (world)->mat[0][1] * (skew)->mat[0][0]+
                          (world)->mat[1][1] * (skew)->mat[1][0]+
                          (world)->mat[2][1] * (skew)->mat[2][0];

    (camera)->mat[1][1] = (world)->mat[0][1] * (skew)->mat[0][1]+
                          (world)->mat[1][1] * (skew)->mat[1][1]+
                          (world)->mat[2][1] * (skew)->mat[2][1];

    (camera)->mat[1][2] = (world)->mat[0][1] * (skew)->mat[0][2]+
                          (world)->mat[1][1] * (skew)->mat[1][2]+
                          (world)->mat[2][1] * (skew)->mat[2][2];


    (camera)->mat[2][0] = (world)->mat[0][2] * (skew)->mat[0][0]+
                          (world)->mat[1][2] * (skew)->mat[1][0]+
                          (world)->mat[2][2] * (skew)->mat[2][0];

    (camera)->mat[2][1] = (world)->mat[0][2] * (skew)->mat[0][1]+
                          (world)->mat[1][2] * (skew)->mat[1][1]+
                          (world)->mat[2][2] * (skew)->mat[2][1];

    (camera)->mat[2][2] = (world)->mat[0][2] * (skew)->mat[0][2]+
                          (world)->mat[1][2] * (skew)->mat[1][2]+
                          (world)->mat[2][2] * (skew)->mat[2][2];

    (camera)->mat[3][0] = cx * (skew)->mat[0][0]+
                          cy * (skew)->mat[1][0]+
                          cz * (skew)->mat[2][0];

    (camera)->mat[3][1] = cx * (skew)->mat[0][1]+
                          cy * (skew)->mat[1][1]+
                          cz * (skew)->mat[2][1];

    (camera)->mat[3][2] = cx * (skew)->mat[0][2]+
                          cy * (skew)->mat[1][2]+
                          cz * (skew)->mat[2][2];

}
