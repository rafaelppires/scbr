######## SGX SDK Settings ########
CXX=g++-5
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
#SGX_MODE ?= SIM
#SGX_DEBUG ?= 1
SGX_PRERELEASE ?=1
SGX_ARCH ?= x64
SCBR_SRCS=../../src
CBRPREFILTER_DIR=../../cbr-prefilter
COMMON_SGX=../../../sgx_common

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g
else
        SGX_COMMON_CFLAGS += -O2
endif

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

Crypto_Library_Name := sgx_tcrypto

CBR_Enclave_Filter_Cpp_Files := trusted/CBR_Enclave_Filter.cpp \
                                $(addprefix $(CBRPREFILTER_DIR)/, \
                                            cbr.cpp graph.cpp util.cpp \
                                            subscription.cpp) 
CBR_Enclave_Filter_C_Files := 
CBR_Enclave_Filter_Include_Paths := -IInclude -Itrusted -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport


Flags_Just_For_C := -Wno-implicit-function-declaration -std=c11
Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(CBR_Enclave_Filter_Include_Paths) -fno-builtin-printf -I.
CBR_Enclave_Filter_C_Flags := $(Flags_Just_For_C) $(Common_C_Cpp_Flags)
CBR_Enclave_Filter_Cpp_Flags :=  $(Common_C_Cpp_Flags) -std=c++11 -nostdinc++ -fno-builtin-printf -I$(CBRPREFILTER_DIR) -I$(COMMON_SGX) -DENCLAVESGX

CBR_Enclave_Filter_Cpp_Flags := $(CBR_Enclave_Filter_Cpp_Flags)  -fno-builtin-printf -DENCLAVED

CBR_Enclave_Filter_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=trusted/CBR_Enclave_Filter.lds

CBR_Enclave_Filter_Cpp_Objects := $(CBR_Enclave_Filter_Cpp_Files:.cpp=.o)
CBR_Enclave_Filter_C_Objects := $(CBR_Enclave_Filter_C_Files:.c=.o)

ifeq ($(EXTRAARGS), plain)
        CBR_Enclave_Filter_Cpp_Flags += -DPLAINTEXT_MATCHING
endif

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif


.PHONY: all run

ifeq ($(Build_Mode), HW_RELEASE)
all: CBR_Enclave_Filter.so
	@echo "Build enclave CBR_Enclave_Filter.so  [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the CBR_Enclave_Filter.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo 


else
all: CBR_Enclave_Filter.signed.so
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/app
	@echo "RUN  =>  app [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif


######## CBR_Enclave_Filter Objects ########

trusted/CBR_Enclave_Filter_t.c: $(SGX_EDGER8R) ./trusted/CBR_Enclave_Filter.edl
	@cd ./trusted && $(SGX_EDGER8R) --trusted ../trusted/CBR_Enclave_Filter.edl --search-path ../trusted --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

trusted/CBR_Enclave_Filter_t.o: ./trusted/CBR_Enclave_Filter_t.c
	@$(CC) $(CBR_Enclave_Filter_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

trusted/%.o: trusted/%.cpp
	@$(CXX) $(CBR_Enclave_Filter_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

trusted/%.o: $(COMMON_SGX)/%.cpp
	@$(CXX) $(CBR_Enclave_Filter_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(CBRPREFILTER_DIR)/%.o: $(CBRPREFILTER_DIR)/%.cc
	@$(CXX) $(CBR_Enclave_Filter_Cpp_Flags) -c $< -o $@ 
	@echo "CXX  <=  $<"

trusted/%.o: trusted/%.c
	@$(CC) $(CBR_Enclave_Filter_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

CBR_Enclave_Filter.so: trusted/CBR_Enclave_Filter_t.o $(CBR_Enclave_Filter_Cpp_Objects) $(CBR_Enclave_Filter_C_Objects) trusted/sgx_cryptoall.o
	@$(CXX) $^ -o $@ $(CBR_Enclave_Filter_Link_Flags)
	@echo "LINK =>  $@"

CBR_Enclave_Filter.signed.so: CBR_Enclave_Filter.so
	@$(SGX_ENCLAVE_SIGNER) sign -key trusted/CBR_Enclave_Filter_private.pem -enclave CBR_Enclave_Filter.so -out $@ -config trusted/CBR_Enclave_Filter.config.xml
	@echo "SIGN =>  $@"
clean:
	@rm -f CBR_Enclave_Filter.* trusted/CBR_Enclave_Filter_t.* $(CBR_Enclave_Filter_Cpp_Objects) $(CBR_Enclave_Filter_C_Objects)
