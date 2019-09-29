#include <stdio.h>
#include "jlog.h"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <math.h>

//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

using namespace emp;
using namespace std;

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
Float BuildTwoNormSqCircuit(Float vect[], int sz) {
    Float sum = Float(0.0, PUBLIC);
    for (int i=0; i<sz; i++) {
        sum = sum + (vect[i] * vect[i]);
    }
    return sum;
}

float test_twonormsq_circuit(int party, NetIO *io, float *vect_in, int sz) {
    setup_semi_honest(io, party);

    Float* vect = static_cast<Float*>(operator new[](sz * sizeof(Float)));
    for(int i=0; i<sz; i++) {
        vect[i] = Float(vect_in[i], PUBLIC);
    }

    Float res = BuildTwoNormSqCircuit(vect, sz);

    cout << "res " << res.reveal<double>() << endl;

    delete[] vect;

    finalize_semi_honest();

    return 0;
}
