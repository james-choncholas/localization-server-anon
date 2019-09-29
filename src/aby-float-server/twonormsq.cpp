#include <stdio.h>
#include "jlog.h"
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


/*
 An implementation of SVD from Numerical Recipes in C and Mike Erhdmann's lectures
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

const uint32_t bitlen = 32;

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
share* BuildTwoNormSqCircuit(share *vect[], int sz, BooleanCircuit *c) {
    float zero=0.0;
    share *sum = c->PutCONSGate((uint32_t*)&zero, bitlen);
    for (int i=0; i<sz; i++) {
        share *temp = c->PutFPGate(vect[i], vect[i], MUL, bitlen, 1, no_status);
        share *oldsum = sum;
        sum = c->PutFPGate(sum, temp, ADD, bitlen, 1, no_status);
        delete temp;
        delete oldsum;
    }
    return sum;
}

float test_twonormsq_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float *vect, int sz) {

    uint32_t reservegates = 65536;
    const std::string& abycircdir = "../../extern/ABY/bin/circ";
    ABYParty* party = new ABYParty(role, address, port, seclvl, bitlen, nthreads, mt_alg, reservegates, abycircdir);
    std::vector<Sharing*>& sharings = party->GetSharings();
    Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

    share **s_vect = new share*[sz];

    for (int i=0; i<sz; ++i) {
        if (role == SERVER)
            s_vect[i] = circ->PutINGate((uint32_t*) &vect[i], 32, role);
        else
            s_vect[i] = circ->PutDummyINGate(32);
    }

    share *s_res = BuildTwoNormSqCircuit(s_vect, sz, (BooleanCircuit*) circ);

    share *s_out = circ->PutOUTGate(s_res, ALL);

    party->ExecCircuit();

    uint32_t* output;
    uint32_t out_bitlen, out_nvals;
    s_out->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
    cout << *(float*)output << endl;

    delete party;
    delete[] s_vect;
    delete s_out;
    return 0;
}
