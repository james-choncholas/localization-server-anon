#include <iostream>
#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

using namespace emp;
using namespace std;

std::pair<bool, int> BuildLM(Float threeDPts[], int numThreeD,
                            Float twoDPts[], int numTwoD,
                            Float *f, Float *cx, Float *cy,
                            Float *x[]);

std::pair<bool, int> test_lm_circuit(int party, NetIO *io,
        vector<cv::Point3f> threeDPts, vector<cv::Point2f> twoDPts,
        float f, float cx, float cy, float* x /* initial guess for { r1, r2, r3, t1, t2, t3 } */ );

std::pair<bool, int> lm_server(int party, NetIO *io, NetIO *ttpio);
