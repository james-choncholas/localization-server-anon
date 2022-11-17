#include <eth3d_features.hpp>
//#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <stdio.h>
#include <iostream>
#include <string>
#include <jlog.h>
#include <stdlib.h>
#include <math.h>
#include <random>

#include <util.h>
#include <gaussnewtonlocalization.h>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <lmlocalization.h>
#include <cleartext-ref/lmlocalization.hpp>

#include <abycore/sharing/sharing.h>
#include <abycore/circuit/booleancircuits.h>
#include <abycore/circuit/arithmeticcircuits.h>
#include <abycore/circuit/circuit.h>
#include <abycore/aby/abyparty.h>

// to get path name
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

using namespace std;

#define NUM_TRIALS 3
#define SEED 0x666

int main(int argc, char** argv) {
    if (argc != 4) {
        MSG("Usage: %s <party number> <num frames> <max num pts>\n", argv[0]);
        return 1;
    }
    e_role role = (e_role) atoi(argv[1]);
    int num_frames = atoi(argv[2]);
    int max_num_pts = atoi(argv[3]);


    MSG("\nStarting ABY...\n");
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


    vector<cv::Point2f> imagePoints;
    vector<cv::Point3f> objectPoints;

    // ETH3D setup
    float f=3408.57;
    float cx=3114.7;
    float cy=2070.92;
    char datadir[PATH_MAX];
    strncpy(datadir, argv[0], sizeof(datadir));
    dirname(datadir);
    std::string bindir(datadir);
    std::string base_path = bindir + "/../../data-eth3d/";

    //for (int l=0; l<eth3d_locations.size(); l++) {
    for (int l=0; l<1; l++) {
        auto feats = ETH3DFeatures<float>(base_path, eth3d_locations[l]);

        int test_num_frames = MIN(feats.numberOfFrames(), num_frames);

        for (int frame=0; frame<=test_num_frames; frame++) {
            imagePoints.clear();
            objectPoints.clear();
            feats.imageFeatures(imagePoints);
            feats.worldFeatures(objectPoints);
            auto gtpose = feats.getGroundTruthPose();
            vector<float> initialGuess = {0, 0, 0, 0, 0, 0};
            //vector<float> initialGuess = {res.first[0], res.first[1], res.first[2],
            //        res.second[0], res.second[1], res.second[2]};
            //uint32_t numPts = imagePoints.size();
            //MSG("Found %d points\n", numPts);
            //for (int i=0; i<imagePoints.size(); i++) {
            //    MSG("Point %d 2d %f, %f\n", i, imagePoints[i].x, imagePoints[i].y);
            //    MSG("Point %d 3d %f, %f, %f\n", i, objectPoints[i].x, objectPoints[i].y, objectPoints[i].z);
            //    //MSG("Point %d 3d %d, %d, %d\n", i, *(int*)&objectPoints[1].x, *(int*)&objectPoints[1].y, *(int*)&objectPoints[1].z);
            //}
            printVector("Ground Truth Pose [rotation; translation]: ", &gtpose[0], 6);


            // General Setup
            float _cM[] = {f, 0, cx,
                           0, f, cy,
                           0, 0, 1};
            cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
            cv::Mat distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
            cv::Mat rvec(3,1,cv::DataType<float>::type);
            cv::Mat tvec(3,1,cv::DataType<float>::type);

            int max_image_points=imagePoints.size();
            int test_num_pts = MIN(max_image_points, max_num_pts);

            //std::random_device dev;
            //std::mt19937 rng(dev());
            std::mt19937 rng(SEED);
            std::uniform_int_distribution<std::mt19937::result_type> dist(0,max_image_points);

            for (int i=6; i<test_num_pts; i+= MAX((test_num_pts-6)/5, 1)) { // at most 5 intervals
                for (int t=0; t<NUM_TRIALS; t++) {
                    cout << "Testing with "<<i<<" randomly selected points on frame "<<frame<<" from location "<<l<<"\n";

                    // randomly sample i feature pairs from total
                    vector<cv::Point2f> imagePointsSubset;
                    vector<cv::Point3f> objectPointsSubset;
                    for (int p=0; p<i; p++) {
                        int r = dist(rng);
                        imagePointsSubset.push_back(imagePoints[r]);
                        objectPointsSubset.push_back(objectPoints[r]);
                    }

                    //{
                    //    cout << "\ntesting opencv\n";
                    //    distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
                    //    rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
                    //    tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
                    //    // OpenCV PnP method
                    //    CLOCK(opencv);
                    //    TIC(opencv);
                    //    cv::solvePnP(objectPointsSubset, imagePointsSubset, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
                    //    TOC(opencv);
                    //    cout << "opencv result:" << endl;
                    //    cout << rvec << endl;
                    //    cout << tvec << endl << endl;
                    //}

#ifdef PPL_GN
                    //{
                    //    cout << "testing gauss newton - cleartext\n";
                    //    vector<float> initialGuessCopy = initialGuess;
                    //    CLOCK(cleartext);
                    //    TIC(cleartext);
                    //    gaussNewton<float>( objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                    //    TOC(cleartext);
                    //    printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    //}

                    {
                        cout << "testing gauss newton - "<< cnames[S_BOOL] <<"\n";
                        vector<float> initialGuessCopy = initialGuess;
                        WALL_CLOCK(aby_bool_float_gn);
                        WALL_TIC(aby_bool_float_gn);
                        //test_gaussnewton_circuit(role, address, port, seclvl, nthreads, mt_alg, S_BOOL,
                        //    objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);

                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
                        if (norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(aby_bool_float_gn, i);
                            PrintSumTimings(objectPoints.size());
                        }
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %f\n", norm);

                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                        ClearSumTimings();
                    }

                    {
                        cout << "testing gauss newton - "<< cnames[S_YAO] <<"\n";
                        vector<float> initialGuessCopy = initialGuess;
                        WALL_CLOCK(aby_yao_float_gn);
                        WALL_TIC(aby_yao_float_gn);
                        //test_gaussnewton_circuit(role, address, port, seclvl, nthreads, mt_alg, S_YAO,
                        //    objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
                        if (norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(aby_yao_float_gn, i);
                            PrintSumTimings(objectPoints.size());
                        }
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %f\n", norm);
                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                        ClearSumTimings();
                    }
#endif
#ifdef PPL_LM
                    //{
                    //    cout << "testing lm - cleartext\n";
                    //    vector<float> initialGuessCopy = initialGuess;
                    //    CLOCK(cleartext);
                    //    TIC(cleartext);
                    //    lm<float>( objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                    //    TOC(cleartext);
                    //    printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    //}

                    {
                        cout << "testing lm - "<< cnames[S_BOOL] <<"\n";
                        vector<float> initialGuessCopy = initialGuess;
                        WALL_CLOCK(aby_bool_float_lm);
                        WALL_TIC(aby_bool_float_lm);
                        //test_lm_circuit(role, address, port, seclvl, nthreads, mt_alg, S_BOOL,
                        //    objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
                        if (norm < GT_MIN_ER)
                            WALL_TOC_CSV_XY(aby_bool_float_lm, i);
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %f\n", norm);
                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    }
                    {
                        cout << "testing lm - "<< cnames[S_YAO] <<"\n";
                        vector<float> initialGuessCopy = initialGuess;
                        WALL_CLOCK(aby_yao_float_lm);
                        WALL_TIC(aby_yao_float_lm);
                        //test_lm_circuit(role, address, port, seclvl, nthreads, mt_alg, S_YAO,
                        //    objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
                        if (norm < GT_MIN_ER)
                            WALL_TOC_CSV_XY(aby_yao_float_lm, i);
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %f\n", norm);
                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    }
#endif
                }
            }
            feats.nextFrame();
        }
    }

    return 0;
}
