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

#include <rodrigues.h>
#include <trigfuncs.h>

using namespace std;

// r=3x1, R=3x4 (only 3x3 is used here)
// values are secret, sizes are public
void BuildRodriguesCircuit(share *r[], share *R[], BooleanCircuit *c) {

    uint32_t bitlen = 32;
    float one = 1;
    float zero = 0;
    share* one_gate = c->PutCONSGate((uint32_t*)&one, bitlen);
    share* zero_gate = c->PutCONSGate((uint32_t*)&zero, bitlen);

    // calculate theta
    share* rzerosq = c->PutFPGate(r[0], r[0], MUL, bitlen, 1, no_status);
    share* ronesq = c->PutFPGate(r[1], r[1], MUL, bitlen, 1, no_status);
    share* rtwosq = c->PutFPGate(r[2], r[2], MUL, bitlen, 1, no_status);
    share* theta1 = c->PutFPGate(rzerosq, ronesq, ADD, bitlen, 1, no_status);
    share* theta2 = c->PutFPGate(theta1, rtwosq, ADD, bitlen, 1, no_status);
    share* theta = c->PutFPGate(theta2, SQRT);

    share* co = BuildCosCircuit(theta, c);
    share* si = BuildSinCircuit(theta, c);

    share* c1 = c->PutFPGate(one_gate, co, SUB, bitlen, 1, no_status);

    share* itheta = c->PutFPGate(one_gate, theta, DIV, bitlen, 1, no_status);
    // if theta == 0 then itheta = 0
    share *thetanotz = theta->get_wire_ids_as_share(0);
    for (uint32_t tt=1; tt<bitlen-1;  tt++) { // -1 -> do not include sign bit
        share *tempbit = theta->get_wire_ids_as_share(tt);
        share *temp = thetanotz;
        thetanotz = c->PutORGate(temp, tempbit);
        delete tempbit;
        delete temp;
    }
    share *tempitheta = itheta;
    itheta = c->PutMUXGate(itheta, zero_gate, thetanotz);
    delete tempitheta;


    share* x = c->PutFPGate(r[0], itheta, MUL, bitlen, 1, no_status);
    share* y = c->PutFPGate(r[1], itheta, MUL, bitlen, 1, no_status);
    share* z = c->PutFPGate(r[2], itheta, MUL, bitlen, 1, no_status);

    share* xx = c->PutFPGate(x, x, MUL, bitlen, 1, no_status);
    share* yy = c->PutFPGate(y, y, MUL, bitlen, 1, no_status);
    share* zz = c->PutFPGate(z, z, MUL, bitlen, 1, no_status);

    share* xy = c->PutFPGate(x, y, MUL, bitlen, 1, no_status);
    share* xz = c->PutFPGate(x, z, MUL, bitlen, 1, no_status);
    share* yz = c->PutFPGate(y, z, MUL, bitlen, 1, no_status);

    share* sx = c->PutFPGate(si, x, MUL, bitlen, 1, no_status);
    share* sy = c->PutFPGate(si, y, MUL, bitlen, 1, no_status);
    share* sz = c->PutFPGate(si, z, MUL, bitlen, 1, no_status);

    share *temp;

    temp = c->PutFPGate(c1, xx, MUL, bitlen, 1, no_status);
    R[0] = c->PutFPGate(temp, co, ADD, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, xy, MUL, bitlen, 1, no_status);
    R[1] = c->PutFPGate(temp, sz, SUB, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, xz, MUL, bitlen, 1, no_status);
    R[2] = c->PutFPGate(temp, sy, ADD, bitlen, 1, no_status);
    delete temp;


    temp = c->PutFPGate(c1, xy, MUL, bitlen, 1, no_status);
    R[4] = c->PutFPGate(temp, sz, ADD, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, yy, MUL, bitlen, 1, no_status);
    R[5] = c->PutFPGate(temp, co, ADD, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, yz, MUL, bitlen, 1, no_status);
    R[6] = c->PutFPGate(temp, sx, SUB, bitlen, 1, no_status);
    delete temp;


    temp = c->PutFPGate(c1, xz, MUL, bitlen, 1, no_status);
    R[8] = c->PutFPGate(temp, sy, SUB, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, yz, MUL, bitlen, 1, no_status);
    R[9] = c->PutFPGate(temp, sx, ADD, bitlen, 1, no_status);
    delete temp;

    temp = c->PutFPGate(c1, zz, MUL, bitlen, 1, no_status);
    R[10] = c->PutFPGate(temp, co, ADD, bitlen, 1, no_status);
    delete temp;


    delete rzerosq;
    delete ronesq;
    delete rtwosq;

    delete theta1;
    delete theta2;
    delete theta;
    delete itheta;

    delete c1;
    delete co;
    delete si;

    delete x;
    delete y;
    delete z;

    delete xx;
    delete yy;
    delete zz;

    delete xy;
    delete xz;
    delete yz;

    delete sx;
    delete sy;
    delete sz;

    delete one_gate;
    delete zero_gate;
}

uint32_t test_rodrigues_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg,
        e_sharing sharing) {

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
    share *s_r[3];


    /**
        Step 5: Initialize plaintext values.
    */
    float* r = new float[3];

    /**
        Step 6: Set the coordinates as inputs for the circuit for the respective party.
        The other party's input length must be specified.
    */
    r[0] = .2f;
    r[1] = .4f;
    r[2] = .2f;
    if (role == SERVER) {
        s_r[0] = circ->PutINGate((uint32_t*) &r[0], 32, role);
        s_r[1] = circ->PutINGate((uint32_t*) &r[1], 32, role);
        s_r[2] = circ->PutINGate((uint32_t*) &r[2], 32, role);
    } else {
        s_r[0] = circ->PutDummyINGate(32);
        s_r[1] = circ->PutDummyINGate(32);
        s_r[2] = circ->PutDummyINGate(32);
    }


    /**
        Step 7: Call the build method for building the circuit for the
                problem by passing the shared objects and circuit object.
                Don't Forget to type cast the circuit object to type of share
    */
    //uint8_t* R = new uint8_t[12];
    share *s_R[12];
    share *R[12]; // stores plaintext
    BuildRodriguesCircuit(s_r, s_R, (BooleanCircuit*) circ);

    /**
        Step 8: Write the circuit output to R for both parties.
    */
    for (int i=0; i<12; ++i) {
        if (i==3 || i==7 || i==11) continue;
        R[i] = circ->PutOUTGate(s_R[i], ALL);
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
    for (int i=0; i<12; i++) {
        if (i==3 || i==7 || i==11) {
            cout << endl;
            continue;
        }
        assert(R[i] != NULL);

        uint32_t* output;
        uint32_t out_bitlen, out_nvals;

        // This method only works for an output length of maximum 64 bits in general,
        // if the output length is higher you must use get_clear_value_ptr
        R[i]->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
        //R[i]->get_clear_value_ptr();

        //cout << out_bitlen << "jfjf" << endl;
        cout << *(float*)output << " ";
    }

    delete party;
    //delete[] A;
    //delete[] B;
    //delete[] s_A;
    //delete[] s_B;
    //delete[] s_out;
    return 0;
}
