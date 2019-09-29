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

using namespace std;

const uint32_t bitlen = 32;

share* BuildSinCircuit(share *theta, BooleanCircuit *c) {
    float pi = M_PI;
    share* pi_gate = c->PutCONSGate((uint32_t*)&pi, bitlen);
    share* adjusted = c->PutFPGate(theta, pi_gate, DIV, bitlen, 1, no_status);
    share* si = c->PutFPGate(adjusted, SIN); // sets bitlength to 40 (?), other circuit is 33 (?)
    //cout << "sine bitlength " <<si->get_bitlength()<<endl;
    si->set_bitlength(32);
    delete pi_gate;
    delete adjusted;
    return si;




    // Below uses taylor series approx
    ////return x - (pow(x,3)/(3*2)) + (pow(x,5)/(5*4*3*2));// - (pow(x,7)/(7*6*5*4*3*2));
    //share *t1 = c->PutFPGate(theta, theta, MUL, bitlen, 1, no_status);
    //t1 = c->PutFPGate(t1, theta, MUL, bitlen, 1, no_status);
    //float threeFact = 3*2;
    //share* tFGate = c->PutCONSGate((uint32_t*)&threeFact, bitlen);
    //t1 = c->PutFPGate(t1, tFGate, DIV, bitlen, 1, no_status);

    //share *t2 = c->PutFPGate(theta, theta, MUL, bitlen, 1, no_status);
    //t2 = c->PutFPGate(t2, theta, MUL, bitlen, 1, no_status);
    //t2 = c->PutFPGate(t2, theta, MUL, bitlen, 1, no_status);
    //t2 = c->PutFPGate(t2, theta, MUL, bitlen, 1, no_status);
    //float fiveFact = 5*4*3*2;
    //share* fFGate = c->PutCONSGate((uint32_t*)&fiveFact, bitlen);
    //t2 = c->PutFPGate(t2, fFGate, DIV, bitlen, 1, no_status);

    //share *res = c->PutFPGate(theta, t1, SUB, bitlen, 1, no_status);
    //res = c->PutFPGate(res, t2, ADD, bitlen, 1, no_status);

    //// TODO still has mem leaks
    //delete t1;
    //delete tFGate;
    //delete t2;
    //delete fFGate;

    //return res;
}


share* BuildCosCircuit(share *theta, BooleanCircuit *c) {
    float pi = M_PI;
    share* pi_gate = c->PutCONSGate((uint32_t*)&pi, bitlen);
    share* adjusted = c->PutFPGate(theta, pi_gate, DIV, bitlen, 1, no_status);
    share* co = c->PutFPGate(adjusted, COS); // sets bitlength to 40 (?), other circuit is 33 (?)
    //cout << "sine bitlength " <<si->get_bitlength()<<endl;
    co->set_bitlength(32);
    delete pi_gate;
    delete adjusted;
    return co;


    // Below uses taylor series approx

    ////return 1 - (pow(x,2)/2) + (pow(x,4)/(4*3*2));// - (pow(x,6)/(6*5*4*3*2));
    //share *t1 = c->PutFPGate(theta, theta, MUL, bitlen, 1, no_status);
    //float twoFact = 2;
    //share* tFGate = c->PutCONSGate((uint32_t*)&twoFact, bitlen);
    //t1 = c->PutFPGate(t1, tFGate, DIV, bitlen, 1, no_status);

    //share *t2 = c->PutFPGate(theta, theta, MUL, bitlen, 1, no_status);
    //t2 = c->PutFPGate(t2, theta, MUL, bitlen, 1, no_status);
    //t2 = c->PutFPGate(t2, theta, MUL, bitlen, 1, no_status);
    //float fourFact = 4*3*2;
    //share* fFGate = c->PutCONSGate((uint32_t*)&fourFact, bitlen);
    //t2 = c->PutFPGate(t2, fFGate, DIV, bitlen, 1, no_status);

    //float one = 1;
    //share* oneGate = c->PutCONSGate((uint32_t*)&one, bitlen);
    //share *res = c->PutFPGate(oneGate, t1, SUB, bitlen, 1, no_status);
    //res = c->PutFPGate(res, t2, ADD, bitlen, 1, no_status);

    //// TODO still has mem leaks
    //delete t1;
    //delete tFGate;
    //delete t2;
    //delete fFGate;

    //return res;
}


uint32_t test_trig_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing, bool trueForSin) {

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
    //share *s_r[3];
    share *s_in;


    /**
        Step 5: Initialize plaintext values.
    */
    float in = .2;

    /**
        Step 6: Set the coordinates as inputs for the circuit for the respective party.
        The other party's input length must be specified.
    */
    if (role == SERVER) {
        s_in = circ->PutINGate((uint32_t*) &in, 32, role);
    } else {
        s_in = circ->PutDummyINGate(32);
    }


    /**
        Step 7: Call the build method for building the circuit for the
                problem by passing the shared objects and circuit object.
                Don't Forget to type cast the circuit object to type of share
    */
    share* s_res = NULL;
    share* res = NULL; // stores plaintext
    if (trueForSin) {
        s_res = BuildSinCircuit(s_in, (BooleanCircuit*) circ);
    } else {
        s_res = BuildCosCircuit(s_in, (BooleanCircuit*) circ);
    }

    /**
        Step 8: Write the circuit output for both parties.
    */
    res = circ->PutOUTGate(s_res, ALL);

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
    assert(res != NULL);

    uint32_t* output;
    uint32_t out_bitlen, out_nvals;

    // This method only works for an output length of maximum 64 bits in general,
    // if the output length is higher you must use get_clear_value_ptr
    res->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
    //res->get_clear_value_ptr();

    //cout << out_bitlen << endl;
    cout << "result: " << *(float*)output << endl;

    cout << "bits: ";
    for (int i=0; i<32; i++) {
        if (*output & 1<<i) {
            cout << "1";
        } else {
            cout << "0";
        }
    }
    cout << endl;


    delete party;
    //delete[] A;
    //delete[] B;
    //delete[] s_A;
    //delete[] s_B;
    //delete[] s_out;
    return 0;
}

