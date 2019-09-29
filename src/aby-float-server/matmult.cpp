#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

using namespace std;

const uint32_t bitlen = 32;

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmultCircuit(share *M[], int m, int n,
                            share *N[], int mm, int nn,
                            share *res[], BooleanCircuit *c) {
    (void) mm;
    assert(n == mm);
    for (int i=0; i<m; i++) { //row
        for (int j=0; j<nn; j++) { //col
            float z = 0;
            res[i*nn +j] = c->PutCONSGate((uint32_t*)&z, 32);
            for (int k=0; k<n; k++) {
                share *temp = c->PutFPGate(M[i*n + k], N[k*nn + j], MUL, bitlen, 1, no_status);
                share *temp2 = res[i*nn +j];
                res[i*nn + j] = c->PutFPGate(res[i*nn + j], temp, ADD, bitlen, 1, no_status);
                delete temp;
                delete temp2;
            }
        }
    }
}

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmult2DwTransposeCircuit(share **M[], int m, int n, const bool transposeM,
                            share **N[], int mm, int nn, const bool transposeN,
                            share **res[], BooleanCircuit *c) {
    if (transposeM) std::swap(m, n);
    if (transposeN) std::swap(mm, nn);
    assert(n == mm);

    float z = 0;
    for (int i=0; i<m; i++) { //row
        for (int j=0; j<nn; j++) { //col
            res[i][j] = c->PutCONSGate((uint32_t*)&z, 32);

            for (int k=0; k<n; k++) {
                share* prod;
                if (!transposeM && !transposeN)
                    prod = c->PutFPGate(M[i][k], N[k][j], MUL, bitlen, 1, no_status);
                else if (!transposeM && transposeN)
                    prod = c->PutFPGate(M[i][k], N[j][k], MUL, bitlen, 1, no_status);
                else if (transposeM && !transposeN)
                    prod = c->PutFPGate(M[k][i], N[k][j], MUL, bitlen, 1, no_status);
                else if (transposeM && transposeN)
                    prod = c->PutFPGate(M[k][i], N[j][k], MUL, bitlen, 1, no_status);

                share *oldres = res[i][j];
                res[i][j] = c->PutFPGate(res[i][j], prod, ADD, bitlen, 1, no_status);
                delete prod;
                delete oldres;
            }
        }
    }
}


uint32_t test_matmult_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
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
    int m=3, n=5, mm=5, nn=4;
    share *s_A[m*n], *s_B[mm*nn];


    /**
        Step 5: Initialize plaintext values.
    */
    float* A = new float[m*n];
    float* B = new float[mm*nn];

    /**
        Step 6: Set the coordinates as inputs for the circuit for the respective party.
        The other party's input length must be specified.
    */

    for (int i=0; i<m*n; ++i) {
        A[i] = 1;
        if (role == SERVER)
            s_A[i] = circ->PutINGate((uint32_t*)&A[i], 32, role);
        else
            s_A[i] = circ->PutDummyINGate(32);
    }

    for (int i=0; i<mm*nn; ++i) {
        B[i] = 2;
        if (role == SERVER)
            s_B[i] = circ->PutINGate((uint32_t*)&B[i], 32, role);
        else
            s_B[i] = circ->PutDummyINGate(32);
    }


    /**
        Step 7: Call the build method for building the circuit for the
                problem by passing the shared objects and circuit object.
                Don't Forget to type cast the circuit object to type of share
    */

    share *res[m*nn];
    share *s_out[m*nn];// stores plaintext
    CLOCK(BuildCircuit);
    TIC(BuildCircuit);
    BuildMatmultCircuit(s_A, m, n, s_B, mm, nn,
            res, (BooleanCircuit*) circ);
    TOC(BuildCircuit);

    /**
        Step 8: Write the circuit output to s_out for both parties.
    */
    for (int i=0; i<m*nn; ++i) {
        s_out[i] = circ->PutOUTGate(res[i], ALL);
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
    for (int i=0; i<m*nn; i++) {
        uint32_t* output;
        uint32_t out_bitlen, out_nvals;

        // This method only works for an output length of maximum 64 bits in general,
        // if the output length is higher you must use get_clear_value_ptr
        s_out[i]->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
        //s_out[i]->get_clear_value_ptr();

        //cout << out_bitlen << "jfjf" << endl;
        cout << *(float*)output << " ";
        //cout << i << " ";
        if (i%nn==nn-1) cout << endl;
    }

    delete party;
    //delete[] A;
    //delete[] B;
    //delete[] s_A;
    //delete[] s_B;
    //delete[] s_out;
    return 0;
}
