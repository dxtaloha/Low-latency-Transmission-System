# Common Makefile parts for BPF-building with libbpf
# --------------------------------------------------
# SPDX-License-Identifier: (GPL-2.0 OR BSD-2-Clause)
#
# This file should be included from your Makefile like:
#  COMMON_DIR = ../common
#  include $(COMMON_DIR)/common.mk
#
# It is expected that you define the variables:
#  XDP_TARGETS and USER_TARGETS
# as a space-separated list
#
LLC ?= llc
CLANG ?= clang
CC ?= gcc

XDP_C = ${XDP_TARGETS:=.c}
XDP_OBJ = ${XDP_C:.c=.o}
USER_C := ${USER_TARGETS:=.c}
USER_OBJ := ${USER_C:.c=.o}

# Expect this is defined by including Makefile, but define if not
COMMON_DIR ?= ../common
LIB_DIR ?= ../lib

FPE_DIR ?= ../advanced03-AF_XDP/src

COPY_LOADER ?=
LOADER_DIR ?= $(LIB_DIR)/xdp-tools/xdp-loader
STATS_DIR ?= $(COMMON_DIR)/../basic-solutions

COMMON_OBJS += $(COMMON_DIR)/common_params.o
include $(LIB_DIR)/defines.mk

# Create expansions for dependencies
COMMON_H := ${COMMON_OBJS:.o=.h}

EXTRA_DEPS +=

# BPF-prog kern and userspace shares struct via header file:
KERN_USER_H ?= $(wildcard common_kern_user.h)


CFLAGS += $(shell printenv CFLAGS) -O2 -Wall -fPIC -Wno-deprecated-declarations -I$(LIB_DIR)/install/include $(EXTRA_CFLAGS) -I$(FPE_DIR)
BPF_CFLAGS += -I$(LIB_DIR)/install/include $(EXTRA_CFLAGS)
LDFLAGS += $(shell printenv LDFLAGS) -lm -lcrypto -L$(LIB_DIR)/install/lib

LIB = libfpe.a libfpe.so
OBJS = $(FPE_DIR)/ff1.o $(FPE_DIR)/ff3.o $(FPE_DIR)/fpe_locl.o

BPF_HEADERS := $(wildcard $(HEADER_DIR)/*/*.h) $(wildcard $(INCLUDE_DIR)/*/*.h)

all: llvm-check $(LIB) $(USER_TARGETS) $(XDP_OBJ) $(COPY_LOADER) $(COPY_STATS)

libfpe.a: $(OBJS)
	ar rcs $@ $(OBJS)
libfpe.so: $(OBJS)
	gcc -shared -fPIC -Wl,-soname,libfpe.so $(OBJS) $(LDFLAGS) -o $@

.PHONY: clean $(CLANG) $(LLC)

$(FPE_DIR)/ff1.o: $(FPE_DIR)/ff1.c
	gcc $(CFLAGS) -c $(FPE_DIR)/ff1.c -o $@

$(FPE_DIR)/ff3.o: $(FPE_DIR)/ff3.c
	gcc $(CFLAGS) -c $(FPE_DIR)/ff3.c -o $@

$(FPE_DIR)/fpe_locl.o: $(FPE_DIR)/fpe_locl.c
	gcc $(CFLAGS) -c $(FPE_DIR)/fpe_locl.c -o $@

clean:
	$(Q)rm -f $(LIB) $(USER_TARGETS) $(XDP_OBJ) $(USER_OBJ) $(COPY_LOADER) $(COPY_STATS) *.ll

ifdef COPY_LOADER
$(LOADER_DIR)/$(COPY_LOADER):
	$(Q)make -C $(LOADER_DIR)

$(COPY_LOADER): $(LOADER_DIR)/$(COPY_LOADER)
	$(QUIET_COPY)cp $(LOADER_DIR)/$(COPY_LOADER) $(COPY_LOADER)
endif

ifdef COPY_STATS
$(STATS_DIR)/$(COPY_STATS):	$(STATS_DIR)/${COPY_STATS:=.c} $(COMMON_H)
	$(Q)make -C $(STATS_DIR) $(COPY_STATS)

$(COPY_STATS):	$(STATS_DIR)/$(COPY_STATS)
	$(QUIET_COPY)cp $(STATS_DIR)/$(COPY_STATS) $(COPY_STATS)
# Needing xdp_stats imply depending on header files:
EXTRA_DEPS += $(COMMON_DIR)/xdp_stats_kern.h $(COMMON_DIR)/xdp_stats_kern_user.h
endif

# For build dependency on this file, if it gets updated
COMMON_MK = $(COMMON_DIR)/common.mk

llvm-check: $(CLANG) $(LLC)
	@for TOOL in $^ ; do \
		if [ ! $$(command -v $${TOOL} 2>/dev/null) ]; then \
			echo "*** ERROR: Cannot find tool $${TOOL}" ;\
			exit 1; \
		else true; fi; \
	done

$(OBJECT_LIBBPF):
	@if [ ! -d $(LIBBPF_DIR) ]; then \
		echo "Error: Need libbpf submodule" $(LIBBPF_DIR); \
		echo "May need to run git submodule update --init"; \
		exit 1; \
	else \
		cd $(LIBBPF_DIR) && $(MAKE) all OBJDIR=.; \
		mkdir -p build; $(MAKE) install_headers DESTDIR=build OBJDIR=.; \
	fi

$(OBJECT_LIBXDP):
	@if [ ! -d $(LIBXDP_DIR) ]; then \
		echo "Error: Need libxdp submodule" $(LIBXDP_DIR); \
		echo "May need to run git submodule update --init"; \
		exit 1; \
	else \
		cd $(LIBXDP_DIR) && $(MAKE) all OBJDIR=.; \
	fi

# Create dependency: detect if C-file change and touch H-file, to trigger
# target $(COMMON_OBJS)
$(COMMON_H): %.h: %.c
	touch $@

# Detect if any of common obj changed and create dependency on .h-files
$(COMMON_OBJS):	%.o: %.h
	$(Q)$(MAKE) -C $(COMMON_DIR)

$(USER_TARGETS): %: %.c  $(OBJECT_LIBBPF) $(OBJECT_LIBXDP) Makefile $(COMMON_MK) $(COMMON_OBJS) $(KERN_USER_H) $(EXTRA_DEPS)
	$(QUIET_CC)$(CC) -Wall $(CFLAGS) $(LDFLAGS) -Wl,-rpath,../advanced03-AF_XDP -o $@ $(COMMON_OBJS) $(LIB_OBJS) \
	  $< $(LDLIBS) -L../advanced03-AF_XDP -lfpe

$(XDP_OBJ): %.o: %.c  Makefile $(COMMON_MK) $(KERN_USER_H) $(EXTRA_DEPS) $(OBJECT_LIBBPF)
	$(QUIET_CLANG)$(CLANG) -S \
	    -target bpf \
	    -D __BPF_TRACING__ \
	    $(BPF_CFLAGS) \
	    -Wall \
	    -Wno-unused-value \
	    -Wno-pointer-sign \
	    -Wno-compare-distinct-pointer-types \
	    -Werror \
	    -O2 -emit-llvm -c -g -o ${@:.o=.ll} $<
	$(QUIET_LLC)$(LLC) -march=bpf -filetype=obj -o $@ ${@:.o=.ll}
