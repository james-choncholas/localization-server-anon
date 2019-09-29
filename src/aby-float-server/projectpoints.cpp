#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

#include <matmult.h>
#include <rodrigues.h>

using namespace std;

// Note: All 2D matrix inputs passed as single dimension array
// P=4xnumPoints [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
// x=6x1 [ rotation angles; translation] (column)
// K=3x3 camera intrinsic
// result=3xnumPoints preallocated, only first two rows are x,y.
//      last row needed for intermediate calculation (homogeneous)
//      [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
void BuildProjectPointsCircuit(share *_P[], share *_x[], share *_K[], share *_res[],
        int numPoints, BooleanCircuit *c, bool skipOneGate=false) {

    uint32_t bitlen = 32;

    float one = 1;
    share* one_gate = c->PutCONSGate((uint32_t*)&one, bitlen);

    share *_R[12];

    BuildRodriguesCircuit(_x, _R, (BooleanCircuit*) c);

    // copy translation into last column of R
    _R[3] = _x[3];
    _R[7] = _x[4];
    _R[11] = _x[5];

    //res = K*R*P;
    share* scratch[3*4];
    BuildMatmultCircuit(_K,3,3, _R,3,4, scratch, c);
    BuildMatmultCircuit(scratch,3,4, _P,4,numPoints, _res, c);


    // homogeneous coords to x,y
    for(int i=0; i<numPoints; i++) {
        _res[i] = c->PutFPGate(_res[i], _res[2*numPoints+i], DIV, bitlen, 1, no_status);
        _res[numPoints+i] = c->PutFPGate(_res[numPoints+i], _res[2*numPoints+i], DIV, bitlen, 1, no_status);
        if (!skipOneGate) {
            _res[2*numPoints+i] = one_gate; // can be excluded for efficiency
        }
    }

    for (int i=0; i<12; i++) {
        if (i%4 != 3) delete _R[i]; // skip 3,7,11 (don't delete _x's)
    }
}

// Note: All 2D matrix inputs passed as single dimension array
// P=4xnumPoints [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
// x=6x1 [ rotation angles; translation] (column)
// K=3x3 camera intrinsic
// result=3xnumPoints preallocated, only first two rows are x,y.
//      last row needed for intermediate calculation (homogeneous)
//      [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
uint32_t test_projectpoints_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float* _P, float* _x, float* _K, float* _res, int numPoints) {

    // defines the operation/output bitlength operations for boolean and arithmetic circuits
    // this is why the output bitlen is 32 bits and not 8 bits like the input length
    uint32_t bitlen = 32;

    /**
        Step 1: Create the ABY Party object which defines the basis of
                all the operations which are happening. Operations performed
                are on the basis of the role played by this object.
    */

    uint32_t reservegates = 65536;
    const std::string& abycircdir = "../../extern/ABY/bin/circ";
    ABYParty* party = new ABYParty(role, address, port, seclvl, bitlen, nthreads, mt_alg, reservegates, abycircdir);

    /**
        Step 2: Get to know all the sharings available in the program.
    */
    std::vector<Sharing*>& sharings = party->GetSharings();

    /**
        Step 3: Create the circuit object on the basis of the sharing type
                being inputted.
    */
    Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

    /**
        Step 4: Create the share objects s_x1, s_y1, s_x2, s_y2
        that are the inputs to the circuit. s_out will store the output.
    */
    share *s_P[4*numPoints];
    share *s_x[6];
    share *s_K[9];

    float one = 1;
    share* one_gate = circ->PutCONSGate((uint32_t*)&one, bitlen);

    /**
        Step 5: Initialize plaintext values. (passed in to func - not needed here)
    */

    /**
        Step 6: Set inputs of the circuit for the respective party.
        The other party's input length must be specified.
    */
    if (role == SERVER) {
        for (int i=0; i<3*numPoints; i++) {
            s_P[i] = circ->PutINGate((uint32_t*) &_P[i], 32, role);
        }
        // in gate for contant 1s
        for (int i=3*numPoints; i<4*numPoints; i++) {
            s_P[i] = circ->PutCONSGate((uint32_t*)&one, bitlen);
        }
        for (int i=0; i<6; i++) {
            s_x[i] = circ->PutINGate((uint32_t*) &_x[i], 32, role);
        }
        for (int i=0; i<9; i++) {
            s_K[i] = circ->PutINGate((uint32_t*) &_K[i], 32, role);
        }
    } else {
        for (int i=0; i<3*numPoints; i++) {
            s_P[i] = circ->PutDummyINGate(32);
        }
        // in gate for contant 1s
        for (int i=3*numPoints; i<4*numPoints; i++) {
            s_P[i] = circ->PutCONSGate((uint32_t*)&one, bitlen);
        }
        for (int i=0; i<6; i++) {
            s_x[i] = circ->PutDummyINGate(32);
        }
        for (int i=0; i<9; i++) {
            s_K[i] = circ->PutDummyINGate(32);
        }
    }


    /**
        Step 7: Call the build method for building the circuit for the
                problem by passing the shared objects and circuit object.
                Don't Forget to type cast the circuit object to type of share
    */
    share *s_res[3*numPoints]; // dont forget extra space for homog constant 1's
    share *res[3*numPoints]; // stores plaintext
    BuildProjectPointsCircuit(s_P, s_x, s_K, s_res, numPoints, (BooleanCircuit*) circ);

    /**
        Step 8: Write the circuit output for both parties.
    */
    for (int i=0; i<2*numPoints; ++i) {
        res[i] = circ->PutOUTGate(s_res[i], ALL);
    }

    /**
        Step 9: Execute the circuit using the ABYParty object
    */
    CLOCK(ExecCircuit);
    TIC(ExecCircuit);
    party->ExecCircuit();
    TOC(ExecCircuit);

    /**
        Step 10: Obtain the output from
    */
    for (int i=0; i<2*numPoints; ++i) {
        assert(res[i] != NULL);

        uint32_t* output;
        uint32_t out_bitlen, out_nvals;

        res[i]->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
        //_res[i] = *(float*)output;
        cout << *(float*)output << " ";
        //cout << out_bitlen << "jfjf" << endl;

        if (i%numPoints == numPoints-1) cout << "\n";
    }

    delete party;
    //delete[] A;
    //delete[] B;
    //delete[] s_A;
    //delete[] s_B;
    //delete[] s_out;
    return 0;
}

