# Secure Localization Server
[![tests](https://github.com/secret-snail/localization-server/actions/workflows/cmake.yml/badge.svg)](https://github.com/secret-snail/localization-server/actions/workflows/cmake.yml)
[![container](https://github.com/secret-snail/localization-server/actions/workflows/docker-image.yml/badge.svg)](https://github.com/secret-snail/localization-server/actions/workflows/docker-image.yml)

Privacy preserving localization based on secure multiparty computation (MPC).

## Run the SNaIL
To run turbo the snail, first `docker/build.sh` the container.
Then run `docker/alice.sh` and `docker/bob.sh` in two terminals.
And on the [snail](https://github.com/secret-snail/snail),
run `sudo ./build/bin/visp_snail --secure`.

## Build From Source
ABY Dependencies:
sudo apt install g++ make cmake libgmp-dev libssl-dev libboost-all-dev

EMP Dependencies:
sudo apt install software-properties-common cmake git build-essential libssl-dev

Build and test the code:
```
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
ctest
```

ABY testing requires -DCMAKE_BUILD_TYPE=Release, tests fail when the sanitizers
are turned on because there appears to be memory leaks in the ABY library.

Note - circuits cannot be built on the fly. must be fully specified then executed.
This means if control flow requires some secret data, circuit must be broken and
intermediate ciphertext stored as secret share.
[Developer Guide](https://www.informatik.tu-darmstadt.de/media/encrypto/encrypto_code/abydevguide.pdf)
[Reusing Computation](https://github.com/encryptogroup/ABY/issues/167)


## Experimental Evaluation
Requires the [eth3d dataset](https://www.eth3d.net/datasets#high-res-multi-view)
Use the high-res multi-view undistorted jpg images and ground truth scan evaluation
and extract to data-eth3d directory with the following:
```bash
sudo apt-get install p7zip-full
mkdir ./data-eth3d
curl https://www.eth3d.net/data/multi_view_training_dslr_undistorted.7z -o im.7z
7z x im.7z -o./data-eth3d/
rm im.7z
curl https://www.eth3d.net/data/multi_view_training_dslr_scan_eval.7z -o gt.7z
7z x gt.7z -o./data-eth3d/
rm gt.7z
```

Run the experiments.
```bash
cmake -B ./build -S ./ -DCMAKE_BUILD_TYPE=Release
mkdir -p results

# setup privacy_conf.h for ETH3D, and SiSL
cmake --build ./build
LAT=0msec source scripts/network_setup.sh # outside container
scripts/emp_float_benchmark_run.sh
scripts/aby_float_benchmark_run.sh
scripts/mult_add_fixed_float_time_run.sh

# turn on data oblivious in privacy_conf.h and recompile
cmake --build ./build
LAT=0msec source scripts/network_setup.sh # outside container
scripts/emp_float_benchmark_dataobl_run.sh

# setup privacy_conf.h for ETH3D, and back to SiSL
cmake --build ./build
LAT=5msec source scripts/network_setup.sh # outside container
scripts/emp_float_benchmark_run_latency.sh

source scripts/network_teardown.sh # outside container
```

Plot the data.
```bash
scripts/emp_vs_aby_plot.sh
scripts/loopleak_vs_dataobl_plot.sh
scripts/emp_float_benchmark_plot.sh
scripts/netio_plot.sh
scripts/num_arith_ops_plot.sh
scripts/mult_add_fixed_float_time_plot.sh
```

### Run Client and Server on Separate Machines
Uses ETH3D dataset. Using three separate terminals run:
```
cd build/bin
./emp_float_client
```

```
cd build/bin
./lm_emp_float_server 0
```

```
cd build/bin
./lm_emp_float_server 1
```
