//#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <jlog.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <random>
#include <string>

#include <gaussnewtonlocalization.h>
#include <lmlocalization.h>
#include <printutil.h>
#include <util.h>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <cleartext-ref/lmlocalization.hpp>
#include <eth3d_features.hpp>

// to get path name
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define NUM_TRIALS 3

void localize(NetIO* io, int party, cv::Mat rvec, cv::Mat tvec,
              cv::Mat cameraMatrix, vector<cv::Point3f> objectPoints,
              vector<cv::Point2f> imagePoints, auto secure_localize_func,
              vector<float>& res) {
  setup_semi_honest(io, party);

  float rt[6] = {rvec.at<float>(0), rvec.at<float>(1), rvec.at<float>(2),
                 tvec.at<float>(0), tvec.at<float>(1), tvec.at<float>(2)};
  float f = cameraMatrix.at<float>(0, 0);
  float cx = cameraMatrix.at<float>(0, 2);
  float cy = cameraMatrix.at<float>(1, 2);

  int numPts = objectPoints.size();
  Float* sobjectPoints =
      static_cast<Float*>(operator new[](4 * numPts * sizeof(Float)));
  Float* simagePoints =
      static_cast<Float*>(operator new[](3 * numPts * sizeof(Float)));
  for (int i = 0; i < numPts; i++) {
    // [x1, x2... ; y1, y2... ; z1, z2...]
    sobjectPoints[i] = Float(objectPoints[i].x, ALICE);
    sobjectPoints[numPts + i] = Float(objectPoints[i].y, ALICE);
    sobjectPoints[2 * numPts + i] = Float(objectPoints[i].z, ALICE);
    // sobjectPoints[3*numPts + i] = Float(1.0, PUBLIC);

    // [x1, y1; x2, y2; ...]
    simagePoints[2 * i] = Float(imagePoints[i].x, ALICE);
    simagePoints[2 * i + 1] = Float(imagePoints[i].y, ALICE);
    // simagePoints[2*numPts + i] = Float(1.0, PUBLIC);
  }

  Float sf = Float(f, ALICE);
  Float scx = Float(cx, ALICE);
  Float scy = Float(cy, ALICE);

  Float* sx = static_cast<Float*>(operator new[](6 * sizeof(Float)));
  for (int i = 0; i < 3; i++) {
    sx[i] = Float(rvec.at<float>(i), ALICE);
    sx[i + 3] = Float(tvec.at<float>(i), ALICE);
  }

  auto sres = secure_localize_func(sobjectPoints, simagePoints, numPts, sf, scx,
                                   scy, sx);
  cout << "secure\n";
  printFloatVector(sx, 6, party == BOB);

  for (int i = 0; i < 6; i++) {
    res[i] = sx[i].reveal<double>();
  }

  delete[] sx;
  delete[] sobjectPoints;
  delete[] simagePoints;

  finalize_semi_honest();
}

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
  NetIO* io = new NetIO(party == ALICE ? nullptr : "127.0.0.1", port);

  vector<cv::Point2f> imagePoints;
  vector<cv::Point3f> objectPoints;

  // ETH3D setup
  float f = 3408.57;
  float cx = 3114.7;
  float cy = 2070.92;
  char datadir[PATH_MAX];
  strncpy(datadir, argv[0], sizeof(datadir));
  dirname(datadir);
  std::string bindir(datadir);
  std::string base_path = bindir + "/../../data-eth3d/";

  //for (int l = 0; l < 1; l++) {
  for (int l = 0; l < eth3d_locations.size(); l++) {
    auto feats = ETH3DFeatures<float>(base_path, eth3d_locations[l]);

    int test_num_frames = MIN(feats.numberOfFrames(), num_frames);

    for (int frame = 0; frame < test_num_frames; frame++) {
      imagePoints.clear();
      objectPoints.clear();
      feats.imageFeatures(imagePoints);
      feats.worldFeatures(objectPoints);
      auto gtpose = feats.getGroundTruthPose();
      vector<float> initialGuess = {0, 0, 0, 0, 0, 0};
      //vector<float> initialGuess = {gtpose.first[0], gtpose.first[1], gtpose.first[2],
      //        gtpose.second[0], gtpose.second[1], gtpose.second[2]};
      //uint32_t numPts = imagePoints.size();
      //MSG("Found %d points\n", numPts);
      //for (int i=0; i<imagePoints.size(); i++) {
      //    MSG("Point %d 2d %f, %f\n", i, imagePoints[i].x, imagePoints[i].y);
      //    MSG("Point %d 3d %f, %f, %f\n", i, objectPoints[i].x, objectPoints[i].y, objectPoints[i].z);
      //}
      printVector("Ground Truth Pose [rotation; translation]: ", &gtpose[0], 6);

      // General Setup
      float _cM[] = {f, 0, cx, 0, f, cy, 0, 0, 1};
      cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
      cv::Mat distCoeffs = cv::Mat::zeros(4, 1, cv::DataType<float>::type);
      cv::Mat rvec(3, 1, cv::DataType<float>::type);
      cv::Mat tvec(3, 1, cv::DataType<float>::type);

      int max_image_points = imagePoints.size();
      int test_num_pts = MIN(max_image_points, max_num_pts);

      for (int i = 6; i < test_num_pts;
           i += MAX((test_num_pts - 6) / 5, 1)) {  // at most 5 intervals
        for (int t = 0; t < NUM_TRIALS; t++) {
          cout << "Testing with " << i << " randomly selected points on frame "
               << frame << " from location " << l << "\n";

          std::random_device dev;
          std::mt19937 rng(dev());
          std::uniform_int_distribution<std::mt19937::result_type> dist6(
              0, max_image_points);

          // randomly sample i feature pairs from total
          vector<cv::Point2f> imagePointsSubset;
          vector<cv::Point3f> objectPointsSubset;
          for (int p = 0; p < i; p++) {
            int r = dist6(rng);
            imagePointsSubset.push_back(imagePoints[r]);
            objectPointsSubset.push_back(objectPoints[r]);
          }

          //{
          //    cout << "\ntesting opencv\n";
          //    distCoeffs = cv::Mat::zeros(4,1,cv::DataType<float>::type);
          //    rvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
          //    tvec = cv::Mat::zeros(3,1,cv::DataType<float>::type);
          //    CLOCK(opencv);
          //    TIC(opencv);
          //    // OpenCV PnP method
          //    cv::solvePnP(objectPointsSubset, imagePointsSubset, cameraMatrix, distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
          //    TOC(opencv);
          //    cout << "opencv result:" << endl;
          //    cout << rvec << endl;
          //    cout << tvec << endl << endl;
          //}

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
            vector<float> res = initialGuess;
#if PPL_FLOW == PPL_FLOW_DO
            WALL_CLOCK(emp_float_gn_time_vs_points_dataobl);
            WALL_TIC(emp_float_gn_time_vs_points_dataobl);
#else
            WALL_CLOCK(emp_float_gn_time_vs_points);
            WALL_TIC(emp_float_gn_time_vs_points);
#endif
            localize(io, party, rvec, tvec, cameraMatrix, objectPointsSubset,
                     imagePointsSubset, BuildGaussNewton, res);
#if PPL_FLOW == PPL_FLOW_DO
            WALL_TOC_CSV_XY(emp_float_gn_time_vs_points_dataobl, i);
#else
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER) {
              WALL_TOC_CSV_XY(emp_float_gn_time_vs_points, i);
            } else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);
#endif
            printVector("[rotation; translation]", &res[0], 6);
          }
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "gn_additions", i, num_additions);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "gn_subtractions", i, num_subtractions);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "gn_multiplications", i, num_multiplications);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "gn_divisions", i, num_divisions);
          MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "gn_bytes_tx", i,
              io->size_tx);
          MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "gn_bytes_rx", i,
              io->size_rx);

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
            vector<float> res = initialGuess;
#if PPL_FLOW == PPL_FLOW_DO
            WALL_CLOCK(emp_float_lm_time_vs_points_dataobl);
            WALL_TIC(emp_float_lm_time_vs_points_dataobl);
#else
            WALL_CLOCK(emp_float_lm_time_vs_points);
            WALL_TIC(emp_float_lm_time_vs_points);
#endif
            localize(io, party, rvec, tvec, cameraMatrix, objectPointsSubset,
                     imagePointsSubset, BuildLM, res);
#if PPL_FLOW == PPL_FLOW_DO
            WALL_TOC_CSV_XY(emp_float_lm_time_vs_points_dataobl, i);
#else
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER) {
              WALL_TOC_CSV_XY(emp_float_lm_time_vs_points, i);
            } else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);
#endif
            printVector("[rotation; translation]", &res[0], 6);
          }
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "lm_additions", i, num_additions);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "lm_subtractions", i, num_subtractions);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "lm_multiplications", i, num_multiplications);
          MSG("SeNtInAl,grouped_bar,%s,%s,%d,%d\n", __FUNCTION__,
              "lm_divisions", i, num_divisions);
          MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "lm_bytes_tx", i,
              io->size_tx);
          MSG("SeNtInAl,xy,%s,%s,%d,%lu\n", __FUNCTION__, "lm_bytes_rx", i,
              io->size_rx);
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
