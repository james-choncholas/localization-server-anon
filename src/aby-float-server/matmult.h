#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmultCircuit(share *M[], int m, int n,
                            share *N[], int mm, int nn,
                            share *res[], BooleanCircuit *c);

// M mxn (M is secret, m n are public)
// N mmxnn
// res mxnn
void BuildMatmult2DwTransposeCircuit(share **M[], int m, int n, const bool transposeM,
                            share **N[], int mm, int nn, const bool transposeN,
                            share **res[], BooleanCircuit *c);

uint32_t test_matmult_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing);

