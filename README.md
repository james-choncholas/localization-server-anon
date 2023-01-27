# Secure Localization Prototype
Contains opencv, plain c, and MPC (ABY/EMP).

# Docker
TODO

# Build From Source
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

For testing, download eth3d [dataset](https://www.eth3d.net/datasets#high-res-multi-view)
Use the high-res multi-view undistorted jpg images and ground truth scan evaluation.
Extract to data-eth3d directory:
```
sudo apt-get install p7zip-full
mkdir ./data-eth3d
curl https://www.eth3d.net/data/multi_view_training_dslr_undistorted.7z -o im.7z
7z x im.7z -o./data-eth3d/
rm im.7z
curl https://www.eth3d.net/data/multi_view_training_dslr_scan_eval.7z -o gt.7z
7z x gt.7z -o./data-eth3d/
rm gt.7z
```


## Run client server test
Uses eth3d dataset. Using three separate terminals run:
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


## Run server-side only benchmarking
To run a benchmark using eth3d data run the follwoing in two separate terminals:
```
cd build/bin
./emp_float_server_benchmark lm 100 1 6
./aby_float_server_benchmark lm 100 1 6
```

# ABY Notes
Note - circuits cannot be built on the fly. must be fully specified then executed.
This means if control flow requires some secret data, circuit must be broken and
intermediate ciphertext stored as secret share.
[Developer Guide](https://www.informatik.tu-darmstadt.de/media/encrypto/encrypto_code/abydevguide.pdf)
[Reusing Computation](https://github.com/encryptogroup/ABY/issues/167)
