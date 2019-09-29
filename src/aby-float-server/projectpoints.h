#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"

#include <matmult.h>
#include <rodrigues.h>

// Note: All 2D matrix inputs passed as single dimension array
// P=4xnumPoints [x1, x2, ...;  y1, y2, ...;  z1, z2, ...; 1, 1, ... ]
// x=6x1 [ rotation angles; translation] (column)
// K=3x3 camera intrinsic
// result=3xnumPoints preallocated, only first two rows are x,y.
//      last row needed for intermediate calculation (homogeneous)
//      [ u1, u2, ...;  v1, v2, ...;  1, 1, ... ]
void BuildProjectPointsCircuit(share *_P[], share *_x[], share *_K[], share *_res[],
        int numPoints, BooleanCircuit *c, bool skipOneGate);

uint32_t test_projectpoints_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float* _P, float* _x, float* _K, float* _res, int numPoints);
