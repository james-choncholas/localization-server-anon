#include <jlog.h>
#include <privacyconf.h>
#include <stdio.h>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

#include <fixed_point_emp.h>

#include <lmlocalization.h>
#include <projectpoints.h>
#include <invert.h>
#include <svd.h>
#include <twonormsq.h>
#include <matmult.h>
#include <util.h>

using namespace emp;
using namespace std;

// Note: All 2D matrix inputs passed as single dimension array
// Note: the constant 1's can be ignored (but space must be allocated)
//
// 3D World Points (4xnumPts) which look like:
//  _             _
// |  x1 x2 x3     |
// |  y1 y2 y3 ... |
// |  z1 z2 z3     |
// |_ 1  1  1     _|
// must be pass as single dimension:
// [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
//
// 2D Image Points (3xnumPts) which look like:
//  _             _
// |  u1 u2 u3     |
// |  v1 v2 v3 ... |
// |_ 1  1  1     _|
// must be passed as single dimension:
// [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
//
// x (initial guess) : 6x1 [ rotation angles, translations ]
std::pair<bool, int> BuildLM(Float threeDPts[], Float y0[], int numPts,
                            Float f, Float cx, Float cy, Float x[]) {
    if (_verbosity & DBG_FLOW)
        cout << "Levenberg–Marquardt\n";

    if (_verbosity & DBG_ARGS) {
        cout << "P\n"; printFloatVector(threeDPts, 3*numPts);
        cout << "y0\n"; printFloatVector(y0, 2*numPts);
        cout << "f: " << f.reveal<double>() << endl;
        cout << "cx: " << cx.reveal<double>() << endl;
        cout << "cy: " << cy.reveal<double>() << endl;
    }

    // add constant ones to threeDPts
    Float one_gate = Float(1.0, PUBLIC);
    for (int i=0; i<numPts; i++) {
        threeDPts[3*numPts+i] = one_gate;
    }

    // Camera params (3x3)
    Float K[9] = {f, Float(0.0, PUBLIC), cx,
                   Float(0.0, PUBLIC), f, cy,
                   Float(0.0, PUBLIC), Float(0.0, PUBLIC), Float(1.0, PUBLIC)};


    float jacob_epsilon = JACOB_EPSILON; // this is needed, can't &JACOB_EPSILON directly
    Float jacob_epsilon_gate = Float(jacob_epsilon, PUBLIC);

    Float lambda = Float(LM_LAMBDA_INIT, PUBLIC);
    Float prevErrNorm = Float(std::numeric_limits<float>::max(), PUBLIC);
    Float lambdaBoundTop = Float(LM_LAMBDA_MAX, PUBLIC);
    Float lambdaBoundBottom = Float(LM_LAMBDA_MIN, PUBLIC);

    Float min_er = Float(MIN_ER, PUBLIC);
    int i;
    for (i=0; i<LM_MAX_ITR; i++) {
        if (_verbosity & DBG_FLOW)
            cout << RED << "\n\nLM Iteration " << i << RESET << endl;
        if (_verbosity & DBG_POSE_UPDATE) {
            cout << "x\n"; printFloatVector(x,6);
        }

        // project points using x guess
        //share **yHomog = new share*[3*numPts];
        Float *yHomog = static_cast<Float*>(operator new[](3*numPts * sizeof(Float)));
        BuildProjectPointsCircuit(threeDPts, x, K, yHomog, numPts, true);
        if (_verbosity & DBG_PROJECT) {
            cout << "projected y\n"; printFloatVector(yHomog, 2*numPts);
        }


        // throw away last "row" of result (constant ones),
        // interleave, and transpose y into 2nx1 vector
        // e.g. [x1; y1; x2; y2 ...]
        //share **y = new share*[2*numPts];
        Float *y = static_cast<Float*>(operator new[](2*numPts * sizeof(Float)));
        for(int p=0; p<numPts; ++p) {
            y[p*2] = yHomog[p];
            y[(p*2)+1] = yHomog[p+numPts];
        }
        delete []yHomog;
        if (_verbosity & DBG_PROJECT) {
            cout << "reshapen y\n"; printFloatVector(y, 2*numPts);
        }
        //cout << "y:\n";
        //printFloatVector(y, 2*numPts);

        // calculate jacobian (real 2D matrix not linearized)
        //  _                                            _
        // | du1/dr1 du1/dr2 du1/dr3 du1/t1 du1/t2 du1/t3 |
        // | dv1/dr1 dv1/dr2 dv1/dr3 dv1/t1 dv1/t2 dv1/t3 |
        // | du2/dr1 du2/dr2 du2/dr3 du2/t1 du2/t2 du2/t3 |
        // |                      .                       |
        // |                      .                       |
        // |_                     . (2n)                 _|
        Float **jacob = new Float*[numPts*2];
        for(int p=0; p<numPts*2; p++) {
            jacob[p] = static_cast<Float*>(operator new[](6 * sizeof(Float)));
            //for(int pp=0; pp<6; pp++) {
            //    jacob[p][pp] = Float(0.0, PUBLIC);
            //}
        }
        for (int j=0; j<6; j++) { // for each DOF
            //perturb x
            Float temp = x[j];
            x[j] = x[j] + jacob_epsilon_gate;

            // project with epsilon
            Float *ytempHomog = static_cast<Float*>(operator new[](3*numPts * sizeof(Float)));
            BuildProjectPointsCircuit(threeDPts, x, K, ytempHomog, numPts, true);

            // throw away last "row" of result,
            // reshape, and transpose y into 2nx1 vector
            // e.g. [x1; y1; x2; y2 ...]
            Float *ytemp = static_cast<Float*>(operator new[](2*numPts * sizeof(Float)));
            for (int p=0; p<numPts; ++p) {
                ytemp[p*2] = ytempHomog[p];
                ytemp[(p*2)+1] = ytempHomog[p+numPts];
            }
            delete []ytempHomog;

            // put into jacob
            for (int p=0; p<2*numPts; ++p) {
                jacob[p][j] = (ytemp[p] - y[p]) / jacob_epsilon_gate;
            }

            delete[] ytemp;

            // un-perturb x
            x[j] = temp;
        }
        if (_verbosity & DBG_JACOB) {
            cout << "jacob\n"; printFloatMatrix(jacob, 2*numPts, 6);
        }

        // calculate error
        //dy = y0 - y;
        Float *dy = static_cast<Float*>(operator new[](2*numPts * sizeof(Float)));
        for (int p=0; p<2*numPts; p++) {
            dy[p] = y0[p] - y[p];
        }
        if (_verbosity & DBG_ER) {
            cout << "dy\n"; printFloatVector(dy, 2*numPts);
        }
        delete[] y;

        // LM with fletcher improvement
        // inv(Jt*J + lambda*diag(Jt*J)) * Jt * dy
        Float **JtJ = new Float*[6];// 6x6
        for(int p=0; p<6; p++) {
            JtJ[p] = static_cast<Float*>(operator new[](6 * sizeof(Float)));
        }
        BuildMatmult2DwTransposeCircuit(jacob, numPts*2, 6, true,
                                        jacob, numPts*2, 6, false,
                                        JtJ); // must use 2D arrays, not 1D
        // add lambda * diag(Jt*J) in-place
        for (int p=0; p<6; p++) {
            // add fletcher column-wise to JtJ
            Float fletcher = lambda * JtJ[p][p];

            //for (int pp=0; pp<6; pp++) {
            //    JtJ[pp][p] = JtJ[pp][p] + fletcher;
            //}
            JtJ[p][p] = JtJ[p][p] + fletcher;
        }

        Float **JtJ_i = new Float*[6];// 6x6
        for(int p=0; p<6; p++) {
            JtJ_i[p] = static_cast<Float*>(operator new[](6 * sizeof(Float)));
        }
        BuildInvertCircuit(JtJ, 6, 6, JtJ_i);
        for(int p=0; p<6; p++) {
            delete[] JtJ[p];
        }
        delete[] JtJ;


        Float **JtJ_i_Jt = new Float*[6];// 6x2n
        for(int p=0; p<6; p++) {
            JtJ_i_Jt[p] = static_cast<Float*>(operator new[](numPts*2 * sizeof(Float)));
        }
        BuildMatmult2DwTransposeCircuit(JtJ_i, 6, 6, false,
                                        jacob, numPts*2, 6, true,
                                        JtJ_i_Jt);
        for(int p=0; p<numPts*2; p++)
            delete[] jacob[p];
        delete[] jacob;
        for(int p=0; p<6; p++)
            delete[] JtJ_i[p];
        delete[] JtJ_i;

        // linearize for matmult
        Float *JtJ_i_Jt_linear = static_cast<Float*>(operator new[](6*numPts*2 * sizeof(Float)));
        for(int p=0; p<6; p++) {
            for(int pp=0; pp<numPts*2; pp++) {
                JtJ_i_Jt_linear[(p*numPts*2)+pp] = JtJ_i_Jt[p][pp];
            }
        }
        Float *dx = static_cast<Float*>(operator new[](6 * sizeof(Float)));
        BuildMatmultCircuit(JtJ_i_Jt_linear, 6, numPts*2,
                            dy, numPts*2, 1, //column vector
                            dx);

        if (_verbosity & DBG_POSE_UPDATE) {
            cout << "dx\n"; printFloatVector(dx, 6);
        }

        delete[] dy;
        for(int p=0; p<6; p++)
            delete[] JtJ_i_Jt[p];
        delete[] JtJ_i_Jt;
        delete[] JtJ_i_Jt_linear;

        //  abs(norm(dy, cv::NORM_L2SQR)) < MIN_ER
        Float norm = BuildTwoNormSqCircuit(dx, 6);
        Float absnorm = norm.abs();
        if (_verbosity & DBG_ER)
            cout << "errNorm\n" << absnorm.reveal<double>() << endl;

        Float biggerLambda = lambda * Float(10, PUBLIC);
        Float smallerLambda = lambda / Float(10, PUBLIC);

        Bit errWentDown = absnorm.less_than(prevErrNorm);
        lambda = biggerLambda.If(errWentDown, smallerLambda);

        Bit lambdaTooBig = lambdaBoundTop.less_than(lambda);
        lambda = lambda.If(lambdaTooBig, lambdaBoundTop);
        Bit lambdaTooSmall = lambda.less_than(lambdaBoundBottom);
        lambda = lambda.If(lambdaTooSmall, lambdaBoundBottom);

        if (_verbosity & DBG_LAMBDA)
            cout << "lambda " << lambda.reveal<double>() << endl;

        prevErrNorm = absnorm;

        // error threshold
        Bit er_flag = absnorm.less_than(min_er);

        // update pose
        for (int p=0; p<6; p++) {
#if PPL_FLOW==PPL_FLOW_DO // set dx to zero if no error
            for (int bi=0; bi<dx[p].size(); bi++)
                dx[p][i] = dx[p][i] & er_flag;
#endif
            x[p] = x[p] + dx[p];
        }
        delete[] dx;

#if PPL_FLOW!=PPL_FLOW_DO
        // okay to reveal how many iterations it took
        if (er_flag.reveal<bool>()) {
            if (_verbosity & DBG_ER) {
                cout << "breaking because error " << absnorm.reveal<double>()
                    << " is smaller than " << MIN_ER << endl;
            }
            break;
        }
#endif
    }

    if (_verbosity & DBG_FLOW)
        cout << "Did " << i+1 << " LM iterations\n";
    return {i < LM_MAX_ITR, i+1};
}

std::pair<bool, int> test_lm_circuit(int party, NetIO *io,
        vector<cv::Point3f> threeDPts, vector<cv::Point2f> twoDPts,
        float f, float cx, float cy, float* x /* initial guess for { r1, r2, r3, t1, t2, t3 } */ ) {

    setup_semi_honest(io, party);

    int numPts = threeDPts.size();

    Float *s_threeDPts = static_cast<Float*>(operator new[](4*numPts * sizeof(Float)));
    Float *s_twoDPts = static_cast<Float*>(operator new[](3*numPts * sizeof(Float)));
    for(int i=0; i<numPts; i++) {
        // [x1, x2... ; y1, y2... ; z1, z2...]
        s_threeDPts[i] = Float(threeDPts[i].x, ALICE);
        s_threeDPts[numPts + i] = Float(threeDPts[i].y, ALICE);
        s_threeDPts[2*numPts + i] = Float(threeDPts[i].z, ALICE);
        //s_threeDPts[3*numPts + i] = Float(1.0, PUBLIC);

        // [x1, y1; x2, y2; ...]
        s_twoDPts[2*i] = Float(twoDPts[i].x, ALICE);
        s_twoDPts[2*i + 1] = Float(twoDPts[i].y, ALICE);
        //s_twoDPts[2*numPts + i] = Float(1.0, PUBLIC);
    }

    Float s_f  = Float(f, ALICE);
    Float s_cx = Float(cx, ALICE);
    Float s_cy = Float(cy, ALICE);

    Float *s_x = static_cast<Float*>(operator new[](6 * sizeof(Float)));
    for(int i=0; i<6; i++) {
        s_x[i] = Float(x[i], ALICE);
    }

    CLOCK(LM);
    TIC(LM);
    auto start = clock_start();
    auto res = BuildLM(s_threeDPts, s_twoDPts, numPts,
                            s_f, s_cx, s_cy, s_x);
    double interval = time_from(start);
    TOC(LM);

    for(int i=0; i<6; i++) {
        x[i] = s_x[i].reveal<double>();
        s_x[i].~Float();
    }
    delete[] s_x;
    delete[] s_threeDPts;
    delete[] s_twoDPts;

    uint64_t numand = CircuitExecution::circ_exec->num_and();
    cout << "number of and gates: " << numand << endl;
    cout << "garbling speed : " << numand / interval << " million gate per second\n";

    finalize_semi_honest();

    return res;
}

std::pair<bool, int> lm_server(int party, NetIO *io, NetIO *ttpio) {
    uint32_t numPts;
    ttpio->recv_data(&numPts, sizeof(numPts));
    MSG("Performing LM with %d points\n", numPts);

    setup_semi_honest(io, party, 1024*16, ttpio);
    MSG("setup done\n");

    Float *s_threeDPts = static_cast<Float*>(operator new[](4*numPts * sizeof(Float)));
    Float *s_twoDPts = static_cast<Float*>(operator new[](3*numPts * sizeof(Float)));
    for(uint32_t i=0; i<numPts; i++) {
        // [x1, x2... ; y1, y2... ; z1, z2...]
        s_threeDPts[i] = Float(0, TTP);
        s_threeDPts[numPts + i] = Float(0, TTP);
        s_threeDPts[2*numPts + i] = Float(0, TTP);
        //s_threeDPts[3*numPts + i] = Float(0, PUBLIC);

        // [x1, y1; x2, y2; ...]
        s_twoDPts[2*i] = Float(0, TTP);
        s_twoDPts[2*i + 1] = Float(0, TTP);
        //s_twoDPts[2*numPts + i] = Float(1.0, PUBLIC);
    }

    Float s_f  = Float(0, TTP);
    Float s_cx = Float(0, TTP);
    Float s_cy = Float(0, TTP);

    Float *s_x = static_cast<Float*>(operator new[](6 * sizeof(Float)));
    for(int i=0; i<6; i++) {
        s_x[i] = Float(0, TTP);
    }

    CLOCK(LM);
    TIC(LM);
    auto start = clock_start();
    auto res = BuildLM(s_threeDPts, s_twoDPts, numPts,
                            s_f, s_cx, s_cy, s_x);
    double interval = time_from(start);
    TOC(LM);

    for(int i=0; i<6; i++) {
        s_x[i].reveal<double>(TTP); // ignore output, it goes to ttp
        s_x[i].~Float();
    }
    delete[] s_x;
    delete[] s_threeDPts;
    delete[] s_twoDPts;

    uint64_t numand = CircuitExecution::circ_exec->num_and();
    cout << "number of and gates: " << numand << endl;
    cout << "garbling speed : " << numand / interval << " million gate per second\n";

    finalize_semi_honest();

    return res;
}
