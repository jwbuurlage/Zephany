ESDK=/home/jw/adapteva/esdk
EBSP=ext/epiphany-bsp

## ARM
CCPP = arm-linux-gnueabihf-g++
CCPP_FLAGS = -std=c++14 -Wfatal-errors -Wall -g

OUTPUT_DIR = bin
INCLUDE_DIRS = -Iinclude\
				-Iext/zee/include\
				-Iext/epiphany-bsp/include\
				-I/usr/include\
				-I${ESDK}/tools/host/include

LIB_DIRS = -L${EBSP}/lib -L${ESDK}/tools/host/lib
LIB_DEPS = -lpthread
LIB_EBSP = -lhost-bsp -le-hal -le-loader

## Epiphany
EGCC = epiphany-elf-gcc
E_CFLAGS	= -static -std=c99 -O3 -ffast-math -Wall
E_LDF		= ${EBSP}/ebsp_fast.ldf
E_INCLUDES  = -I${EBSP}/include
E_LIBS = -L${EBSP}/lib \
		 -L/home/jw/adapteva#${ESDK}/tools/e-gnu/epiphany-elf/lib
E_LIB_NAMES = -le-bsp -le-lib


# Prerequisites
all: dirs examples kernels

kernels: bin/kernels/k_hello_world.srec bin/kernels/k_spmv.srec bin/kernels/k_cannon.srec

examples: streaming_lial hello_ebsp

dirs:
	@mkdir -p ${OUTPUT_DIR}
	@mkdir -p ${OUTPUT_DIR}/kernels

ebsp:
	@cd ext/epiphany-bsp/ && make

matrices: examples/matrices.cpp
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DEPS}

hello_ebsp: examples/ebsp_example.cpp kernels/k_hello_world.c
	@echo 'CC $@'
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DIRS} ${LIB_DEPS} ${LIB_EBSP}

streaming_lial: examples/streaming_lial.cpp kernels/k_spmv.c
	@echo 'CC $@'
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DIRS} ${LIB_DEPS} ${LIB_EBSP}

# Kernels
bin/kernels/%.srec: bin/kernels/%.elf
	@epiphany-elf-objcopy --srec-forceS3 --output-target srec $< $@

bin/kernels/k_hello_world.elf: kernels/k_hello_world.c
	@echo 'ECC $@'
	@${EGCC} ${E_CFLAGS} -T ${E_LDF} ${E_INCLUDES} -o $@ $< ${E_LIBS} ${E_LIB_NAMES}

bin/kernels/k_spmv.elf: kernels/k_spmv.c
	@echo 'ECC $@'
	@${EGCC} ${E_CFLAGS} -T ${E_LDF} ${E_INCLUDES} -o $@ $< ${E_LIBS} ${E_LIB_NAMES}

bin/kernels/k_cannon.elf: kernels/k_cannon.c
	@echo 'ECC $@'
	@${EGCC} ${E_CFLAGS} -T ${E_LDF} ${E_INCLUDES} -o $@ $< ${E_LIBS} ${E_LIB_NAMES}



clean:
	rm -r bin
