CC ?= cc

CPPFLAGS += -Iupstream \
	-DMLK_CONFIG_PARAMETER_SET=512 \
	-DMLK_CONFIG_NAMESPACE_PREFIX=mlkem
CFLAGS += -O3 -std=c99 -Wall -Wextra -Wpedantic

BUILD_DIR := build
COMMON_SOURCES := upstream/mlkem_native.c src/randombytes_ohos.c
SELFTEST := $(BUILD_DIR)/mlkem_selftest
HYBRID_SELFTEST := $(BUILD_DIR)/mlkem_hybrid_selftest
BENCHMARK := $(BUILD_DIR)/mlkem_benchmark

.PHONY: all test benchmark clean

all: $(SELFTEST) $(HYBRID_SELFTEST) $(BENCHMARK)

$(BUILD_DIR):
	mkdir -p $@

$(SELFTEST): $(COMMON_SOURCES) src/mlkem_selftest.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

$(HYBRID_SELFTEST): $(COMMON_SOURCES) src/mlkem_hybrid_selftest.c \
		src/mlkem_hybrid.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) -Isrc $(CFLAGS) $(COMMON_SOURCES) \
		src/mlkem_hybrid_selftest.c -o $@

$(BENCHMARK): $(COMMON_SOURCES) src/mlkem_benchmark.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

test: $(SELFTEST) $(HYBRID_SELFTEST)
	./$(SELFTEST)
	./$(HYBRID_SELFTEST)

benchmark: $(BENCHMARK)
	./$(BENCHMARK)

clean:
	rm -rf $(BUILD_DIR)
