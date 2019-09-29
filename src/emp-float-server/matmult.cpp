#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

using namespace emp;
using namespace std;

const uint32_t bitlen = 32;

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmultCircuit(Float M[], int m, int n,
                            Float N[], int mm, int nn,
                            Float res[]) {
    (void) mm;
    assert(n == mm);
    for (int i=0; i<m; i++) { //row
        for (int j=0; j<nn; j++) { //col
            //res[i*nn +j] = Float(0.0, ALICE);
            new(&res[i*nn +j]) Float(0.0, PUBLIC);
            for (int k=0; k<n; k++) {
                Float temp = M[i*n + k] * N[k*nn + j];
                res[i*nn +j] = res[i*nn +j] + temp;
            }
        }
    }
}

uint32_t test_matmult_circuit(int party, NetIO *io) {

    CLOCK(Setup);
    TIC(Setup);
    setup_semi_honest(io, party);
    TOC(Setup);

    int m=3, n=5, mm=5, nn=4;

    void *raw_memoryA = operator new[](m*n * sizeof(Float));
    void *raw_memoryB = operator new[](mm*nn * sizeof(Float));
    void *raw_memoryres = operator new[](m*nn * sizeof(Float));
    Float *A = static_cast<Float *>(raw_memoryA);
    Float *B = static_cast<Float *>(raw_memoryB);
    Float *res = static_cast<Float *>(raw_memoryres);

    for (int i=0; i<m*n; ++i) {
        new(&A[i]) Float(1.0, ALICE);
    }

    for (int i=0; i<mm*nn; ++i) {
        new(&B[i]) Float(2.0, ALICE);
    }

    CLOCK(Run);
    TIC(Run);
    BuildMatmultCircuit(A, m, n, B, mm, nn, res);
    TOC(Run);

    cout << "matmult result:" << endl;
    for (int i=0; i<m*nn; ++i) {
        cout << res[i].reveal<double>() << ", ";
        if (i%nn == nn-1) cout << endl;
        res[i].~Float();
    }
    delete []raw_memoryres;

    CLOCK(Finalize);
    TIC(Finalize);
    finalize_semi_honest();
    TOC(Finalize);

    for (int i=0; i<m*n; i++) {
        A[i].~Float();
    }
    operator delete[](raw_memoryA);
    for (int i=0; i<mm*nn; i++) {
        B[i].~Float();
    }
    operator delete[](raw_memoryB);
    return 0;
}

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmult2DwTransposeCircuit(Float **M, int m, int n, const bool transposeM,
                            Float **N, int mm, int nn, const bool transposeN,
                            Float **res) {
    if (transposeM) std::swap(m, n);
    if (transposeN) std::swap(mm, nn);
    assert(n == mm);

    for (int i=0; i<m; i++) { //row
        for (int j=0; j<nn; j++) { //col
            new(&res[i][j]) Float(0.0, PUBLIC);
            for (int k=0; k<n; k++) {
                Float temp = Float(0.0, PUBLIC);
                if (!transposeM && !transposeN)
                    temp = M[i][k] * N[k][j];
                else if (!transposeM && transposeN)
                    temp = M[i][k] * N[j][k];
                else if (transposeM && !transposeN)
                    temp = M[k][i] * N[k][j];
                else if (transposeM && transposeN)
                    temp = M[k][i] * N[j][k];
                res[i][j] = res[i][j] + temp;
            }
        }
    }
}

uint32_t test_matmult2dwtranspose_circuit(int party, NetIO *io) {

    CLOCK(Setup);
    TIC(Setup);
    setup_semi_honest(io, party);
    TOC(Setup);

    int m=3, n=5, mm=4, nn=5;

    Float **A = new Float*[m];
    for(int p=0; p<m; p++) {
        A[p] = static_cast<Float*>(operator new[](n * sizeof(Float)));
        for(int pp=0; pp<n; pp++) {
            new(&A[p][pp]) Float(1.0, ALICE);
        }
    }
    Float **B = new Float*[mm];
    for(int p=0; p<mm; p++) {
        B[p] = static_cast<Float*>(operator new[](nn * sizeof(Float)));
        for(int pp=0; pp<nn; pp++) {
            new(&B[p][pp]) Float(2.0, ALICE);
        }
    }
    Float **res = new Float*[m];
    for(int p=0; p<m; p++) {
        res[p] = static_cast<Float*>(operator new[](mm * sizeof(Float)));
    }

    CLOCK(Run);
    TIC(Run);
    BuildMatmult2DwTransposeCircuit(A, m, n, false, B, mm, nn, true, res);
    TOC(Run);

    cout << "matmult result:" << endl;
    for (int p=0; p<m; ++p) {
        for (int pp=0; pp<mm; ++pp) {
            cout << res[p][pp].reveal<double>() << ", ";
            res[p][pp].~Float();
        }
        cout << endl;
    }
    delete []res;

    CLOCK(Finalize);
    TIC(Finalize);
    finalize_semi_honest();
    TOC(Finalize);

    for(int p=0; p<m; p++) {
        for(int pp=0; pp<n; pp++) {
            A[p][pp].~Float();
        }
        delete[] A[p];
    }
    for(int p=0; p<mm; p++) {
        for(int pp=0; pp<nn; pp++) {
            B[p][pp].~Float();
        }
        delete[] B[p];
    }
    return 0;
}
