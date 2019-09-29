//#include <stdio.h>
//#include "jlog.h"
//#include <iostream>
//#include <string>

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

share* BuildTwoNormSqCircuit(share *vect[], int sz, BooleanCircuit *c);

float test_twonormsq_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float *vect, int sz);
