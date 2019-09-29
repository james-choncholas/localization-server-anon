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

#include <matmult.h>
#include <rodrigues.h>
#include <util.h>

using namespace emp;
using namespace std;

// Note: All 2D matrix inputs passed as single dimension array
// P=4xnumPoints [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
// x=6x1 [ rotation angles; translation] (column)
// K=3x3 camera intrinsic
// result=3xnumPoints preallocated, only first two rows are x,y.
//      last row needed for intermediate calculation (homogeneous)
//      [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
void BuildProjectPointsCircuit(Float _P[], Float _x[], Float _K[], Float _res[],
        int numPoints, bool skipOneGate=false) {
    Float one_gate = Float(1.0, PUBLIC);

    void *raw_memoryR = operator new[](12 * sizeof(Float));
    Float *R = static_cast<Float *>(raw_memoryR);

    BuildRodriguesCircuit(_x, R);

    // copy translation into last column of R
    R[3] = _x[3];
    R[7] = _x[4];
    R[11] = _x[5];

    //cout << "x:"<< endl;
    //printFloatVector(_x, 6);
    //cout <<"R:"<< endl;
    //printFloatVector(R, 12);
    //cout << endl;

    //res = K*R*P;
    void *raw_memoryscratch = operator new[](3*4 * sizeof(Float));
    Float *scratch = static_cast<Float *>(raw_memoryscratch);
    BuildMatmultCircuit(_K,3,3, R,3,4, scratch);
    BuildMatmultCircuit(scratch,3,4, _P,4,numPoints, _res);


    // homogeneous coords to x,y
    for(int i=0; i<numPoints; i++) {
        _res[i] = _res[i] / _res[2*numPoints+i];
        _res[numPoints+i] = _res[numPoints+i] / _res[2*numPoints+i];
        if (!skipOneGate) {
            _res[2*numPoints+i] = one_gate; // can be excluded for efficiency
        }
    }

    for (int i=0; i<12; i++) {
        R[i].~Float();
    }
    delete []R;
    for (int i=0; i<3*4; i++) {
        scratch[i].~Float();
    }
    delete []scratch;
}

// Note: All 2D matrix inputs passed as single dimension array
// P=4xnumPoints [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
// x=6x1 [ rotation angles; translation] (column)
// K=3x3 camera intrinsic
// result=3xnumPoints preallocated, only first two rows are x,y.
//      last row needed for intermediate calculation (homogeneous)
//      [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
uint32_t test_projectpoints_circuit(int party, NetIO *io,
        float* _P, float* _x, float* _K, float* _res, int numPoints) {

    setup_semi_honest(io, party);


    void *raw_memoryP = operator new[](4*numPoints * sizeof(Float));
    void *raw_memoryx = operator new[](6 * sizeof(Float));
    void *raw_memoryK = operator new[](9 * sizeof(Float));
    void *raw_memoryres = operator new[](3*numPoints * sizeof(Float));
    Float *P = static_cast<Float *>(raw_memoryP);
    Float *x = static_cast<Float *>(raw_memoryx);
    Float *K = static_cast<Float *>(raw_memoryK);
    Float *res = static_cast<Float *>(raw_memoryres);

    for (int i=0; i<3*numPoints; i++) {
        new(&P[i]) Float(_P[i], ALICE);
    }
    for (int i=3*numPoints; i<4*numPoints; i++) { // need contant 1s
        new(&P[i]) Float(1.0, PUBLIC);
    }
    for (int i=0; i<6; i++) {
        new(&x[i]) Float(_x[i], ALICE);
    }
    for (int i=0; i<9; i++) {
        new(&K[i]) Float(_K[i], ALICE);
    }

    CLOCK(Run);
    TIC(Run);
    BuildProjectPointsCircuit(P, x, K, res, numPoints);
    TOC(Run);


    cout << "projectpoints result:" << endl;
    for (int i=0; i<2*numPoints; ++i) {
        cout << res[i].reveal<double>() << ", ";
        if (i%numPoints == numPoints-1) cout << endl;
        res[i].~Float();
    }
    operator delete[](raw_memoryres);


    for (int i=0; i<3*numPoints; i++) { // no in gate for contant 1s
        P[i].~Float();
    }
    operator delete[](raw_memoryP);
    for (int i=0; i<6; i++) {
        x[i].~Float();
    }
    operator delete[](raw_memoryx);
    for (int i=0; i<9; i++) {
        K[i].~Float();
    }
    operator delete[](raw_memoryK);

    return 0;
}

