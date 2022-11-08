//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>
#include <iostream>
#include <string>
#include <jlog.h>
#include <hoff_features.hpp>

#include <util.h>
#include <printutil.h>
#include <matmult.h>
#include <rodrigues.h>
#include <projectpoints.h>
#include <trigfuncs.h>
#include <svd.h>
#include <invert.h>
#include <twonormsq.h>
#include <gaussnewtonlocalization.h>
#include <lmlocalization.h>

#include <cleartext-ref/matmult.hpp>
#include <cleartext-ref/rodrigues.hpp>
#include <cleartext-ref/projectpoints.hpp>
#include <cleartext-ref/trigfuncs.hpp>
#include <cleartext-ref/svd.hpp>
#include <cleartext-ref/invert.hpp>
#include <cleartext-ref/twonormsq.hpp>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <cleartext-ref/lmlocalization.hpp>

#include "emp-sh2pc/emp-sh2pc.h"
#include <fixed_point_emp.h>

#include <math.h>

using namespace emp;
using namespace std;

typedef fixed_point_emp<16,16> baset;

int main(int argc, char** argv) {
    if (argc != 2) {
        MSG("Usage: %s <party number (0 or 1)>\n", argv[0]);
        return 1;
    }

    int party = atoi(argv[1]) + 1;
    int port = 8080;
    cout << "party: " << party << " port: " << port << endl;
    NetIO *io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

    { // sanity test
        setup_semi_honest(io, party);
        float fa = .1;
        float fb = -.2;
        baset a(fa);
        baset b(fb);
        baset c = a + b;
        baset d = a - b;
        baset e = a * b;
        baset f = a / b;
        cout << fa << "+" << fb << "=" << c << endl;
        cout << fa << "-" << fb << "=" << d << endl;
        cout << fa << "*" << fb << "=" << e << " " << e.value_.reveal<string>() << endl;
        cout << fa << "/" << fb << "=" << f << endl;
        finalize_semi_honest();
    }

    { // fabs
        setup_semi_honest(io, party);
        baset a(-.1);
        baset c = fabs(a);
        cout << "fabs(" << a << ")=" << c << endl;
        finalize_semi_honest();
    }

    { // trig
        setup_semi_honest(io, party);
        float fa = .1;
        baset a(fa);
        baset c = cos(a);
        baset s = sin(a);
        cout << "cos(" << a << ")=" << c << endl;
        cout << "real cos=" << cos(fa) << endl;
        cout << "sin(" << a << ")=" << s << endl;
        cout << "real sin=" << sin(fa) << endl;
        finalize_semi_honest();
    }

    { // sqrt
        setup_semi_honest(io, party);
        float fa = 16;
        baset a(fa);
        baset s = sqrt(a);
        cout << "sqrt(" << a << ")=" << s << endl;
        cout << "real sqrt=" << sqrt(fa) << endl;
        finalize_semi_honest();
    }
    return 0;
}
