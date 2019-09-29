#pragma once
#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

using namespace emp;

int BuildSvdCircuit(Float **a, int nRows, int nCols, Float *w, Float **v);

uint32_t test_sign_circuit(int party, NetIO *io, float a_in, float b_in);

uint32_t test_pythag_circuit(int party, NetIO *io, float a_in, float b_in);

uint32_t test_svd_circuit(int party, NetIO *io,
        float **a_in, int nRows, int nCols, float *w_in, float **v_in);
