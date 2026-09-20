BUILD_DIR := build
NEMU_HOME := $(abspath ../nemu)

all:
	@echo "compiler to exe"
	verilator -cc --exe --build -Wall vsrc/top.v csrc/main.cpp --top-module top

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."  
	@mkdir -p $(BUILD_DIR)/obj_dir
	verilator -Wall -Wno-fatal -Wno-DECLFILENAME \
	--cc --trace-fst --exe --build -Ivsrc \
	--Mdir $(BUILD_DIR)/obj_dir --top-module top \
	 $(abspath vsrc/top.v) \
	 $(abspath csrc/main.cpp) \
	 $(abspath csrc/utils.cpp)
	$(BUILD_DIR)/obj_dir/Vtop

minirvEMU:
	g++ -std=c++17 -Wall -Wextra -Werror csrc/minirvEMU.cpp csrc/utils.cpp csrc/minirvEMU_test.cpp -o minirvEMU_test

clean:
	@echo "clean the obj_dir"
	rm -rf build/
	rm minirvEMU_test
	rm *.fst
include ../Makefile
