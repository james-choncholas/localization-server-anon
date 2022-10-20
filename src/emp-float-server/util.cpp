#include <vector>
#include <stdio.h>
#include <iostream>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

#include "util.h"
#include <jlog.h>

using namespace emp;
using namespace std;

#define BUILD_TIMING 0
#define EXEC_TIMING 0

void print_float32_bits(Float a) {
    for(int i = 31; i >= 0; i--)
        printf("%d", a[i].reveal<bool>());
    cout << endl;
}

// prints an arbitrary size vector to the standard output
void printFloatVector(Float *v, int size) {
    int i;

    for (i = 0; i < size; i++) {
        printf("%.8lf ", v[i].reveal<double>(PUBLIC));
    }
    printf("\n\n");
}

// prints an arbitrary size matrix to the standard output
void printFloatMatrix(Float **a, int rows, int cols) {
    int i, j;

    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++) {
            printf("%.8lf ", a[i][j].reveal<double>(PUBLIC));
        }
        printf("\n");
    }
    printf("\n");
}

