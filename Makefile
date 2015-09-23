CCPP = arm-linux-gnueabi-g++
CCPP_FLAGS = -static -std=c++14 -Wfatal-errors -Wall -g

OUTPUT_DIR = bin
INCLUDE_DIRS = -Iinclude -Iext/zee/include

LIB_DEPS = -lpthread

all: matrices

dirs:
	mkdir -p ${OUTPUT_DIR}

matrices: examples/matrices.cpp
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DEPS}
