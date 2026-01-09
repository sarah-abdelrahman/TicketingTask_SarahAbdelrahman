# set build folder path
ROOT_BUILD_DIR := ../build

# Per-SWC build folders
BACKOFFICE_BUILD := $(ROOT_BUILD_DIR)/Backoffice_SWC_build
GATE_BUILD      := $(ROOT_BUILD_DIR)/Gate_SWC_build
TVM_BUILD       := $(ROOT_BUILD_DIR)/TVM_SWC_build

# Executable names
BACKOFFICE_EXE := backoffice_app
GATE_EXE      := gate_app
TVM_EXE       := tvm_app

all: backoffice gate tvm

backoffice:
	mkdir -p $(BACKOFFICE_BUILD) $(ROOT_BUILD_DIR)
	cmake -S Backoffice_SWC -B $(BACKOFFICE_BUILD)
	cmake --build $(BACKOFFICE_BUILD) --target $(BACKOFFICE_EXE)
	cp -f $(BACKOFFICE_BUILD)/$(BACKOFFICE_EXE) $(ROOT_BUILD_DIR)/

gate:
	mkdir -p $(GATE_BUILD) $(ROOT_BUILD_DIR)
	cmake -S Gate_SWC -B $(GATE_BUILD)
	cmake --build $(GATE_BUILD) --target $(GATE_EXE)
	cp -f $(GATE_BUILD)/$(GATE_EXE) $(ROOT_BUILD_DIR)/

tvm:
	mkdir -p $(TVM_BUILD) $(ROOT_BUILD_DIR)
	cmake -S TVM_SWC -B $(TVM_BUILD)
	cmake --build $(TVM_BUILD) --target $(TVM_EXE)
	cp -f $(TVM_BUILD)/$(TVM_EXE) $(ROOT_BUILD_DIR)/

clean:
	rm -rf $(BACKOFFICE_BUILD) $(GATE_BUILD) $(TVM_BUILD)
	rm -f $(ROOT_BUILD_DIR)/$(BACKOFFICE_EXE) $(ROOT_BUILD_DIR)/$(GATE_EXE) $(ROOT_BUILD_DIR)/$(TVM_EXE)

.PHONY: all backoffice gate tvm clean
