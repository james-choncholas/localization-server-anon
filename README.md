# Secure Localization Prototype
Contains opencv, plain c, MPC (ABY/EMP), and VIP-Bench implementations.

# TODO
fix aby trig circuits


# Install instructions
Tested on Ubuntu 20.04.

ABY Dependencies:
sudo apt install g++ make cmake libgmp-dev libssl-dev libboost-all-dev

EMP Dependencies:
sudo apt install software-properties-common cmake git build-essential libssl-dev

Build the secure localization repo
```
git submodule update --init --recursive
mkdir build && cd build
cmake ..
make
```

For testing, download eth3d [dataset](https://www.eth3d.net/datasets#high-res-multi-view)
Use the high-res multi-view undistorted jpg images and ground truth scan evaluation.
Extract to data-eth3d directory.


# Run client server test
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


# Run server-side only benchmarking
To run a benchmark using eth3d data run the follwoing in two separate terminals:
```
cd build/bin
./<gn or lm>_<aby or emp>_<float or int>_server_benchmark 0 1 6
```
e.g. `./gn_aby_float_server_benchmark`

```
cd build/bin
./<gn or lm>_<aby or emp>_<float or int>_server_benchmark 1 1 6
```


# Run tests
In two terminals:
```
cd build/bin
./<gn or lm>_<aby or emp>_<float or int>_server_tester 0
```
e.g. `./gn_aby_float_server_tester 0`

```
cd build/bin
./<gn or lm>_<aby or emp>_<float or int>_server_tester 1
```


# ABY Notes
Note - circuits cannot be built on the fly. must be fully specified then executed.
This means if control flow requires some secret data, circuit must be broken and
intermediate ciphertext stored as secret share.
The ABY Debug print functions do not make this easier as they do in EMP.
[Developer Guide](https://www.informatik.tu-darmstadt.de/media/encrypto/encrypto_code/abydevguide.pdf)

[Reusing Computation](https://github.com/encryptogroup/ABY/issues/167)
