## SCBR 

Secure Content-Based Routing: pub/sub with filtering phase inside SGX enclaves

[More info](https://arxiv.org/abs/1701.04612)


# To compile
```
cd sgx
./build-encr-sgx.sh
```

* `build-encr-sgx.sh` : with encryption, with SGX enclaves
* `build-encr-unshield.sh` : with encryption, no SGX enclaves
* `build-plain-sgx.sh` : no encryption, with SGX enclaves
* `build-plain-unshield.sh` : no encryption, no SGX enclaves

# Notes

If one wants to run SGX in simulation mode, the following files need to be changed:
```
sgx/enclave_CBR_Enclave_Filter/sgx_t.mk
sgx/enclave_CBR_Enclave_Filter/sgx_u.mk
```

Modifications in both of them are the same. Instead of:
```
SGX_MODE ?= HW
#SGX_MODE ?= SIM
```

It should be:
```
#SGX_MODE ?= HW
SGX_MODE ?= SIM
```

