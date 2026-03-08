TOP = NPC_Top
OBJ_DIR = ./obj_dir
PRJ = playground

# 1. 自动寻找源码
VSRCS = $(shell find ./vsrc -name "*.v" -o -name "*.sv")
CSRCS = $(shell find ./csrc -name "*.cpp")

# 2. Capstone 配置 (DiffTest 必需，保持不变)
CAPSTONE_REPO_DIR = $(abspath ./tools/capstone/repo)
LIBCAPSTONE = $(CAPSTONE_REPO_DIR)/libcapstone.so.5
CXXFLAGS += -I./include -I$(CAPSTONE_REPO_DIR)/include -DTOP_NAME="\"V$(TOP)\""
LDFLAGS  += -lreadline -ldl

# 3. Verilator 参数
# --Mdir ./obj_dir : 指定生成目录，对应 npc.mk 里的路径
VERILATOR_FLAGS += --cc --exe --build --trace \
	--top-module $(TOP) \
	--Mdir $(OBJ_DIR) \
	-CFLAGS "$(CXXFLAGS)" \
	-LDFLAGS "$(LDFLAGS)"

.PHONY: sim clean

# 默认目标
sim: $(LIBCAPSTONE)
	$(call git_commit, "sim RTL")
	@echo "Build Simulator..."
	# Verilator 会自动在 obj_dir 下生成 VNPC_Top
	verilator $(VERILATOR_FLAGS) $(VSRCS) $(CSRCS)

$(LIBCAPSTONE):
	$(MAKE) -C $(CAPSTONE_REPO_DIR)

clean:
	rm -rf $(OBJ_DIR)

-include ../Makefile