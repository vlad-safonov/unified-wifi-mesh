##########################################################################
 # Copyright 2023 Comcast Cable Communications Management, LLC
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #
 # SPDX-License-Identifier: Apache-2.0
##########################################################################

include ../makefile.inc

#
# program
#

PROGRAM = $(INSTALLDIR)/bin/onewifi_em_ctrl
EM_CERT_SRC = $(INSTALLDIR)/config/test_cert.crt
EM_KEY_SRC  = $(INSTALLDIR)/config/test_cert.key
EM_RESET_JSON = $(INSTALLDIR)/bin/Reset.json
CERT_TARGETS = $(NVRAM_DIR)/test_cert.crt $(NVRAM_DIR)/test_cert.key $(NVRAM_DIR)/Reset.json


INCLUDEDIRS = \
	-I$(ONEWIFI_EM_HOME)/inc \
	-I$(ONEWIFI_HAL_INTF_HOME)/ \
	-I$(ONEWIFI_HOME)/source/utils \
	-I$(ONEWIFI_HOME)/include \
   	-I$(RBUS_HOME)/include \
   	-I$(ONEWIFI_EM_HOME)/src/util_crypto \
	-I$(ONEWIFI_HOME)/lib/log \
	-I$(ONEWIFI_HOME)/lib/ds \
	-I$(ONEWIFI_HOME)/source/platform/linux \
	-I$(ONEWIFI_HOME)/source/platform/common \
	-I$(ONEWIFI_HOME)/source/platform/linux/he_bus/inc \
	-I$(ONEWIFI_HOME)/source/ccsp
ifeq ($(WITH_SAP), 1)
INCLUDEDIRS += -I$(AL_SAP_HOME)/include
endif
INDLUDEDIRS += -I$(WIFI_CJSON)

CXX_COMMON_FLAGS = -Wall -Wextra -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-aliasing -fno-common -fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIC -pie -ftrapv -Wformat=2 -Wformat-security -Wuninitialized -Winit-self -Wsign-conversion -Wno-unused-parameter -fsanitize=address -fsanitize=undefined -Werror #-Wconversion -O2 -Weffc++
CXX_SPECIFIC_FLAGS = -Wctor-dtor-privacy -Wold-style-cast -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -std=c++17

CXXFLAGS += $(INCLUDEDIRS) -g $(CXX_COMMON_FLAGS) $(CXX_SPECIFIC_FLAGS) `mariadb_config --include`
CFLAGS += $(INCLUDEDIRS) -g $(CXX_COMMON_FLAGS)

ifeq ($(WITH_SAP), 1)
CXXFLAGS += -DAL_SAP
endif

ifeq ($(ENABLE_DEBUG_MODE),ON)
$(info ENABLE_DEBUG_MODE is set)
CXXFLAGS += -DDEBUG_MODE
endif

ifdef TESTING
CXXFLAGS += -DTESTING
endif

LDFLAGS += $(LIBDIRS) $(LIBS) -fsanitize=address -fsanitize=undefined
LIBDIRS = \
	-L$(INSTALLDIR)/lib \
	-L$(ONEWIFI_HOME)/install/lib/
ifeq ($(WITH_SAP), 1)
LIBDIRS += -L$(AL_SAP_HOME)/
endif

LIBS = -lm -lpthread -ldl -luuid -lcjson -lssl -lcrypto -lhebus `mariadb_config --libs`
ifeq ($(WITH_SAP), 1)
LIBS += -lalsap
endif

GENERIC_SOURCES = $(ONEWIFI_HOME)/source/utils/collection.c \
	$(ONEWIFI_EM_SRC)/util_crypto/aes_siv.c \
	$(ONEWIFI_HOME)/lib/common/util.c \
    $(ONEWIFI_HOME)/source/platform/linux/bus.c \


CTRL_SOURCES = $(wildcard $(ONEWIFI_EM_SRC)/em/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/config/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/prov/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/prov/easyconnect/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/disc/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/channel/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/capability/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/metrics/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/steering/*.cpp) \
    	$(wildcard $(ONEWIFI_EM_SRC)/em/policy_cfg/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/em/crypto/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/cmd/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/ctrl/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/db/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/dm/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/ctrl/tr_181/*.cpp) \
	$(wildcard $(ONEWIFI_EM_SRC)/ctrl/tr_181/wfa_data_model/*.cpp) \
	$(ONEWIFI_EM_SRC)/orch/em_orch.cpp \
	$(ONEWIFI_EM_SRC)/orch/em_orch_ctrl.cpp \
	$(ONEWIFI_EM_SRC)/utils/util.cpp \
	$(ONEWIFI_EM_SRC)/utils/timer.cpp \

CTRL_OBJECTS = $(CTRL_SOURCES:.cpp=.o)
GENERIC_OBJECTS = $(GENERIC_SOURCES:.c=.o) 
ALLOBJECTS = $(CTRL_OBJECTS) $(GENERIC_OBJECTS)

# Per-target build directory — keeps ctrl objects isolated from cli objects
# so each target compiles with its own flags (-fsanitize, -fPIC, etc.)
CTRL_BUILDDIR  = $(ONEWIFI_EM_HOME)/build/ctrl/obj
CTRL_OBJDIR_GENERIC = $(CTRL_BUILDDIR)/generic

CTRL_OBJECTS  := $(patsubst $(ONEWIFI_EM_SRC)/%.cpp, $(CTRL_BUILDDIR)/%.o, $(CTRL_SOURCES))
GENERIC_OBJECTS := $(addprefix $(CTRL_OBJDIR_GENERIC)/, \
                    $(patsubst %.c, %.o, $(notdir $(GENERIC_SOURCES))))
ALLOBJECTS := $(CTRL_OBJECTS) $(GENERIC_OBJECTS)

all: $(PROGRAM) $(CERT_TARGETS)

$(PROGRAM): $(ALLOBJECTS)
	$(CXX) -o $@ $(ALLOBJECTS) $(LDFLAGS)

# Ctrl source objects — mirror src directory structure under CTRL_BUILDDIR
$(CTRL_BUILDDIR)/%.o: $(ONEWIFI_EM_SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

# Generic source objects — flat layout under CTRL_OBJDIR_GENERIC
# Explicit per-file rules to handle sources from different directories
$(CTRL_OBJDIR_GENERIC):
	mkdir -p $@

define COMPILE_CTRL_GENERIC_C
$(CTRL_OBJDIR_GENERIC)/$(patsubst %.c,%.o,$(notdir $(1))): $(1) | $(CTRL_OBJDIR_GENERIC)
	$(CC) $(CFLAGS) -o $$@ -c $$<
endef
$(foreach src,$(GENERIC_SOURCES),$(eval $(call COMPILE_CTRL_GENERIC_C,$(src))))

# Make sure $(NVRAM_DIR) exists
$(NVRAM_DIR):
	@if [ ! -d "$@" ]; then \
        sudo mkdir -p "$@" && sudo chmod 777 "$@"; \
    fi

#copy the certificate and key to /nvram
$(NVRAM_DIR)/test_cert.crt: $(EM_CERT_SRC) | $(NVRAM_DIR)
	$(CP) $< $@

$(NVRAM_DIR)/test_cert.key: $(EM_KEY_SRC) | $(NVRAM_DIR)
	$(CP) $< $@

$(NVRAM_DIR)/Reset.json: $(EM_RESET_JSON) | $(NVRAM_DIR)
	$(CP) $< $@

# Clean everything
clean:
	$(RM) $(CTRL_BUILDDIR) $(PROGRAM)
	$(MAKE) clean_tests

# Google Test integration
TEST_DIR = $(ONEWIFI_EM_HOME)/tests
TEST_EXEC = $(INSTALLDIR)/bin/onewifi_em_ctrl_test
# Common test files (everything except agent specific unit test files and test_l1_al*.cpp)
EXCLUDE_TESTS = \
    $(TEST_DIR)/test_l1_al%.cpp \
    $(TEST_DIR)/test_em_crypto.cpp \
    $(TEST_DIR)/test_l1_em_cmd_em_config.cpp \
    $(TEST_DIR)/test_l1_em_cmd_set_channel.cpp \
    $(TEST_DIR)/test_l1_em_cmd.cpp \
    $(TEST_DIR)/test_l1_em_simulator.cpp \
    $(TEST_DIR)/test_l1_dm_device.cpp \
    $(TEST_DIR)/test_l1_dm_easy_mesh.cpp \
    $(TEST_DIR)/test_l1_dm_easy_mesh_list.cpp \
    $(TEST_DIR)/test_l1_dm_op_class.cpp \
    $(TEST_DIR)/test_l1_dm_radio_cap.cpp \
    $(TEST_DIR)/test_l1_dm_network_ssid.cpp \
    $(TEST_DIR)/test_l1_em_cmd_scan_result.cpp \
    $(TEST_DIR)/test_l1_ec_enrollee.cpp \
    $(TEST_DIR)/test_l1_ec_util.cpp \
    $(TEST_DIR)/test_l1_ec_manager.cpp \
    $(TEST_DIR)/test_l1_em_cmd_channel_pref_query.cpp \
    $(TEST_DIR)/test_l1_em_cmd_remove_device.cpp \
    $(TEST_DIR)/test_l1_em_cmd_sta_link_metrics.cpp \
    $(TEST_DIR)/test_l1_em_cmd_sta_steer.cpp \
    $(TEST_DIR)/test_l1_em_cmd_start_dpp.cpp \
    $(TEST_DIR)/test_l1_em_ctrl.cpp \
    $(TEST_DIR)/test_l1_em_msg.cpp \
    $(TEST_DIR)/test_l1_em_orch_ctrl.cpp \
    $(TEST_DIR)/test_l1_em_orch.cpp \
    $(TEST_DIR)/test_l1_em_orch_agent.cpp \
    $(TEST_DIR)/test_l1_em_network_topo.cpp \
    $(TEST_DIR)/test_l1_em_provisioning.cpp \
    $(TEST_DIR)/test_l1_em_sm.cpp \
    $(TEST_DIR)/test_l1_em_cmd_ap_metrics_report.cpp \
    $(TEST_DIR)/test_l1_em_metrics.cpp \
    $(TEST_DIR)/test_l1_em_cmd_btm_report.cpp
COMMON_TEST_SOURCES = $(filter-out $(EXCLUDE_TESTS), $(wildcard $(TEST_DIR)/*.cpp))

# SAP-specific test files
SAP_TEST_SOURCES = $(wildcard $(TEST_DIR)/test_l1_al*.cpp)
SAP_TEST_OBJECTS = $(SAP_TEST_SOURCES:.cpp=.o)
# Combine them based on WITH_SAP
TEST_SOURCES = $(COMMON_TEST_SOURCES)
ifeq ($(WITH_SAP), 1)
TEST_SOURCES += $(SAP_TEST_SOURCES)
endif
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)
TEST_CXXFLAGS = $(CXXFLAGS) -I/usr/include -I/usr/local/include -DTESTING

# em_ctrl.cpp uses #ifndef TESTING to exclude main() — compile a separate
# test-only version so we don't recompile all objects just for this one file
EM_CTRL_SRC     = $(ONEWIFI_EM_SRC)/ctrl/em_ctrl.cpp
EM_CTRL_MAIN_OBJ = $(CTRL_BUILDDIR)/ctrl/em_ctrl.o
EM_CTRL_TEST_OBJ = $(CTRL_BUILDDIR)/ctrl/em_ctrl_testing.o

$(EM_CTRL_TEST_OBJ): $(EM_CTRL_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DTESTING -o $@ -c $<

# Substitute the regular em_ctrl.o with the TESTING version for the test link
TEST_ALLOBJECTS = $(filter-out $(EM_CTRL_MAIN_OBJ), $(ALLOBJECTS)) $(EM_CTRL_TEST_OBJ)

# Build test executable only (no run)
build_test: check_test_files install_gtest $(TEST_OBJECTS) $(EM_CTRL_TEST_OBJ)
ifeq ($(WITH_SAP), 1)
	@$(MAKE) -C $(AL_SAP_HOME)
endif
	@echo "Building test executable with same link order as main build..."
	$(CXX) -o $(TEST_EXEC) $(TEST_OBJECTS) $(TEST_ALLOBJECTS) $(LIBDIRS) $(LIBS) -lgtest -lgtest_main -pthread -fsanitize=address -fsanitize=undefined

# Run test executable only (must be built first with build_test)
run_test:
	@echo "Running tests..."
	@cd $(TEST_DIR) && \
	$(TEST_EXEC)

# Test target (build and run)
test: build_test run_test

# Compile test source files — depend on install_gtest to ensure headers are present
# before any parallel compilation starts (critical for make -j)
$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp | install_gtest
	@echo "Compiling test file $<..."
	@$(CXX) $(TEST_CXXFLAGS) -o $@ -c $<
	@echo "$< compiled"

# Check if any test files exist
check_test_files:
	@if [ -z "$(TEST_SOURCES)" ]; then \
		echo "Error: No test files found in $(TEST_DIR)/"; \
		exit 1; \
	fi

# Install GTest if needed
install_gtest:
	@echo "Checking and installing Google Test if needed..."
	@chmod +x $(ONEWIFI_EM_HOME)/build/install-gtest.sh
	@$(ONEWIFI_EM_HOME)/build/install-gtest.sh

# Run the program
run:
	./$(PROGRAM)

# Clean test files only
clean_tests:
	$(RM) $(TEST_OBJECTS) $(TEST_EXEC) $(EM_CTRL_TEST_OBJ)
ifeq ($(WITH_SAP), 1)
	$(RM) $(SAP_TEST_OBJECTS)
endif
.PHONY: all test build_test run_test install_gtest check_test_files clean_tests compile_test_objects clean run
