#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>


#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

#include <svd.h>
#include <invert.h>
#include <util.h>

using namespace emp;
using namespace std;

const uint32_t bitlen = 32;

void BuildInvertCircuit(Float *in[], int m, int n, Float *res[]) {
    assert(m>=n);
    Float zero_gate = Float(0.0, PUBLIC);

    Float* w = static_cast<Float*>(operator new[](n * sizeof(Float)));
    for(int i=0; i<n; i++) {
        w[i] = Float(0.0, PUBLIC);
    }
    Float** v = new Float*[n];
    for(int i=0; i<n; i++) {
        v[i] = static_cast<Float*>(operator new[](n * sizeof(Float)));
        for(int j=0; j<n; j++) {
            v[i][j] = Float(0.0, PUBLIC);
        }
    }

    BuildSvdCircuit(in, m, n, w, v);

    Float** u = in;

    //cout << "w\n";
    //printFloatVector(w, m);

    //cout << "v\n";
    //printFloatMatrix(v, n, n);
    //cout << endl;

    //cout << "u\n";
    //printFloatMatrix(u, m, n);
    //cout << endl;


    // res = inv(in) = v*inv(w)*uT
    // inv(w)*uT
    for (int j=0;j<n;j++) {
        // if (w[j]) {
            Bit ifw = w[j].equal(zero_gate);

            for (int i=0;i<m;i++) {
                Float temp = u[i][j] / w[j];
                u[i][j] = temp.If(ifw, Float(0.0, PUBLIC));
            }
        //}
    }

    // v*(w*uT) (don't use matmult so we can do transpose ourselves)
    for (int j=0;j<n;j++) {
        for (int jj=0;jj<m;jj++) {
            res[j][jj] = Float(0.0, PUBLIC);
            for (int k=0; k<n; k++) {
                res[j][jj] = res[j][jj] + (v[j][k]*u[jj][k]); // note u indices do transpose
            }
        }
    }
}

uint32_t test_invert_circuit(int party, NetIO *io,
        float **a_in, int nRows, int nCols, float **res_in) {

    setup_semi_honest(io, party);

    // share arrays
    Float** a = new Float*[nRows];
    for(int i=0; i<nRows; i++) {
        a[i] = static_cast<Float*>(operator new[](nCols * sizeof(Float)));
        for(int j=0; j<nCols; j++) {
            new(&a[i][j]) Float(a_in[i][j], ALICE);
        }
    }


    // res is nxm not mxn
    Float** res = new Float*[nCols];
    for(int i=0; i<nCols; i++) {
        res[i] = static_cast<Float*>(operator new[](nRows * sizeof(Float)));
        //for(int j=0; j<nRows; j++) {
        //    res[i][j] = Float(0.0, PUBLIC);
        //}
    }

    CLOCK(BuildInvert);
    TIC(BuildInvert);
    BuildInvertCircuit(a, nRows, nCols, res);
    TOC(BuildInvert);

    for(int i=0; i<nCols; i++) {
        for (int j=0; j<nRows; j++) {
            res_in[i][j] = res[i][j].reveal<double>(PUBLIC);
            res[i][j].~Float();
        }
        delete[] res[i];
    }
    delete[] res;

    finalize_semi_honest();

    return 0;
}
