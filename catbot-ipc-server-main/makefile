CXX ?= g++
SHELL := $(if $(wildcard /bin/sh),/bin/sh,$(if $(wildcard /bin/bash),/bin/bash,/usr/bin/bash))
BUILD_MAKEFILE := $(lastword $(MAKEFILE_LIST))
OUT_DIR = bin
INSTALL_DIR ?= /opt/cathook/ipc
TARGETS = server exec exec_all console

CXXFLAGS ?= $(shell g++ -std=c++23 -x c++ -E /dev/null > /dev/null 2>&1 && echo "-std=c++23" || echo "-std=c++17") -O2 -g -Wall -Wextra -fmessage-length=0
CXXFLAGS += -I$(CURDIR)/simple-ipc/include -I$(SRC_DIR)
DEPFLAGS = -MMD -MP
LDLIBS = -pthread -lrt
SRC_DIR = src
DEPS = $(addprefix $(OUT_DIR)/,$(addsuffix .d,$(TARGETS)))

.PHONY: all clean install

all: $(addprefix $(OUT_DIR)/,$(TARGETS))

install: all
	install -d -m 0755 "$(INSTALL_DIR)/bin"
	install -m 0755 $(addprefix $(OUT_DIR)/,$(TARGETS)) "$(INSTALL_DIR)/bin/"

$(OUT_DIR)/%: $(SRC_DIR)/%.cpp $(BUILD_MAKEFILE) $(SRC_DIR)/cathookipc.hpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -MF $(OUT_DIR)/$*.d -MT $@ $< $(LDLIBS) -o $@

clean:
	rm -rf "$(OUT_DIR)"

-include $(DEPS)
