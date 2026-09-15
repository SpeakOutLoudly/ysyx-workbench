all:
	@echo "compiler to exe"
	verilator -cc --exe --build -Wall vsrc/top.v csrc/main.cpp --top-module top

sim:
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Write this Makefile by your self."

clean:
	@echo "no file to clean"
include ../Makefile
