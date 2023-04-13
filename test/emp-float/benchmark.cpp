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

#include <libgen.h>        // dirname
#include <linux/limits.h>  // PATH_MAX
#include <unistd.h>        // readlink

#include <gaussnewtonlocalization.h>
#include <lmlocalization.h>
#include <printutil.h>
#include <util.h>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <cleartext-ref/lmlocalization.hpp>
#include <eth3d_features.hpp>

using std::cout;
using std::vector;

constexpr const int port = 8080;
constexpr const int seed = 0x666;
const static constexpr float localization_tol_abs = 0.05f;
const static constexpr float localization_tol_rel = 0.05f;

bool withinRel(float v, float t) {
  return fabs(v - t) <= std::max(localization_tol_rel,
                                 fabs(localization_tol_rel * std::max(v, t)));
}

int eth3d_localize(NetIO* io, int party, int num_frames, int num_trials,
                   int max_num_pts, auto cleartext_localize_func,
                   auto secure_localize_func, bool silent,
                   std::string log_str) {
  setup_semi_honest(io, party);
  uint32_t cv_successes = 0;
  uint32_t cleartext_successes = 0;
  uint32_t secure_successes = 0;
  uint32_t total_runs = 0;

  float f = 3408.57;
  float cx = 3114.7;
  float cy = 2070.92;
  float _cM[] = {f, 0, cx, 0, f, cy, 0, 0, 1};
  cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
  cv::Mat distCoeffs = cv::Mat::zeros(4, 1, cv::DataType<float>::type);
  cv::Mat rvec = cv::Mat::zeros(3, 1, cv::DataType<float>::type);
  cv::Mat tvec = cv::Mat::zeros(3, 1, cv::DataType<float>::type);
  vector<cv::Point2f> imagePoints;
  vector<cv::Point3f> objectPoints;

  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  const char* path;
  if (count != -1) {
    path = dirname(result);
  } else {
    error("cant find path\n");
    return 1;
  }
  std::string base_path = string(path) + "/../../data-eth3d/";

  for (uint32_t l = 0; l < eth3d_locations.size(); l++) {
    // for (int l=0; l<1; l++) {
    auto feats = ETH3DFeatures<float>(base_path, eth3d_locations[l]);

    int test_num_frames = MIN(feats.numberOfFrames(), num_frames);

    for (int frame = 0; frame < test_num_frames; frame++) {
      imagePoints.clear();
      objectPoints.clear();
      feats.imageFeatures(imagePoints);
      feats.worldFeatures(objectPoints);
      auto gtpose = feats.getGroundTruthPose();
      vector<float> initialGuess = {0, 0, 0, 0, 0, 0};
      // vector<float> initialGuess = {res.first[0], res.first[1], res.first[2],
      //         res.second[0], res.second[1], res.second[2]};
      // uint32_t numPts = imagePoints.size();
      // MSG("Found %d points\n", numPts);
      // for (int i=0; i<imagePoints.size(); i++) {
      //     MSG("Point %d 2d %f, %f\n", i, imagePoints[i].x, imagePoints[i].y);
      //     MSG("Point %d 3d %f, %f, %f\n", i, objectPoints[i].x,
      //     objectPoints[i].y, objectPoints[i].z);
      // }
      if (!silent) {
        printVector("Ground Truth Pose [rotation; translation]: ", &gtpose[0],
                    6);
      }

      int max_image_points = imagePoints.size();
      int test_num_pts = MIN(max_image_points, max_num_pts);

      // std::random_device dev;
      // std::mt19937 rng(dev());
      std::mt19937 rng(seed);
      std::uniform_int_distribution<std::mt19937::result_type> dist(
          0, max_image_points);

      for (int num_pts = 6; num_pts <= test_num_pts;
           num_pts += MAX((test_num_pts - 6) / 5, 1)) {  // at most 5 intervals
        for (int t = 0; t < num_trials; t++) {
          if (!silent) {
            cout << "Trial " << t << " with " << num_pts
                 << " randomly selected points on frame " << frame
                 << " from location " << l << "\n";
          }

          // randomly sample i feature pairs from total
          vector<cv::Point2f> imagePointsSubset;
          vector<cv::Point3f> objectPointsSubset;
          for (int p = 0; p < num_pts; p++) {
            int r = dist(rng);
            imagePointsSubset.push_back(imagePoints[r]);
            objectPointsSubset.push_back(objectPoints[r]);
          }

          {
            if (!silent) {
              cout << "\ntesting opencv\n";
            }
            distCoeffs = cv::Mat::zeros(4, 1, cv::DataType<float>::type);
            rvec = cv::Mat::zeros(3, 1, cv::DataType<float>::type);
            tvec = cv::Mat::zeros(3, 1, cv::DataType<float>::type);
            CLOCK(opencv);
            TIC(opencv);
            // OpenCV PnP method
            cv::solvePnP(objectPointsSubset, imagePointsSubset, cameraMatrix,
                         distCoeffs, rvec, tvec, false, cv::SOLVEPNP_ITERATIVE);
            TOC(opencv);
            if (!silent) {
              cout << "opencv result:" << endl;
              cout << rvec << endl;
              cout << tvec << endl << endl;
            }
            bool good = true;
            for (int i = 0; i < 3; ++i) {
              good &= withinRel(rvec.at<float>(i), gtpose[i]);
              good &= withinRel(tvec.at<float>(i), gtpose[i + 3]);
            }
            if (good) {
              cout << "converged!\n";
              ++cv_successes;
            } else {
              cout << "opencv did not converge. skip this iteration\n";
              break;
            }
          }

          {
            if (!silent) {
              cout << "testing cleartext\n";
            }
            vector<float> initialGuessCopy = initialGuess;
            CLOCK(cleartext);
            TIC(cleartext);
            cleartext_localize_func(objectPoints, imagePoints, f, cx, cy,
                                    &initialGuessCopy[0], "");
            TOC(cleartext);
            if (!silent) {
              printVector("cleartext result:\n", &initialGuessCopy[0], 6);
            }
            bool good = true;
            for (int i = 0; i < 6; ++i) {
              good &= withinRel(initialGuessCopy[i], gtpose[i]);
            }
            if (good) {
              cout << "converged!\n";
              ++cleartext_successes;
            }
          }

          {
            if (!silent) {
              cout << "testing secure\n";
            }
            vector<float> initialGuessCopy = initialGuess;
            time_point<high_resolution_clock> tic =
                high_resolution_clock::now();

            Float* sobjectPoints = static_cast<Float*>(operator new[](
                4 * num_pts * sizeof(Float)));
            Float* simagePoints = static_cast<Float*>(operator new[](
                3 * num_pts * sizeof(Float)));
            for (int i = 0; i < num_pts; i++) {
              // [x1, x2... ; y1, y2... ; z1, z2...]
              sobjectPoints[i] = Float(objectPointsSubset[i].x, ALICE);
              sobjectPoints[num_pts + i] =
                  Float(objectPointsSubset[i].y, ALICE);
              sobjectPoints[2 * num_pts + i] =
                  Float(objectPointsSubset[i].z, ALICE);
              // sobjectPoints[3*num_pts + i] = Float(1.0, PUBLIC);

              // [x1, y1; x2, y2; ...]
              simagePoints[2 * i] = Float(imagePointsSubset[i].x, ALICE);
              simagePoints[2 * i + 1] = Float(imagePointsSubset[i].y, ALICE);
              // simagePoints[2*num_pts + i] = Float(1.0, PUBLIC);
            }

            Float sf = Float(f, ALICE);
            Float scx = Float(cx, ALICE);
            Float scy = Float(cy, ALICE);

            Float* sx = static_cast<Float*>(operator new[](6 * sizeof(Float)));
            for (int i = 0; i < 6; i++) {
              sx[i] = Float(initialGuessCopy[i], ALICE);
            }

            auto [converged, num_loc_iterations] = secure_localize_func(
                sobjectPoints, simagePoints, num_pts, sf, scx, scy, sx);

            float sres[6];
            for (int i = 0; i < 6; i++) {
              sres[i] = sx[i].reveal<double>();
            }

            delete[] sobjectPoints;
            delete[] simagePoints;
            delete[] sx;

            bool good = true;
            for (int i = 0; i < 6; ++i) {
              good &= withinRel(sres[i], gtpose[i]);
            }

            if (!silent) {
              if (good) {
                cout << "converged!\n";
                std::string timer_name =
                    "emp_float_" + log_str + "_time_vs_points";
#if PPL_FLOW == PPL_FLOW_DO
                timer_name += "_dataobl";
#endif
                time_point<high_resolution_clock> toc =
                    high_resolution_clock::now();
                MSG("SeNtInAl,xy,%s,%s,%d,%g\n", __FUNCTION__,
                    timer_name.c_str(), num_pts,
                    std::chrono::duration_cast<std::chrono::microseconds>(toc -
                                                                          tic)
                            .count() /
                        1000000.0);
                MSG("SeNtInAl,xy,%s,%s,%d,%g\n", __FUNCTION__,
                    (timer_name + "_per_loc_itr").c_str(), num_pts,
                    std::chrono::duration_cast<std::chrono::microseconds>(toc -
                                                                          tic)
                            .count() /
                        (1000000.0 * num_loc_iterations));

                ++secure_successes;
                MSG("SeNtInAl,grouped_bar,%s,%s%s,%d,%d\n", __FUNCTION__,
                    log_str.c_str(), "_additions", num_pts, num_additions);
                MSG("SeNtInAl,grouped_bar,%s,%s%s,%d,%d\n", __FUNCTION__,
                    log_str.c_str(), "_subtractions", num_pts,
                    num_subtractions);
                MSG("SeNtInAl,grouped_bar,%s,%s%s,%d,%d\n", __FUNCTION__,
                    log_str.c_str(), "_multiplications", num_pts,
                    num_multiplications);
                MSG("SeNtInAl,grouped_bar,%s,%s%s,%d,%d\n", __FUNCTION__,
                    log_str.c_str(), "_divisions", num_pts, num_divisions);
                MSG("SeNtInAl,xy,%s,%s%s,%d,%lu\n", __FUNCTION__,
                    log_str.c_str(), "_bytes_tx", num_pts, io->size_tx);
                MSG("SeNtInAl,xy,%s,%s%s,%d,%lu\n", __FUNCTION__,
                    log_str.c_str(), "_bytes_rx", num_pts, io->size_rx);
              } else {
                MSG("Not printing timing - not converged.\n");
              }
              printVector("secure result:\n", &sres[0], 6);
            }
          }
          // reset stats
          num_additions = 0;
          num_subtractions = 0;
          num_multiplications = 0;
          num_divisions = 0;
          io->num_tx = 0;
          io->num_rx = 0;
          io->size_tx = 0;
          io->size_rx = 0;
          if (!silent) {
            cout << "\n\n";
          }
          ++total_runs;
        }
      }
      feats.nextFrame();
    }
  }
  if (!silent) {
    MSG("SeNtInAl,xy,%s,%s,%d,%u\n", __FUNCTION__, "cv_successes", 0,
        cv_successes);
    MSG("SeNtInAl,xy,%s,%s,%d,%u\n", __FUNCTION__, "cleartext_successes", 0,
        cleartext_successes);
    MSG("SeNtInAl,xy,%s,%s,%d,%u\n", __FUNCTION__, "secure_successes", 0,
        secure_successes);
    MSG("SeNtInAl,xy,%s,%s,%d,%u\n", __FUNCTION__, "total_runs", 0, total_runs);
  }
  finalize_semi_honest();
  return 0;
}

int main(int argc, char** argv) {
  if (argc != 5) {
    MSG("Usage: %s <lm or gn> <num frames> <num trials> <max num pts>\n",
        argv[0]);
    return 1;
  }
  int num_frames = atoi(argv[2]);
  int num_trials = atoi(argv[3]);
  int max_num_pts = atoi(argv[4]);

  std::string lm_str("lm");
  auto clear_func = lm_str == argv[1] ? lm<float> : gaussNewton<float>;
  auto secure_func = lm_str == argv[1] ? BuildLM : BuildGaussNewton;
  auto log_str = lm_str == argv[1] ? "lm" : "gn";

  std::thread bob([&]() {
    NetIO* io = new NetIO("127.0.0.1", port);
    eth3d_localize(io, BOB, num_frames, num_trials, max_num_pts, clear_func,
                   secure_func, true, log_str);
    delete io;
  });

  NetIO* io = new NetIO(nullptr, port);
  eth3d_localize(io, ALICE, num_frames, num_trials, max_num_pts, clear_func,
                 secure_func, false, log_str);
  bob.join();
  delete io;
}
