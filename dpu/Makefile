# Hisilicon Hi35xx sample Makefile

include ../Makefile.param

SMP_SRCS := $(wildcard *.c)
SMP_SRCS += $(wildcard ./sample/*.c)
SMP_SRCS += $(wildcard ./common/*.c)

MPI_LIBS += $(REL_LIB)/libdpu_rect.a
MPI_LIBS += $(REL_LIB)/libdpu_match.a

CFLAGS += -I$(PWD)/common/
CFLAGS += -I$(PWD)/sample/
TARGET := sample_dpu_main

TARGET_PATH := $(PWD)

# compile linux or HuaweiLite
include $(PWD)/../$(ARM_ARCH)_$(OSTYPE).mak
