#include "abycore/sharing/sharing.h"
#include "abycore/circuit/booleancircuits.h"
#include "abycore/circuit/arithmeticcircuits.h"
#include "abycore/circuit/circuit.h"
#include "abycore/aby/abyparty.h"


uint32_t test_invert_circuit(e_role role, const std::string& address, uint16_t port, seclvl seclvl,
        uint32_t nthreads, e_mt_gen_alg mt_alg, e_sharing sharing,
        float **a_in, int nRows, int nCols, float **res);

void BuildInvertCircuit(share **_s_in[], int m, int n, share **s_res[],
        BooleanCircuit *c, ABYParty* party, e_role role,
        std::function<void()> toRawShares=[](){}, std::function<void()> toShareObjects=[](){});
