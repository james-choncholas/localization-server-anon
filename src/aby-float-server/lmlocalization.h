#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <abycore/sharing/sharing.h>
#include <abycore/circuit/booleancircuits.h>
#include <abycore/circuit/arithmeticcircuits.h>
#include <abycore/circuit/circuit.h>
#include <abycore/aby/abyparty.h>

using namespace std;

void BuildGaussNewtonCircuit(share *threeDPts[], int numThreeD,
                            share *twoDPts[], int numTwoD,
                            share *f, share *cx, share *cy,
                            share *x[], BooleanCircuit *c);

uint32_t test_lm_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        vector<cv::Point3f> threeDPts, vector<cv::Point2f> twoDPts,
        float f, float cx, float cy, float* x /* initial guess for { r1, r2, r3, t1, t2, t3 } */ );
