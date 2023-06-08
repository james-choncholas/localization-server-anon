#include <eth3d_features.hpp>
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
#include <util.h>
#include <cleartext-ref/gaussnewtonlocalization.hpp>
#include <cleartext-ref/lmlocalization.hpp>

#include <abycore/aby/abyparty.h>
#include <abycore/circuit/arithmeticcircuits.h>
#include <abycore/circuit/booleancircuits.h>
#include <abycore/circuit/circuit.h>
#include <abycore/sharing/sharing.h>

// to get path name
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define NUM_TRIALS 5
#define SEED 0x666

const std::string address = "127.0.0.1";
constexpr const int port = 7766;
constexpr const uint32_t nthreads = 1;
constexpr const e_mt_gen_alg mt_alg = MT_OT;
const seclvl slvl = LT;  // get_sec_lvl(secparam);
const vector<e_sharing> ctypes = {/*S_ARITH,*/ S_BOOL,
                                  S_YAO};  // no floats with arith
map<int, string> cnames = {
    {S_ARITH, "arith"},
    {S_BOOL, "bool"},
    {S_YAO, "yao"},
};
constexpr const uint32_t reservegates = 65536;
constexpr const uint32_t bitlen = 32;
static std::string get_circuit_dir() {
  char result[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
  const char* path;
  if (count != -1) {
    path = dirname(result);
  } else {
    std::cerr << "cant find circuit path\n";
    assert(false);
  }
  std::string base_path = string(path) + "/../../extern/ABY/bin/circ";
  return base_path;
}

void localize(e_role role, e_sharing sharing, cv::Mat rvec, cv::Mat tvec,
              cv::Mat cameraMatrix, [[maybe_unused]] cv::Mat distCoeffs,
              vector<cv::Point3f> objectPoints, vector<cv::Point2f> imagePoints,
              auto secure_localize_func, vector<float>& res) {
  float f = cameraMatrix.at<float>(0, 0);
  float cx = cameraMatrix.at<float>(0, 2);
  float cy = cameraMatrix.at<float>(1, 2);

  ABYParty* party = new ABYParty(role, address, port, slvl, bitlen, nthreads,
                                 mt_alg, reservegates, get_circuit_dir());
  std::vector<Sharing*>& sharings = party->GetSharings();
  Circuit* circ = sharings[sharing]->GetCircuitBuildRoutine();

#if PPL_FLOW == PPL_FLOW_LOOP_LEAK || PPL_FLOW == PPL_FLOW_SiSL
  //  always use boolean shares (not yao) so BuildAndRunLM can get raw shares.
  //  this is because you cant use Y2B shares on Yao input gates.
  Circuit* bc = sharings[S_BOOL]->GetCircuitBuildRoutine();
#else
  Circuit* bc = circ;
#endif

  assert(objectPoints.size() == imagePoints.size());
  int numPts = objectPoints.size();

  // Allocate space for shares
  share** s_objectPoints = new share*[4 * numPts];
  share** s_imagePoints = new share*[3 * numPts];
  share* s_f;
  share* s_cx;
  share* s_cy;
  share** s_x = new share*[6];

  // Prepare inputs
  if (role == SERVER) {
    // float one=1.0;
    for (int p = 0; p < numPts; p++) {
      s_objectPoints[p] =
          bc->PutINGate((uint32_t*)&objectPoints[p].x, bitlen, role);
    }
    for (int p = 0; p < numPts; p++) {
      s_objectPoints[numPts + p] =
          bc->PutINGate((uint32_t*)&objectPoints[p].y, bitlen, role);
    }
    for (int p = 0; p < numPts; p++) {
      s_objectPoints[(2 * numPts) + p] =
          bc->PutINGate((uint32_t*)&objectPoints[p].z, bitlen, role);
    }
    // for(int p=0; p<numPts; p++) {
    //     s_objectPoints[3*numPts+p] = bc->PutINGate((uint32_t*) &one, bitlen,
    //     role);
    // }
    for (int p = 0; p < numPts; p++) {
      s_imagePoints[p] =
          bc->PutINGate((uint32_t*)&imagePoints[p].x, bitlen, role);
    }
    for (int p = 0; p < numPts; p++) {
      s_imagePoints[numPts + p] =
          bc->PutINGate((uint32_t*)&imagePoints[p].y, bitlen, role);
    }
    // for(int p=0; p<numPts; p++) {
    //     s_imagePoints[2*numPts+p] = bc->PutINGate((uint32_t*) &one, bitlen,
    //     role);
    // }
    s_f = bc->PutINGate((uint32_t*)&f, bitlen, role);
    s_cx = bc->PutINGate((uint32_t*)&cx, bitlen, role);
    s_cy = bc->PutINGate((uint32_t*)&cy, bitlen, role);
    //float zero = 0.0f;
    for (int p = 0; p < 3; p++) {
      s_x[p] = bc->PutINGate((uint32_t*)&rvec.at<float>(p), bitlen, role);
    }
    for (int p = 0; p < 3; p++) {
      s_x[p + 3] = bc->PutINGate((uint32_t*)&tvec.at<float>(p), bitlen, role);
    }
  } else {
    for (int p = 0; p < 3 * numPts; p++) {  // ignore constant 1s
      s_objectPoints[p] = bc->PutDummyINGate(bitlen);
    }
    for (int p = 0; p < 2 * numPts; p++) {  // ignore constant 1s
      s_imagePoints[p] = bc->PutDummyINGate(bitlen);
    }
    s_f = bc->PutDummyINGate(bitlen);
    s_cx = bc->PutDummyINGate(bitlen);
    s_cy = bc->PutDummyINGate(bitlen);
    for (int p = 0; p < 6; p++) {
      s_x[p] = bc->PutDummyINGate(bitlen);
    }
  }

  secure_localize_func(s_objectPoints, s_imagePoints, numPts, s_f, s_cx, s_cy,
                       s_x, (BooleanCircuit*)circ, party, role);

  for (int i = 0; i < 6; i++) {
    share* temp = s_x[i];
    s_x[i] = bc->PutOUTGate(s_x[i], ALL);
    delete temp;
  }

  party->ExecCircuit();

  if (role == SERVER) {
    cout << "secure result: ";
  }
  for (int i = 0; i < 6; i++) {
    uint32_t* output;
    uint32_t out_bitlen, out_nvals;
    s_x[i]->get_clear_value_vec(&output, &out_bitlen, &out_nvals);
    res[i] = *(float*)output;

    if (role == SERVER) {
      cout << *(float*)output << " ";
    }
  }

  delete party;
}

int main(int argc, char** argv) {
  if (argc != 4) {
    MSG("Usage: %s <party number> <num frames> <max num pts>\n", argv[0]);
    return 1;
  }
  e_role role = (e_role)atoi(argv[1]);
  int num_frames = atoi(argv[2]);
  int max_num_pts = atoi(argv[3]);

  MSG("\nStarting ABY...\n");

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

    for (int frame = 0; frame <= test_num_frames; frame++) {
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
      //     //MSG("Point %d 3d %d, %d, %d\n", i, *(int*)&objectPoints[1].x,
      //     *(int*)&objectPoints[1].y,
      //     *(int*)&objectPoints[1].z);
      // }
      printVector("Ground Truth Pose [rotation; translation]: ", &gtpose[0], 6);

      // General Setup
      float _cM[] = {f, 0, cx, 0, f, cy, 0, 0, 1};
      cv::Mat cameraMatrix = cv::Mat(3, 3, cv::DataType<float>::type, _cM);
      cv::Mat distCoeffs = cv::Mat::zeros(4, 1, cv::DataType<float>::type);
      cv::Mat rvec(3, 1, cv::DataType<float>::type);
      cv::Mat tvec(3, 1, cv::DataType<float>::type);

      int max_image_points = imagePoints.size();
      int test_num_pts = MIN(max_image_points, max_num_pts);

      // std::random_device dev;
      // std::mt19937 rng(dev());
      std::mt19937 rng(SEED);
      std::uniform_int_distribution<std::mt19937::result_type> dist(
          0, max_image_points);

      for (int i = 6; i < test_num_pts;
           i += MAX((test_num_pts - 6) / 5, 1)) {  // at most 5 intervals
        for (int t = 0; t < NUM_TRIALS; t++) {
          cout << "Testing with " << i << " randomly selected points on frame "
               << frame << " from location " << l << "\n";

          // randomly sample i feature pairs from total
          vector<cv::Point2f> imagePointsSubset;
          vector<cv::Point3f> objectPointsSubset;
          for (int p = 0; p < i; p++) {
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
          //    cv::solvePnP(objectPointsSubset, imagePointsSubset,
          //    cameraMatrix, distCoeffs, rvec, tvec, false,
          //    cv::SOLVEPNP_ITERATIVE); TOC(opencv); cout << "opencv result:"
          //    << endl; cout << rvec << endl; cout << tvec << endl << endl;
          //}

#ifdef PPL_GN
          //{
          //    cout << "testing gauss newton - cleartext\n";
          //    vector<float> initialGuessCopy = initialGuess;
          //    CLOCK(cleartext);
          //    TIC(cleartext);
          //    gaussNewton<float>( objectPointsSubset, imagePointsSubset, f,
          //    cx, cy, &initialGuessCopy[0]); TOC(cleartext);
          //    printVector("[rotation; translation]", &initialGuessCopy[0], 6);
          //}

          {
            cout << "testing gauss newton - " << cnames[S_BOOL] << "\n";
            vector<float> res = initialGuess;
            WALL_CLOCK(aby_bool_float_gn);
            WALL_TIC(aby_bool_float_gn);
            localize(role, S_BOOL, rvec, tvec, cameraMatrix, distCoeffs,
                     objectPointsSubset, imagePointsSubset,
                     BuildAndRunGaussNewton, res);
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER) {
              WALL_TOC_CSV_XY(aby_bool_float_gn, i);
              PrintSumTimings(objectPoints.size());
            } else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);

            printVector("[rotation; translation]", &res[0], 6);
            ClearSumTimings();
          }

          {
            cout << "testing gauss newton - " << cnames[S_YAO] << "\n";
            vector<float> res = initialGuess;
            WALL_CLOCK(aby_yao_float_gn);
            WALL_TIC(aby_yao_float_gn);
            localize(role, S_YAO, rvec, tvec, cameraMatrix, distCoeffs,
                     objectPointsSubset, imagePointsSubset,
                     BuildAndRunGaussNewton, res);
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER) {
              WALL_TOC_CSV_XY(aby_yao_float_gn, i);
              PrintSumTimings(objectPoints.size());
            } else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);
            printVector("[rotation; translation]", &res[0], 6);
            ClearSumTimings();
          }
#endif
#ifdef PPL_LM
          //{
          //    cout << "testing lm - cleartext\n";
          //    vector<float> initialGuessCopy = initialGuess;
          //    CLOCK(cleartext);
          //    TIC(cleartext);
          //    lm<float>( objectPointsSubset, imagePointsSubset, f, cx, cy,
          //    &initialGuessCopy[0]); TOC(cleartext); printVector("[rotation;
          //    translation]", &initialGuessCopy[0], 6);
          //}

          {
            cout << "testing lm - " << cnames[S_BOOL] << "\n";
            vector<float> res = initialGuess;
            WALL_CLOCK(aby_bool_float_lm);
            WALL_TIC(aby_bool_float_lm);
            localize(role, S_BOOL, rvec, tvec, cameraMatrix, distCoeffs,
                     objectPointsSubset, imagePointsSubset, BuildAndRunLM, res);
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER)
              WALL_TOC_CSV_XY(aby_bool_float_lm, i);
            else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);
            printVector("[rotation; translation]", &res[0], 6);
          }
          {
            cout << "testing lm - " << cnames[S_YAO] << "\n";
            vector<float> res = initialGuess;
            WALL_CLOCK(aby_yao_float_lm);
            WALL_TIC(aby_yao_float_lm);
            localize(role, S_YAO, rvec, tvec, cameraMatrix, distCoeffs,
                     objectPointsSubset, imagePointsSubset, BuildAndRunLM, res);
            float error[6];
            for (int e = 0; e < 6; e++) {
              error[e] = res[e] - gtpose[e];
            }
            float norm = twonormsq(error, 6);
            if (norm < GT_MIN_ER)
              WALL_TOC_CSV_XY(aby_yao_float_lm, i);
            else
              MSG("Not printing timing - not converged. Pose error norm: "
                  "%f\n",
                  norm);
            printVector("[rotation; translation]", &res[0], 6);
          }
#endif
        }
      }
      feats.nextFrame();
    }
  }

  return 0;
}
