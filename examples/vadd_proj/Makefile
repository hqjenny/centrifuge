include common.mk

TL_BASE_DIR=src/hls/vadd_tl
TL_BASE_LIB=$(TL_BASE_DIR)/libvadd_tl.a

ROCC_BASE_DIR=src/hls/vadd_rocc
ROCC_BASE_LIB=$(ROCC_BASE_DIR)/libvadd_rocc.a

TL_WRAPPER_DIR=centrifuge_wrappers/tl0_vadd_tl_vadd_tl
TL_WRAPPER_LIB=$(TL_WRAPPER_DIR)/libvadd_tl_accel.a

ROCC_WRAPPER_DIR=centrifuge_wrappers/rocc0_vadd_rocc_vadd_rocc
ROCC_WRAPPER_LIB=$(ROCC_WRAPPER_DIR)/libvadd_rocc_accel.a

BENCHMARK_DIR=src/benchmark
BENCHMARK=$(BENCHMARK_DIR)/vadd

BUILD_DIR=./build
OUTPUTS=$(BENCHMARK) $(TL_BASE_LIB) $(ROCC_BASE_LIB) $(ROCC_WRAPPER_LIB) $(TL_WRAPPER_LIB)
INSTALLS=$(foreach output,$(OUTPUTS),$(join $(BUILD_DIR)/,$(notdir output)))

hardware: vadd_soc.json
	centrifuge generate_hw -c vadd_soc.json

clean_hardward:
	centrifuge clean_hw -c vadd_soc.json

wrappers: vadd_soc.json
	centrifuge generate_sw -c vadd_soc.json
	cd $(ROCC_WRAPPER_DIR) && ln -sf ../../wrapper.mk Makefile
	cd $(TL_WRAPPER_DIR) && ln -sf ../../wrapper.mk Makefile

clean_wrappers:
	rm -rf centrifuge_wrappers

$(BUILD_DIR):
	mkdir -p $@ 
	
.PHONY: bench
bench: $(BENCHMARK)

$(BENCHMARK): $(TL_ACCEL_LIB) $(ROCC_ACCEL_LIB) build
	make -C $(BENCHMARK_DIR) $(notdir $@)

$(TL_BASE_LIB): build
	make -C $(TL_ACCEL_DIR) $(notdir $@)

$(ROCC_BASE_LIB): build
	make -C $(ROCC_ACCEL_DIR) $(notdir $@)

$(TL_WRAPPER_LIB): build
	make -C $(TL_WRAPPER_DIR) $(notdir $@)

$(ROCC_WRAPPER_LIB): build
	make -C $(ROCC_WRAPPER_DIR) $(notdir $@)

.PHONY: install
install: $(OUTPUTS)

$(INSTALLS): 
clean:
	make -C $(BENCHMARK_DIR) clean 
	make -C $(TL_ACCEL_DIR) clean 
	make -C $(ROCC_ACCEL_DIR) clean

print-%  :
	@echo $* = $($*)
