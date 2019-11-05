ARCH =  riscv64

CC :=$(ARCH)-unknown-linux-gnu-g++

CFLAGS += -fpermissive -I../
LDFLAGS += -fpermissive

DEPS := $(wildcard *.c) $(wildcard *.h)
src_files := $(TARGET).c
OBJECTS = $(src_files:.c=.o)
DEPS += $(OBJECTS)

all: $(TARGET)$(POSTFIX)

.PHONY: $(TARGET)$(POSTFIX)

$(TARGET)$(POSTFIX): Makefile $(DEPS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	$(ARCH)-unknown-linux-gnu-objdump -D $@ > $(TARGET).asm

.c.o: $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

.cpp.o: $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

run:
	spike pk $(TARGET).rv

print-%:
	@echo $* = $($*)

clean:
	rm -f $(OBJECTS) $(TARGET).rv $(TARGET).asm $(TARGET).ll $(TARGET).S

cleanall: clean
	rm -f $(TARGET).inst $(TARGET) $(TARGET).driver
