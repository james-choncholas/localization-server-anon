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

std::pair<bool, int> BuildLM(Float threeDPts[], Float y0[], int numPts,
                            Float f, Float cx, Float cy, Float x[]);

std::pair<bool, int> lm_server(int party, NetIO *io, NetIO *ttpio);
