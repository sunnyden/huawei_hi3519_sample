# Hisilicon sample Makefile

include ../Makefile.param

# target source
SMP_SRCS  := $(wildcard *.c)
TARGET := $(SMP_SRCS:%.c=%)

MPI_LIBS += $(REL_LIB)/libhiavslut.a
MPI_LIBS += $(REL_LIB)/libpos_query.a

TARGET_PATH := $(PWD)

# compile linux or HuaweiLite
include $(PWD)/../$(ARM_ARCH)_$(OSTYPE).mak
