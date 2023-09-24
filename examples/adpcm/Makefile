VERBOSE=1
TARGET ?=adpcm
FUNC=encode
WRAPPER_DIR=${RDIR}/tools/centrifuge/examples/chstone_proj/src/hls/adpcm/centrifuge_wrappers/rocc0_adpcm_encode/
BM_INC_DIR=${RDIR}/tools/centrifuge/scripts/sw_aux/sw_helper/

CFLAGS+=-fno-common -fno-builtin-printf -specs=htif_nano.specs 
LDFLAGS+= -L$(WRAPPER_DIR) -laccel_wrapper
INCLUDES+=-I${BM_INC_DIR}

CC=riscv64-unknown-elf-gcc
OBJDUMP=riscv64-unknown-elf-objdump

%.dump: %.rv
	$(OBJDUMP) -D $< > $@

SRC = $(wildcard *.c)

all: $(addsuffix .bm.rv,$(TARGET)) $(addsuffix .bm_accel.rv,$(TARGET))  

$(TARGET).bm.rv: 
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SRC) -o $@

$(TARGET).bm_accel.rv:
	$(CC) $(CFLAGS) -DCUSTOM_INST -DWRAP_$(FUNC) $(LDFLAGS) $(INCLUDES) $(SRC)  -o $@

clean:
	rm -f *.rv *.o *.dump


