# THIS MAKEFILE IS ONLY INTENDED FOR OPENBENCH
# BUILD WITH CMAKE PER THE INSTRUCTIONS IN THE README

EXE = polaris

SOURCES := src/main.cpp src/uci.cpp src/util/split.cpp src/hash.cpp src/position/position.cpp src/movegen.cpp src/attacks/black_magic/attacks.cpp src/search.cpp src/util/timer.cpp src/pretty.cpp src/ttable.cpp src/limit/time.cpp src/eval/nnue.cpp src/perft.cpp src/bench.cpp src/syzygy/tbprobe.cpp

SUFFIX :=

CXX := clang++
CXXFLAGS := -std=c++20 -O3 -flto -march=native -DNDEBUG -DPS_NATIVE -DPS_VERSION=OpenBench

LDFLAGS :=

ifeq ($(OS), Windows_NT)
    DETECTED_OS := Windows
    SUFFIX := .exe
# don't support gcc on windows, too much of a pain for now
    ifeq (,$(findstring clang,$(shell $(CXX) --version)))
        $(error GCC and MSVC unsupported on Windows)
    endif
    LDFLAGS += -fuse-ld=lld
else
    DETECTED_OS := $(shell uname -s)
    SUFFIX :=
    ifneq (,$(findstring clang,$(shell $(CXX) --version)))
        ifneq ($(DETECTED_OS),Darwin)
            LDFLAGS += -fuse-ld=lld
        endif
    endif
    LDFLAGS += -pthread
# don't ask
ifdef IS_COSMO
    CXXFLAGS += -stdlib=libc++
endif
endif

OUT := $(EXE)$(SUFFIX)

all: $(EXE)

$(EXE): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(OUT) $^

clean:

