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

#include <fixed_point.h>

using namespace std;

typedef fixed_point<int64_t,32> baset;

int main(int argc, char** argv) {
    vector<cv::Point_<float>> fimagePoints;
    vector<cv::Point3_<float>> fobjectPoints;
    vector<cv::Point_<baset>> bimagePoints;
    vector<cv::Point3_<baset>> bobjectPoints;

    // ETH3D setup
    baset ff=3408.57;
    baset fcx=3114.7;
    baset fcy=2070.92;
    baset bf=3408.57;
    baset bcx=3114.7;
    baset bcy=2070.92;

    char datadir[PATH_MAX];
    strncpy(datadir, argv[0], sizeof(datadir));
    dirname(datadir);
    std::string bindir(datadir);
    std::string base_path = bindir + "/../../data-eth3d/";

    auto ffeats = ETH3DFeatures<float>(base_path, eth3d_locations[0]);
    ffeats.imageFeatures(fimagePoints);
    ffeats.worldFeatures(fobjectPoints);
    auto fgtpose = ffeats.getGroundTruthPose();
    vector<float> finitialGuess = {0, 0, 0, 0, 0, 0};

    auto bfeats = ETH3DFeatures<baset>(base_path, eth3d_locations[0]);
    bfeats.imageFeatures(bimagePoints);
    bfeats.worldFeatures(bobjectPoints);
    auto bgtpose = bfeats.getGroundTruthPose();
    vector<baset> binitialGuess = {0, 0, 0, 0, 0, 0};
    //vector<float> initialGuess = {res.first[0], res.first[1], res.first[2],
    //        res.second[0], res.second[1], res.second[2]};
    //uint32_t numPts = imagePoints.size();
    //MSG("Found %d points\n", numPts);
    //for (int i=0; i<5; i++) {
        //cout << "Point "<<i<<" 2d "<<imagePoints[i].x<<" "<<imagePoints[i].y<< endl;
        //cout << "Point "<<i<<" 3d "<<objectPoints[i].x<<" "<<objectPoints[i].y<<" "<<objectPoints[i].z <<endl;
    //}
    printVector("Ground Truth Pose [rotation; translation]: ", &fgtpose[0], 6);


    int maxPoints=fimagePoints.size();

    //for (int i=6; i<maxPoints; i++) {
    for (int i=6; i<276; i++) { // gn breaks after 276
        vector<cv::Point_<float>> fimagePointsSubset(fimagePoints.begin(), fimagePoints.begin() + i);
        vector<cv::Point3_<float>> fobjectPointsSubset(fobjectPoints.begin(), fobjectPoints.begin() + i);
        vector<cv::Point_<baset>> bimagePointsSubset(bimagePoints.begin(), bimagePoints.begin() + i);
        vector<cv::Point3_<baset>> bobjectPointsSubset(bobjectPoints.begin(), bobjectPoints.begin() + i);
        cout << "Testing with " << bimagePointsSubset.size() << " points\n";

        {   // requires float, does not work with fixed point
            cout << "\ntesting opencv\n";
            float _cM[] = {ff, 0, fcx,
                           0, ff, fcy,
                           0, 0, 1};
            cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
            cv::Mat distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
            cv::Mat rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
            cv::Mat tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
            CLOCK(opencv);
            TIC(opencv);
            // OpenCV PnP method
            cv::solvePnP(fobjectPointsSubset, fimagePointsSubset, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
            TOC(opencv);
            cout << "opencv result:" << endl;
            cout << rvec << endl;
            cout << tvec << endl << endl;

            float error[6];
            error[0] = fgtpose[0] - rvec.at<float>(0);
            error[1] = fgtpose[1] - rvec.at<float>(1);
            error[2] = fgtpose[2] - rvec.at<float>(2);
            error[3] = fgtpose[3] - tvec.at<float>(0);
            error[4] = fgtpose[4] - tvec.at<float>(1);
            error[5] = fgtpose[5] - tvec.at<float>(2);
            printf("SeNtInAl,xy,%s,opencv_error_vs_numpts,%d,%f\n", __FUNCTION__, i, twonormsq(error, 6));
        }

#ifdef PPL_GN
        {
            cout << "testing gn - cleartext - float\n";
            vector<float> finitialGuessCopy = finitialGuess;
            CLOCK(gn_float_cleartext);
            TIC(gn_float_cleartext);
            gaussNewton<float>(fobjectPointsSubset, fimagePointsSubset, ff, fcx, fcy, &finitialGuessCopy[0], "float");
            TOC(gn_float_cleartext);
            printVector("[rotation; translation]", &finitialGuessCopy[0], 6);

            float error[6];
            error[0] = fgtpose[0] -  finitialGuessCopy[0];
            error[1] = fgtpose[1] -  finitialGuessCopy[1];
            error[2] = fgtpose[2] -  finitialGuessCopy[2];
            error[3] = fgtpose[3] - finitialGuessCopy[3];
            error[4] = fgtpose[4] - finitialGuessCopy[4];
            error[5] = fgtpose[5] - finitialGuessCopy[5];
            printf("SeNtInAl,xy,%s,gn_float_error_vs_numpts,%d,%f\n", __FUNCTION__, i, twonormsq(error, 6));
        }
        {
            cout << "testing gn - cleartext - baset\n";
            vector<baset> binitialGuessCopy = binitialGuess;
            CLOCK(gn_baset_cleartext);
            TIC(gn_baset_cleartext);
            gaussNewton<baset>(bobjectPointsSubset, bimagePointsSubset, bf, bcx, bcy, &binitialGuessCopy[0], "baset");
            TOC(gn_baset_cleartext);
            printVector("[rotation; translation]", &binitialGuessCopy[0], 6);

            baset error[6];
            error[0] = bgtpose[0] -  binitialGuessCopy[0];
            error[1] = bgtpose[1] -  binitialGuessCopy[1];
            error[2] = bgtpose[2] -  binitialGuessCopy[2];
            error[3] = bgtpose[3] - binitialGuessCopy[3];
            error[4] = bgtpose[4] - binitialGuessCopy[4];
            error[5] = bgtpose[5] - binitialGuessCopy[5];
            printf("SeNtInAl,xy,%s,gn_baset_error_vs_numpts,%d,%f\n", __FUNCTION__, i, (double)twonormsq(error, 6));
        }
#endif
#ifdef PPL_LM
        {
            cout << "testing lm - cleartext - float\n";
            vector<float> finitialGuessCopy = finitialGuess;
            CLOCK(lm_float_cleartext);
            TIC(lm_float_cleartext);
            lm<float>(fobjectPointsSubset, fimagePointsSubset, ff, fcx, fcy, &finitialGuessCopy[0]);
            TOC(lm_float_cleartext);
            printVector("[rotation; translation]", &finitialGuessCopy[0], 6);

            float error[6];
            error[0] = fgtpose[0] -  finitialGuessCopy[0];
            error[1] = fgtpose[1] -  finitialGuessCopy[1];
            error[2] = fgtpose[2] -  finitialGuessCopy[2];
            error[3] = fgtpose[3] - finitialGuessCopy[3];
            error[4] = fgtpose[4] - finitialGuessCopy[4];
            error[5] = fgtpose[5] - finitialGuessCopy[5];
            printf("SeNtInAl,xy,%s,lm_float_error_vs_numpts,%d,%f\n", __FUNCTION__, i, twonormsq(error, 6));
        }
        {
            cout << "testing lm - cleartext - baset\n";
            vector<baset> binitialGuessCopy = binitialGuess;
            CLOCK(lm_baset_cleartext);
            TIC(lm_baset_cleartext);
            lm<baset>(bobjectPointsSubset, bimagePointsSubset, bf, bcx, bcy, &binitialGuessCopy[0]);
            TOC(lm_baset_cleartext);
            printVector("[rotation; translation]", &binitialGuessCopy[0], 6);

            baset error[6];
            error[0] = bgtpose[0] -  binitialGuessCopy[0];
            error[1] = bgtpose[1] -  binitialGuessCopy[1];
            error[2] = bgtpose[2] -  binitialGuessCopy[2];
            error[3] = bgtpose[3] - binitialGuessCopy[3];
            error[4] = bgtpose[4] - binitialGuessCopy[4];
            error[5] = bgtpose[5] - binitialGuessCopy[5];
            printf("SeNtInAl,xy,%s,lm_baset_error_vs_numpts,%d,%f\n", __FUNCTION__, i, (double)twonormsq(error, 6));
        }
#endif
        cout << "\n\n";
    }

    return 0;
}
