TL_ACCEL_DIR=src/hls/vadd_tl
TL_ACCEL_LIB=$(TL_ACCEL_DIR)/libvadd_tl.a

ROCC_ACCEL_DIR=src/hls/vadd_rocc
ROCC_ACCEL_LIB=$(ROCC_ACCEL_DIR)/libvadd_rocc.a

BENCHMARK_DIR=src/benchmark
BENCHMARK=$(BENCHMARK_DIR)/vadd

hardware: vadd_soc.json
	centrifuge generate_hw -c vadd_soc.json

clean_hardward:
	centrifuge clean_hw -c vadd_soc.json

wrappers: vadd_soc.json
	centrifuge generate_sw -c vadd_soc.json

clean_wrappers:
	rm -rf centrifuge_wrappers

.PHONY: bench
bench: $(BENCHMARK)

$(BENCHMARK): $(TL_ACCEL_LIB) $(ROCC_ACCEL_LIB)
	make -C $(BENCHMARK_DIR) vadd

$(TL_ACCEL_LIB):
	make -C $(TL_ACCEL_DIR) libvadd_tl.a

$(ROCC_ACCEL_LIB):
	make -C $(ROCC_ACCEL_DIR) libvadd_rocc.a

clean: clean_hw clean_sw
	
