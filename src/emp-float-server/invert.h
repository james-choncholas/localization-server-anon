#include "emp-sh2pc/emp-sh2pc.h"
#include "emp-tool/emp-tool.h"

using namespace emp;
using namespace std;

void BuildInvertCircuit(Float *in[], int m, int n, Float *res[]);

uint32_t test_invert_circuit(int party, NetIO *io,
        float **a_in, int nRows, int nCols, float **res_in);
