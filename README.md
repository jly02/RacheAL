# RacheCKKS
Rache is a radix-based homomorphic encryption algorithm that utilizes a base scheme, leveraging the fact that in practical usage, homomorphic operations have far lower performance overhead than the encryption itself. Rache uses caching of certain "interesting" pre-computed ciphertexts (_which happen to be powers of a particular number, hence_ **_radices_**) in order to gain performance benefits over naively encrypting new values, using homomorphic operations. 

The [Rache paper](https://dl.acm.org/doi/10.1145/3588920) that premiered in SIGMOD'23 uses two schemes, [Paillier](https://link.springer.com/content/pdf/10.1007/3-540-48910-X_16.pdf) and [Symmetria](https://dl.acm.org/doi/10.14778/3389133.3389144), and shows that great performance benefits can be gained in systems using Rache over purely one or the other. However, these schemes only work on integers. This project aims to make use of the [CKKS](https://eprint.iacr.org/2016/421.pdf) scheme to extend the functionality of Rache to operations over the real numbers.

## Steps to Build
### Dependencies: 
- CMake 3.13+
- A C++ compiler meeting the C++17 standard

### Steps:
1. Clone this repository with
  ```shell
  # Download source code and switch into directory
  $ git clone git@github.com:jly02/RacheCKKS.git
  $ cd RacheCKKS
  ```
2. Run `git submodule init`, and then `git submodule update`. This will install vcpkg, which is required to build this repository.
3. Run `cmake .` to setup the project, and `make` to build.
4. A test executable is provided. To run this, simply use `./bin/RacheCKKS`.