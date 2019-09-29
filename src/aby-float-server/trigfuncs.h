#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

using namespace std;

// r=3x1, R=3x4 (only 3x3 is used here)
// values are secret, sizes are public
share* BuildSinCircuit(share *theta, BooleanCircuit *c);
share* BuildCosCircuit(share *theta, BooleanCircuit *c);

uint32_t test_trig_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing, bool trueForSin);
