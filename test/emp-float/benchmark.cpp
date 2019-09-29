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
#include <printutil.h>
#include <lmlocalization.h>
#include <cleartext-ref/lmlocalization.hpp>
#include <gaussnewtonlocalization.h>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <eth3d_features.hpp>


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

    int party = atoi(argv[1]) + 1;
    int port = 8080;
    int num_frames = atoi(argv[2]);
    int max_num_pts = atoi(argv[3]);
    cout << "party: " << party << " port: " << port << endl;
    NetIO *io = new NetIO(party==ALICE ? nullptr : "127.0.0.1", port);

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

        for (int frame=0; frame<test_num_frames; frame++) {
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

            for (int i=6; i<=test_num_pts; i+= MAX((test_num_pts-6)/5, 1)) { // at most 5 intervals
                for (int t=0; t<NUM_TRIALS; t++) {
                    cout << "Trial "<<t<<" with "<<i<<" randomly selected points on frame "<<frame<<" from location "<<l<<"\n";

                    // randomly sample i feature pairs from total
                    vector<cv::Point2f> imagePointsSubset;
                    vector<cv::Point3f> objectPointsSubset;
                    for (int p=0; p<i; p++) {
                        int r = dist(rng);
                        imagePointsSubset.push_back(imagePoints[r]);
                        objectPointsSubset.push_back(objectPoints[r]);
                    }

                    {
                        cout << "\ntesting opencv\n";
                        distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
                        rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
                        tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
                        CLOCK(opencv);
                        TIC(opencv);
                        // OpenCV PnP method
                        cv::solvePnP(objectPointsSubset, imagePointsSubset, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
                        TOC(opencv);
                        cout << "opencv result:" << endl;
                        cout << rvec << endl;
                        cout << tvec << endl << endl;
                    }

#ifdef PPL_GN
                    //{
                    //    cout << "testing gn - cleartext\n";
                    //    vector<float> initialGuessCopy = initialGuess;
                    //    CLOCK(cleartext);
                    //    TIC(cleartext);
                    //    gaussNewton<float>( objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                    //    TOC(cleartext);
                    //    printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    //}

                    {
                        cout << "testing gn\n";
                        vector<float> initialGuessCopy = initialGuess;
#if PPL_FLOW == PPL_FLOW_DO
                        WALL_CLOCK(emp_float_gn_time_vs_points_dataobl);
                        WALL_TIC(emp_float_gn_time_vs_points_dataobl);
#else
                        WALL_CLOCK(emp_float_gn_time_vs_points);
                        WALL_TIC(emp_float_gn_time_vs_points);
#endif

                        auto [converged, num_iterations] = test_gaussnewton_circuit(party, io,
                            objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);

                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
#if PPL_FLOW == PPL_FLOW_DO
                        if (norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(emp_float_gn_time_vs_points_dataobl, i);
                            WALL_TOC_CSV_XY_NORMALIZED(emp_float_gn_time_vs_points_dataobl, i, num_iterations);
#else
                        if (converged && norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(emp_float_gn_time_vs_points, i);
                            WALL_TOC_CSV_XY_NORMALIZED(emp_float_gn_time_vs_points, i, num_iterations);
#endif
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "gn_additions", i, num_additions);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "gn_subtractions", i, num_subtractions);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "gn_multiplications", i, num_multiplications);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "gn_divisions", i, num_divisions);
                            MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "gn_bytes_tx", i, io->size_tx);
                            MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "gn_bytes_rx", i, io->size_rx);
                        }
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %d\n", norm);
                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    }

#endif
#ifdef PPL_LM
                    //{
                    //    cout << "testing lm - cleartext\n";
                    //    vector<float> initialGuessCopy = initialGuess;
                    //    CLOCK(cleartext);
                    //    TIC(cleartext);
                    //    lm<float>(objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);
                    //    TOC(cleartext);
                    //    printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    //}

                    {
                        cout << "testing lm\n";
                        vector<float> initialGuessCopy = initialGuess;
#if PPL_FLOW == PPL_FLOW_DO
                        WALL_CLOCK(emp_float_lm_time_vs_points_dataobl);
                        WALL_TIC(emp_float_lm_time_vs_points_dataobl);
#else
                        WALL_CLOCK(emp_float_lm_time_vs_points);
                        WALL_TIC(emp_float_lm_time_vs_points);
#endif

                        auto [converged, num_iterations] = test_lm_circuit(party, io,
                            objectPointsSubset, imagePointsSubset, f, cx, cy, &initialGuessCopy[0]);

                        float error[6];
                        for (int e=0; e<6; e++) {
                            error[e] = initialGuessCopy[e] - gtpose[e];
                        }
                        float norm = twonormsq(error, 6);
#if PPL_FLOW == PPL_FLOW_DO
                        if (norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(emp_float_lm_time_vs_points_dataobl, i);
                            WALL_TOC_CSV_XY_NORMALIZED(emp_float_lm_time_vs_points_dataobl, i, num_iterations);
#else
                        if (converged && norm < GT_MIN_ER) {
                            WALL_TOC_CSV_XY(emp_float_lm_time_vs_points, i);
                            WALL_TOC_CSV_XY_NORMALIZED(emp_float_lm_time_vs_points, i, num_iterations);
#endif
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "lm_additions", i, num_additions);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "lm_subtractions", i, num_subtractions);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "lm_multiplications", i, num_multiplications);
                            MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__, "lm_divisions", i, num_divisions);
                            MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "lm_bytes_tx", i, io->size_tx);
                            MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "lm_bytes_rx", i, io->size_rx);
                        }
                        else
                            MSG("Not printing timing - not converged. Pose error norm: %d\n", norm);
                        printVector("[rotation; translation]", &initialGuessCopy[0], 6);
                    }
#endif

                    // reset stats
                    num_additions = 0;
                    num_subtractions = 0;
                    num_multiplications = 0;
                    num_divisions = 0;
                    io->num_tx = 0;
                    io->num_rx = 0;
                    io->size_tx = 0;
                    io->size_rx = 0;
                    cout << "\n\n";
                }
            }
            feats.nextFrame();
        }
    }

    return 0;
}
