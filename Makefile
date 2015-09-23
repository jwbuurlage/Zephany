## ARM
CCPP = arm-linux-gnueabi-g++
CCPP_FLAGS = -static -std=c++14 -Wfatal-errors -Wall -g

OUTPUT_DIR = bin
INCLUDE_DIRS = -Iinclude -Iext/zee/include

LIB_DEPS = -lpthread

## Epiphany
EGCC = e-gcc

# Prerequisites
all: dirs matrices

dirs:
	@mkdir -p ${OUTPUT_DIR}
	@mkdir -p ${OUTPUT_DIR}/kernels

ebsp:
	@cd ext/epiphany-bsp/ && make
# ... build ebsp

# Examples
matrices: examples/matrices.cpp
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DEPS}

hello_ebsp: examples/ebsp_example.cpp kernels/hello_world.c
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DEPS}

# Kernels
k_hello_world: kernels/hello_world.c
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/kernels/$@ $< ${LIB_DEPS}
