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

share* BuildSinCircuit(share *theta, BooleanCircuit *c);
share* BuildCosCircuit(share *theta, BooleanCircuit *c);

