VERBOSE=1
TARGET ?=adpcm
FUNC=encode
WRAPPER_DIR=${RDIR}/tools/centrifuge/examples/chstone_proj/src/hls/adpcm/centrifuge_wrappers/rocc0_adpcm_encode/
BM_INC_DIR=${RDIR}/tools/centrifuge/scripts/sw_aux/sw_helper/

CFLAGS+=-fno-common -fno-builtin-printf -specs=htif_nano.specs 
LDFLAGS+= 
INCLUDES+=-I$(BM_INC_DIR) 
ACCFLAGS=-DCUSTOM_INST -DWRAP_$(FUNC) -L$(WRAPPER_DIR) -laccel_wrapper -I$(WRAPPER_DIR)  

CC=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump

%.dump: %.rv
	$(OBJDUMP) -D $< > $@

SRC = $(wildcard *.c)

all: $(addsuffix .bm.rv,$(TARGET)) $(addsuffix .bm_accel.rv,$(TARGET))  

# generate reference software code
$(TARGET).bm.rv: 
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o $@

# generate accelerator code
$(TARGET).bm_accel.rv:
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(ACCFLAGS) -o $@

clean:
	rm -f *.rv *.o *.dump


