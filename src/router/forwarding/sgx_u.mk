######## SGX SDK Settings ########
CXX=g++-5
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
#SGX_MODE ?= SIM
#SGX_DEBUG ?= 1
SGX_PRERELEASE ?= 1
SGX_ARCH ?= x64
UNTRUSTED_DIR=untrusted
SCBR_SRCS=../glue
SGXCOMM_DIR = ../../../../sgx_common/
CBRPREFILTER_DIR=../matching
USERLIBDIR=../../user-library
TARGET=scbr

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

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

# App_Cpp_Files := App/App.cpp $(wildcard App/Edger8rSyntax/*.cpp) $(wildcard App/TrustedLibrary/*.cpp)
App_Cpp_Files := $(UNTRUSTED_DIR)/$(TARGET).cpp \
                 $(addprefix $(SGXCOMM_DIR)/, sgx_initenclave.cpp sgx_errlist.cpp sgx_cryptoall.cpp sgx_utils_rp.cpp) \
                 $(addprefix $(SCBR_SRCS)/, pubsubco.cpp) \
                 $(addprefix $(CBRPREFILTER_DIR)/, cbr.cpp prefilter.cpp \
                             graph.cpp util.cpp event.cpp subscription.cpp) \
                             # $(wildcard App/TrustedLibrary/*.cpp)
App_Include_Paths := -IInclude -I$(UNTRUSTED_DIR) -I$(SGX_SDK)/include

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11 -I$(SCBR_SRCS) -I$(SGXCOMM_DIR) -I$(CBRPREFILTER_DIR) -I$(USERLIBDIR)/include
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread 

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif
App_Link_Flags += -lcryptopp -lzmq -lboost_system -lboost_chrono

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

ifeq ($(ARGS), noenclave)
        App_Cpp_Flags += -DNONENCLAVE_MATCHING -Itrusted
        SamplePreReq= $(App_Cpp_Objects)
else
        SamplePreReq= $(UNTRUSTED_DIR)/CBR_Enclave_Filter_u.o $(App_Cpp_Objects)
endif

ifeq ($(EXTRAARGS), plain)
        App_Cpp_Flags += -DPLAINTEXT_MATCHING
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
all: $(TARGET)
	@echo "Build $(TARGET) [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the CBR_Enclave_Filter.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo


else
all: $(TARGET)
endif

run: all
ifneq ($(Build_Mode), HW_RELEASE)
	@$(CURDIR)/$(TARGET)
	@echo "RUN  =>  $(TARGET) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

######## App Objects ########

$(UNTRUSTED_DIR)/CBR_Enclave_Filter_u.c: $(SGX_EDGER8R) trusted/CBR_Enclave_Filter.edl
	@echo "GEN  =>  $@"
	@cd $(UNTRUSTED_DIR) && $(SGX_EDGER8R) --untrusted ../trusted/CBR_Enclave_Filter.edl --search-path ../trusted --search-path $(SGX_SDK)/include

$(UNTRUSTED_DIR)/CBR_Enclave_Filter_u.o: $(UNTRUSTED_DIR)/CBR_Enclave_Filter_u.c
	@echo "CC   <=  $< '$(App_C_Flags)'"
	@$(CC) $(App_C_Flags) -c $< -o $@

$(UNTRUSTED_DIR)/%.o: $(UNTRUSTED_DIR)/%.cpp
	@echo "CXX  <=  $<"
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ 

$(SCBR_SRCS)/%.o : $(SCBR_SRCS)/%.cpp
	@echo "CXX  <=  $<"
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ -I$(SCBR_SRCS)

$(SGXCOMM_DIR)/%.o : $(SGXCOMM_DIR)/%.cpp
	@echo "CXX  <=  $<"
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ -I$(SGXCOMM_DIR)

$(CBRPREFILTER_DIR)/%.o : $(CBRPREFILTER_DIR)/%.cc
	@echo "CXX  <=  $<"
	@$(CXX) $(App_Cpp_Flags) -c $< -o $@ -I$(CBRPREFILTER_DIR)

USERLIB := ../../../lib/libscbr.a
$(TARGET) : $(SamplePreReq) $(USERLIB)
	@echo "LINK =>  $@"
	@$(CXX) $^ -o $@ $(App_Link_Flags)

$(USERLIB):
	@make -C ../../.. lib/$(basename $@)

.PHONY: clean

clean:
	@rm -f $(TARGET)  $(App_Cpp_Objects) $(UNTRUSTED_DIR)/CBR_Enclave_Filter_u.* 
