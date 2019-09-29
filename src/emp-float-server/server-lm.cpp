#include <sys/ioctl.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include "jlog.h"

#include "util.h"
#include "matmult.h"
#include "rodrigues.h"
#include "projectpoints.h"
#include "trigfuncs.h"
#include "svd.h"
#include "invert.h"
#include "twonormsq.h"
#include "lmlocalization.h"

//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "emp-sh2pc/emp-sh2pc.h"

#include <signal.h>
void signal_callback_handler(int signum) {
   cout << "Caught signal " << signum << endl;
   // Terminate program
   exit(signum);
}

using namespace emp;
using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        MSG("Usage: %s <party number (0 or 1)>\n", argv[0]);
        return 1;
    }

    while(true) {
      int party = atoi(argv[1]) + 1;
      int port = 8080;
      cout << "party: " << party << " port: " << port << endl;
      NetIO *io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

      int ttpport = port+party*17;
      cout << "ttp port: " << ttpport << endl;
      NetIO *ttpio = new NetIO(nullptr, ttpport);

      while(true) {
        auto t1 = chrono::steady_clock ::now();
        auto sec_passed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock ::now() - t1).count();
        int bytes_ready = 0;
        while (!bytes_ready && sec_passed < 3) {
          sec_passed = chrono::duration_cast<chrono::seconds>(chrono::steady_clock ::now() - t1).count();
          ioctl(ttpio->consocket, FIONREAD, &bytes_ready);
        }
        if (sec_passed >= 3) {
          cout << "waited for 3 seconds with no data, restarting server\n";
          break;
        }

        cout << "starting pplm server\n";
        auto [converged, num_iterations] = lm_server(party, io, ttpio);
        if (converged)
            cout << "converged in " << num_iterations << " iterations\n";
        else
            cout << "did not converge\n";
      }
    }

    return 0;
}
