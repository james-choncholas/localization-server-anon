#include <privacyconf.h>
#include <jlog.h>
#include <stdio.h>
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

#include <svd.h>
#include <invert.h>

using namespace std;

const uint32_t bitlen = 32;

void BuildInvertCircuitWSubCircuits(share **s_in[], int m, int n, share **s_res[],
        BooleanCircuit *c, ABYParty* party, e_role role,
        std::function<void()> toRawShares, std::function<void()> toShareObjects) {

    assert(m>=n);
    float zero = 0;

    // svd overwrites with s_in with u matrix, must copy shares
    share*** s_u = new share**[m];
    for(int i=0; i<m; i++) {
        s_u[i] = new share*[n];
        for (int j=0; j<n; j++) {
            s_u[i][j]=s_in[i][j];
        }
    }

    share** s_w = new share*[m]; // aka sigma, only diag
    share*** s_v = new share**[n]; //nxn
    for(int i=0; i<n; i++)
        s_v[i] = new share*[n];

    BuildAndRunSvd(s_u, m, n, s_w, s_v, (BooleanCircuit*)c, party, role, toRawShares, toShareObjects);

    // make new zero gate after BuildAndRunSvd because it makes a new circuit
    share *zerogate = c->PutCONSGate((uint32_t*)&zero, bitlen);

    // res = inv(in) = v*inv(w)*uT
    // inv(w)*uT
    for (int j=0;j<n;j++) {
        // if (s_w[j]) {
            share *ifw = s_w[j]->get_wire_ids_as_share(0);
            for (uint32_t tt=1; tt<bitlen-1;  tt++) { // -1 -> do not include sign bit
                share *tempbit = s_w[j]->get_wire_ids_as_share(tt);
                share *temp = ifw;
                ifw = c->PutORGate(temp, tempbit);
                delete tempbit;
                delete temp;
            }

            for (int i=0;i<m;i++) {
                share *temp = c->PutFPGate(s_u[i][j], s_w[j], DIV, bitlen, 1, no_status);
                share *temp2 = s_u[i][j];
                s_u[i][j] = c->PutMUXGate(temp, zerogate, ifw);
                delete temp;
                //delete temp2; DO NOT DELETE s_u, it is still part of _s_in
            }
            delete ifw;
        //}
    }

    // v*(w*uT) (don't use matmult so we can do transpose ourselves)
    for (int j=0;j<n;j++) {
        for (int jj=0;jj<m;jj++) {
            // dont delete s_res[j][jj] - share may be elsewhere
            s_res[j][jj] = c->PutCONSGate((uint32_t*)&zero, bitlen);
            for (int k=0; k<n; k++) {
                // note u indices do transpose
                share *temp = c->PutFPGate(s_v[j][k], s_u[jj][k], MUL, bitlen, 1, no_status);
                share *temp2 = s_res[j][jj];
                s_res[j][jj] = c->PutFPGate(s_res[j][jj], temp, ADD, bitlen, 1, no_status);
                delete temp;
                delete temp2;
            }
        }
    }
}

void BuildInvertCircuitDO(share **s_in[], int m, int n, share **s_res[],
        BooleanCircuit *c, ABYParty* party, e_role role) {
    assert(m>=n);
    float zero = 0;

    // svd overwrites with s_in with u matrix
    //share*** s_in = new share**[m]; no need to copy shares because new circuits run by svd trash them anyway
    //for(int i=0; i<m; i++) {
    //    s_in[i] = new share*[n];
    //    for (int j=0; j<n; j++) {
    //        s_in[i][j]=_s_in[i][j];
    //    }
    //}

    share** s_w = new share*[m]; // aka sigma, only diag
    share*** s_v = new share**[n]; //nxn
    for(int i=0; i<n; i++)
        s_v[i] = new share*[n];

    BuildAndRunSvd(s_in, m, n, s_w, s_v, (BooleanCircuit*)c, party, role);

    share*** s_u = s_in;
    share *zerogate = c->PutCONSGate((uint32_t*)&zero, bitlen);

    // res = inv(in) = v*inv(w)*uT
    // inv(w)*uT
    for (int j=0;j<n;j++) {
        // if (s_w[j]) {
            share *ifw = s_w[j]->get_wire_ids_as_share(0);
            for (uint32_t tt=1; tt<bitlen-1;  tt++) { // -1 -> do not include sign bit
                share *tempbit = s_w[j]->get_wire_ids_as_share(tt);
                share *temp = ifw;
                ifw = c->PutORGate(temp, tempbit);
                delete tempbit;
                delete temp;
            }

            for (int i=0;i<m;i++) {
                share *temp = c->PutFPGate(s_u[i][j], s_w[j], DIV, bitlen, 1, no_status);
                share *temp2 = s_u[i][j];
                s_u[i][j] = c->PutMUXGate(temp, zerogate, ifw);
                delete temp;
                //delete temp2; DO NOT DELETE s_u, it is still part of _s_in
            }
            delete ifw;
        //}
    }

    // v*(w*uT) (don't use matmult so we can do transpose ourselves)
    for (int j=0;j<n;j++) {
        for (int jj=0;jj<m;jj++) {
            // dont delete s_res[j][jj] - share may be elsewhere
            s_res[j][jj] = c->PutCONSGate((uint32_t*)&zero, bitlen);
            for (int k=0; k<n; k++) {
                // note u indices do transpose
                share *temp = c->PutFPGate(s_v[j][k], s_u[jj][k], MUL, bitlen, 1, no_status);
                share *temp2 = s_res[j][jj];
                s_res[j][jj] = c->PutFPGate(s_res[j][jj], temp, ADD, bitlen, 1, no_status);
                delete temp;
                delete temp2;
            }
        }
    }
}

void BuildInvertCircuit(share **s_in[], int m, int n, share **s_res[],
        BooleanCircuit *c, ABYParty* party, e_role role,
        std::function<void()> toRawShares, std::function<void()> toShareObjects) {
#if PPL_FLOW==PPL_FLOW_DO // set dx to zero if no error
    (void)toRawShares;
    (void)toShareObjects;
    BuildInvertCircuitDO(s_in, m, n, s_res,
            c, party, role);
#elif PPL_FLOW==PPL_FLOW_LOOP_LEAK
    BuildInvertCircuitWSubCircuits(s_in, m, n, s_res,
            c, party, role, toRawShares, toShareObjects);
#endif
}

uint32_t test_invert_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float **a_in, int nRows, int nCols, float **res) {
    uint32_t reservegates = 65536;
    const std::string& abycircdir = "../../extern/ABY/bin/circ";
    ABYParty* party = new ABYParty(role, address, port, seclvl, bitlen, nthreads, mt_alg, reservegates, abycircdir);
    std::vector<Sharing*>& sharings = party->GetSharings();
    Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();
    Circuit* bc = sharings[S_BOOL]->GetCircuitBuildRoutine();

    // share arrays
    share*** s_in = new share**[nRows];
    for(int i=0; i<nRows; i++) {
        s_in[i] = new share*[nCols];
    }

    // initialize input shares
    //  always use boolean shares (not yao) so BuildAndRunSvd (called in BuildInvertCircuit)
    //  can get raw shares. This is because you cant use Y2B shares on Yao input gates.
    if (role == SERVER) {
        for(int i=0; i<nRows; i++) {
            for (int j=0; j<nCols; j++) {
                s_in[i][j] = bc->PutINGate((uint32_t*) &a_in[i][j], 32, role);
            }
        }
    } else {
        for(int i=0; i<nRows; i++) {
            for (int j=0; j<nCols; j++) {
                s_in[i][j] = bc->PutDummyINGate(32);
            }
        }
    }

    // res is nxm not mxn
    share*** s_res = new share**[nCols];
    for(int i=0; i<nCols; i++) {
        s_res[i] = new share*[nRows];
    }

    CLOCK(BuildInvert);
    TIC(BuildInvert);
    BuildInvertCircuit(s_in, nRows, nCols, s_res,
            (BooleanCircuit*) circ, party, role);
    TOC(BuildInvert);

    for(int i=0; i<nCols; i++) {
        for (int j=0; j<nRows; j++) {
            share *temp = s_res[i][j];
            s_res[i][j] = circ->PutOUTGate(s_res[i][j], ALL);
            delete temp;
        }
    }

    CLOCK(ExecInvert);
    TIC(ExecInvert);
    party->ExecCircuit();
    TOC(ExecInvert);

    for(int i=0; i<nCols; i++) {
        for (int j=0; j<nRows; j++) {
            uint32_t* output;
            uint32_t out_bitlen, out_nvals;

            // This method only works for an output length of maximum 64 bits in general,
            // if the output length is higher you must use get_clear_value_ptr
            s_res[i][j]->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
            //s_out[i]->get_clear_value_ptr();

            //cout << out_bitlen << "jfjf" << endl;
            cout << *(float*)output << " ";
            //cout << i << " ";
        }
        cout << endl;
    }

    delete party;

    return 0;
}
