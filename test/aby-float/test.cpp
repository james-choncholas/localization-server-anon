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

#include <abycore/sharing/sharing.h>
#include <abycore/circuit/booleancircuits.h>
#include <abycore/circuit/arithmeticcircuits.h>
#include <abycore/circuit/circuit.h>
#include <abycore/aby/abyparty.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        MSG("Usage: %s <party number>\n", argv[0]);
        return 1;
    }


    MSG("\nStarting ABY...\n");
    e_role role = (e_role) atoi(argv[1]);
    uint32_t /*secparam = 128,*/ nthreads = 1;
    uint16_t port = 7766;
    std::string address = "127.0.0.1";
    e_mt_gen_alg mt_alg = MT_OT;
    seclvl seclvl = LT;//get_sec_lvl(secparam);

    std::vector<e_sharing> ctypes = {/*S_ARITH,*/ S_BOOL, S_YAO}; // cant do floats with arith
    map<int, string> cnames = {
        {S_ARITH, "arith" },
        {S_BOOL, "bool" },
        {S_YAO, "yao" },
    };

    float f=715;
    float cx=354;
    float cy=245;
    float _cM[] = {f, 0, cx,
                   0, f, cy,
                   0, 0, 1};
    cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
    cv::Mat distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
    cv::Mat rvec(3,1,cv::DataType<float>::type);
    cv::Mat tvec(3,1,cv::DataType<float>::type);

    vector<cv::Point2f> imagePoints;
    vector<cv::Point3f> objectPoints;
    Hoffs2DPoints(imagePoints);
    Hoffs3DPoints(objectPoints);



    MSG("\n\ntesting trigfuncs - cleartext\n");
    float lulz = .2;
    float lulzsin = sin(lulz);
    float lulzcos = cos(lulz);

    cout << "real      sin: "<< lulzsin <<" cos: "<< lulzcos << endl;
    cout << "plaintext sin: "<< mysin(lulz) <<" cos: "<< mycos(lulz) << endl;

    cout << "sin bits: ";
    for (int i=0; i<32; i++) {
        if (*(uint32_t*)&lulzsin & 1<<i) {
            cout << "1";
        } else {
            cout << "0";
        }
    }
    cout << endl;
    cout << "cos bits: ";
    for (int i=0; i<32; i++) {
        if (*(uint32_t*)&lulzcos & 1<<i) {
            cout << "1";
        } else {
            cout << "0";
        }
    }
    cout << endl;

    for (auto ctype : ctypes) {
        cout << "test sin circuit - "<< cnames[ctype] <<"\n";
        CLOCK(sin);
        TIC(sin);
        test_trig_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, true);
        TOC(sin);
        cout << "test cos circuit - "<< cnames[ctype] <<"\n";
        CLOCK(cos);
        TIC(cos);
        test_trig_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, false);
        TOC(cos);
    }
    //return 0;



    MSG("\n\ntesting matmult - cleartext\n");
    cv::Mat A = cv::Mat::ones(3, 5, cv::DataType<float>::type);
    cv::Mat B = cv::Mat::ones(5, 4, cv::DataType<float>::type)*2;
    cv::Mat C = cv::Mat::zeros(3, 4, cv::DataType<float>::type);
    matmult(A.ptr<float>(),3,5, B.ptr<float>(),5,4, C.ptr<float>());
    cout << C << endl;

    for (auto ctype : ctypes) {
        cout << "test matmult circuit - "<< cnames[ctype] <<"\n";
        test_matmult_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype);
    }



    MSG("WARNING - if this fails because it can't find circuit directory, cd to where the bin is");

    MSG("\n\ntesting rodrigues - cleartext\n");
    float r[3]={.2, .4, .2}; //3x1
    cv::Mat R = cv::Mat::zeros(3, 4, cv::DataType<float>::type);
    rodrigues(r, R.ptr<float>());
    cout << R << endl;

    for (auto ctype : ctypes) {
        cout << "test rodrigues circuit - "<< cnames[ctype] <<"\n";
        test_rodrigues_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype);
    }





    MSG("\n\ntesting projectpoints - cleartext\n");
    distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
    rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
    tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);

    // Find rotation and translation
    cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);

    int nOp = objectPoints.size();
    cv::Mat x = cv::Mat::zeros(6, 1, cv::DataType<float>::type);
    float* xptr = x.ptr<float>();
    xptr[0] = rvec.at<float>(0);
    xptr[1] = rvec.at<float>(1);
    xptr[2] = rvec.at<float>(2);
    xptr[3] = tvec.at<float>(0);
    xptr[4] = tvec.at<float>(1);
    xptr[5] = tvec.at<float>(2);
    //cout << x << endl;

    cv::Mat matOP = cv::Mat(objectPoints, false);
    matOP = matOP.reshape(1, matOP.total());
    matOP.convertTo(matOP, cv::DataType<float>::type);
    cv::Mat P = cv::Mat::ones(4, nOp, cv::DataType<float>::type);
    P( cv::Range(0,3), cv::Range(0,nOp) ) = matOP.t(); // copy points into new matrix
    //cout << P << endl;

    // These points had better be close to the original points
    cout << "\nThe original points" << endl;
    cout << imagePoints << endl;

    cv::Mat projected = cv::Mat(3, nOp, cv::DataType<float>::type);
    projectPoints(P.ptr<float>(), x.ptr<float>(), cameraMatrix.ptr<float>(), projected.ptr<float>(),nOp);
    cout << "\ncleartext projection" << endl;
    cout << projected << endl;

    for (auto ctype : ctypes) {
        cout << "test project points circuit - "<< cnames[ctype] <<"\n";
        test_projectpoints_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype,
                P.ptr<float>(), x.ptr<float>(), cameraMatrix.ptr<float>(), NULL, nOp);
    }


    float a,b;
    MSG("\n\ntesting svd - fabs subfunc - cleartext\n");
    a=-2.40;
    cout << "fabs: " << fabs(a) << endl;
    for (auto ctype : ctypes) {
        cout << "test svd circuit - fabs subfunc - "<< cnames[ctype] <<"\n";
        test_fabs_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, a);
    }

    MSG("testing svd - sign subfunc - cleartext\n");
    a=-2.40,b=-96.0;
    cout << "sign: " << mysign(a,b) << endl;
    for (auto ctype : ctypes) {
        cout << "test svd circuit - sign subfunc - "<< cnames[ctype] <<"\n";
        test_sign_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, a, b);
    }

    MSG("testing svd - fmax subfunc - cleartext\n");
    a=-2.40,b=96.0;
    cout << "fmax: " << myfmax(a,b) << endl;
    for (auto ctype : ctypes) {
        cout << "test svd circuit - fmax subfunc - "<< cnames[ctype] <<"\n";
        test_fmax_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, a, b);
    }

    MSG("testing svd - negative subfunc - cleartext\n");
    a=-2.40;
    cout << "result: " << -a << endl;
    for (auto ctype : ctypes) {
        cout << "test svd circuit - negative subfunc - "<< cnames[ctype] <<"\n";
        test_negative_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, a);
    }

    MSG("testing svd - pythag subfunc - cleartext\n");
    a=-2.40, b=96.0;
    cout << "result: " << mypythag(a,b) << endl;
    for (auto ctype : ctypes) {
        cout << "test svd circuit - pythag subfunc - "<< cnames[ctype] <<"\n";
        test_pythag_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype, a, b);
    }


    for (int iter=0; iter<1+ctypes.size(); iter++) {
        int m=5,n=4;
        // svd overwrites with u matrix
        float** in = new float*[m];
        for(int i=0; i<m; i++) {
            in[i] = new float[n];
            for (int j=0; j<n; j++) {
                in[i][j]=i+j;
            }
        }
        in[0][0] = 0;
        in[1][0] = 0;
        in[2][0] = 0;
        in[3][0] = 0;
        in[4][0] = 0;

        float* w = new float[n]; // aka sigma, only diag
        float** v = new float*[n]; //nxn
        for(int i=0; i<n; i++)
            v[i] = new float[n];

        if (iter == 0) {
            MSG("\n\ntesting svd - cleartext\n");
            svdcmp(in, m, n, w, v);
        } else {
            auto ct = ctypes[iter-1];
            cout << "testing svd - "<< cnames[ct] <<"\n";
            test_svd_circuit(role, address, port, seclvl, nthreads, mt_alg, ct,
                    in, m, n, w, v);
        }

        printVector("w", w, n);
        printMatrix("v", v, n, n);
        printMatrix("a", in, m, n);
        for (int i=0; i<m; i++) {
            delete[] in[i];
        }
        delete[] in;
        delete[] w;
        for (int i=0; i<n; i++) {
            delete[] v[i];
        }
        delete[] v;
    }



    MSG("\ntesting myinvert\n");
    int m=12,n=6;
    //float one_d_M[] = {3, 0, 1, // toy example
    //                   0, 0, 0,
    //                   0, 4, 0,
    //                   0, 0, 0,
    //                   2, 0, 0,
    //                   0, 0, 3,
    //                   0, 0, 0};
    float one_d_M[] = {-9.1552734, 149.53613, -59.509277, 18.310547, 0, 3.0517578,
                       -164.79492, -64.086914, -47.302246, 0, 19.836426, 1.5258789,
                       27.46582, 77.819824, 50.354004, 22.888184, 0, 1.5258789,
                       -56.45752, -42.724609, -53.405762, 0, 22.888184, 3.0517578,
                       45.776367, 79.345703, 79.345703, 21.362305, 0, 0,
                       -32.806396, -13.73291, -20.599365, 0, 24.414062, 4.5776367,
                       -21.362305, 108.3374, -93.078613, 16.784668, 0, 3.0517578,
                       -152.58789, -45.776367, -10.681152, 0, 19.836426, 3.0517578,
                       6.1035156, 39.672852, 3.0517578, 21.362305, 0, 0,
                       -39.672852, -21.362305, -16.784668, 0, 22.888184, 1.5258789,
                       24.414062, 48.828125, 27.46582, 24.414062, 0, 0,
                       -13.73291, 6.1035156, 12.207031, 0, 22.888184, 1.5258789};

    cv::Mat cvM = cv::Mat(m, n, cv::DataType<float>::type, one_d_M);
    cv::Mat ores;
    invert(cvM, ores, cv::DECOMP_SVD);
    cout << "opencv result:\n" << ores << endl;

    float** M = new float*[m];
    for(int i=0; i<m; i++) {
        M[i] = new float[n];
        for (int j=0; j<n; j++) {
            M[i][j] = one_d_M[i*n+j];
        }
    }

    // res is nxm (not mxn)
    float** res = new float*[n];
    for(int i=0; i<n; i++) {
        res[i] = new float[m];
    }

    myinvert(M, m, n, res);
    printMatrix("cleartext result", res, n, m);

    for (auto ctype : ctypes) {
        cout << "testing invert - "<< cnames[ctype] <<"\n";
        test_invert_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype,
            M, m, n, res);
    }



    cout << "\ntesting 2 norm squared - cleartext\n";
    float tvect[] = { 3.1, 5, 2, 4, 1 };
    cout << twonormsq(tvect, 5) << endl;

    for (auto ctype : ctypes) { // Cant do arithmetic circuit
        cout << "testing 2 norm squared - "<< cnames[ctype] <<"\n";
        test_twonormsq_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype,
            tvect, 5);
    }

    cout << "\ntesting gauss newton - opencv\n";
    distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
    rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
    tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
    CLOCK(opencv);
    TIC(opencv);
    // OpenCV PnP method
    cv::solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
    TOC(opencv);
    cout << "opencv result:" << endl;
    cout << rvec << endl;
    cout << tvec << endl << endl;

#ifdef PPL_GN
    {
        cout << "testing gauss newton - cleartext\n";
        float rt[6] = {0, 0, 0, 0, 0, 0}; // initial guess
        CLOCK(cleartext);
        TIC(cleartext);
        gaussNewton<float>( objectPoints, imagePoints, f, cx, cy, rt);
        TOC(cleartext);
        printVector("[rotation; translation]", rt, 6);

        for (auto ctype : ctypes) { // Cant do arithmetic circuit
            cout << "testing gauss newton - "<< cnames[ctype] <<"\n";
            float rt[6] = {0, 0, 0, 0, 0, 0}; // initial guess
            test_gaussnewton_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype,
                objectPoints, imagePoints, f, cx, cy, rt);
            printVector("[rotation; translation]", rt, 6);
            PrintSumTimings(objectPoints.size());
            ClearSumTimings();
        }
    }
#endif

#ifdef PPL_LM
    {
        cout << "testing lm - cleartext\n";
        float rt[6] = {0, 0, 0, 0, 0, 0}; // initial guess
        CLOCK(cleartext);
        TIC(cleartext);
        lm<float>( objectPoints, imagePoints, f, cx, cy, rt);
        TOC(cleartext);
        cout << "[rotation; translation]:\n";
        printVector("[rotation; translation]", rt, 6);

        for (auto ctype : ctypes) { // Cant do arithmetic circuit
            cout << "testing lm - "<< cnames[ctype] <<"\n";
            float rt[6] = {0, 0, 0, 0, 0, 0}; // initial guess
            test_lm_circuit(role, address, port, seclvl, nthreads, mt_alg, ctype,
                objectPoints, imagePoints, f, cx, cy, rt);
            printVector("[rotation; translation]", rt, 6);
            PrintSumTimings(objectPoints.size());
            ClearSumTimings();
        }
    }
#endif

    return 0;
}
