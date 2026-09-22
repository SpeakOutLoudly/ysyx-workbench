BUILD_DIR := build
NEMU_HOME := $(abspath ../nemu)
OBJ_DIR   := $(BUILD_DIR)/obj_dir
NPC_EXEC  := $(OBJ_DIR)/Vtop

VERILOG_SRCS := $(abspath vsrc/top.v)
VERILOG_DEPS := $(wildcard vsrc/*.v vsrc/*.vh vsrc/sub/*.v)
CPP_SRCS     := $(abspath csrc/main.cpp) \
                $(abspath csrc/minirvEMU.cpp) \
                $(abspath csrc/utils.cpp)
CPP_HDRS     := $(wildcard csrc/*.h)

$(NPC_EXEC): $(VERILOG_DEPS) $(CPP_SRCS) $(CPP_HDRS)
	@mkdir -p $(OBJ_DIR)
	verilator -Wall -Wno-fatal -Wno-DECLFILENAME \
	  --cc --trace-fst --exe --build -Ivsrc \
	  --Mdir $(OBJ_DIR) --top-module top \
	  $(VERILOG_SRCS) $(CPP_SRCS)

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."  
	@mkdir -p $(BUILD_DIR)/obj_dir
	verilator -Wall -Wno-fatal -Wno-DECLFILENAME \
	--cc --trace-fst --exe --build -Ivsrc \
	--Mdir $(BUILD_DIR)/obj_dir --top-module top \
	 $(abspath vsrc/top.v) \
	 $(abspath csrc/main.cpp) \
	 $(abspath csrc/minirvEMU.cpp) \
	 $(abspath csrc/utils.cpp)
	$(BUILD_DIR)/obj_dir/Vtop

minirvEMU:
	g++ -std=c++17 -Wall -Wextra -Werror csrc/minirvEMU.cpp csrc/utils.cpp csrc/minirvEMU_test.cpp -o minirvEMU_test

run: $(NPC_EXEC)
	$(NPC_EXEC) $(IMG) $(ARGS)

clean:
	@echo "clean the obj_dir"
	rm -rf build/
	rm minirvEMU_test
	rm *.fst
include ../Makefile

.PHONY: sim minirvEMU run clean
