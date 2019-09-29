#include <stdio.h>
#include <jlog.h>
#include <iostream>
#include <string>

#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

#include <trigfuncs.h>

using namespace emp;
using namespace std;

Float BuildSinCircuit(Float a) {
    Float pi = Float(M_PI, PUBLIC);
    return (a/pi).sin(); // circuit mutliplies by pi for some reason
}


Float BuildCosCircuit(Float a) {
    Float pi = Float(M_PI, PUBLIC);
    return (a/pi).cos(); // circuit mutliplies by pi for some reason
}

void print_float32(Float a) {
    for(int i = 31; i >= 0; i--)
        printf("%d", a[i].reveal<bool>());
    cout << endl;
}

void test_trig_circuit(float val, int party, NetIO *io, bool trueForSin) {
    setup_semi_honest(io, party);

    Float v = Float(val, ALICE);
    if (trueForSin) {
        Float res = BuildSinCircuit(v);
        cout << "sin result: " << res.reveal<double>() << endl;
    } else {
        Float res = BuildCosCircuit(v);
        cout << "cos result: " << res.reveal<double>() << endl;
    }
    cout << "bits: ";
    print_float32(v);
    //cout << CircuitExecution::circ_exec->num_and()<<endl;
    finalize_semi_honest();
    return;
}
