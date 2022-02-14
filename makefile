RM := rm -rf
MKDIR_P = mkdir -p

UNUSED := -Wno-unused-variable

HEADERS += \
src/parser.h \
src/replay.h \
src/analyzer.h \
src/analysis.h \
src/compressor.h \
src/enums.h \
src/schema.h \
src/gecko-legacy.h \
src/util.h

HEADERS_TEST += \
src/tests.h

OBJS += \
build/parser.o \
build/replay.o \
build/analyzer.o \
build/analysis.o \
build/compressor.o

CPP_DEPS += \
build/parser.d \
build/replay.d \
build/analyzer.d \
build/analysis.d \
build/compressor.d

OBJS_MAIN = ${OBJS} build/main.o
CPP_DEPS_MAIN = ${CPP_DEPS} build/main.d

OBJS_TEST = ${OBJS} build/tests.o
CPP_DEPS_TEST = ${CPP_DEPS} build/tests.d

DEFINES += \
	-D__GXX_EXPERIMENTAL_CXX0X__

OUT_DIR = build

# OLEVEL := -O0
OLEVEL := -O3
# OLEVEL := -Ofast -march=native -frename-registers -fno-signed-zeros -fno-trapping-math

all: directories base

base: INCLUDES += -I/usr/include/lzma
base: LIBS += -llzma
base: slippc

test: INCLUDES += -I/usr/include/lzma
test: LIBS += -llzma
test: slippc-tests

gui: GUI = -DGUI_ENABLED=1
gui: base

static: LIBS += -L ./lib-lin -static -llzma
static: slippc

statictest: LIBS += -L ./lib-lin -static -llzma
statictest: slippc-tests

staticgui: GUI = -DGUI_ENABLED=1
staticgui: static

slippc: $(OBJS_MAIN)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib -std=c++17 -o "./slippc" $(OBJS_MAIN) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

slippc-tests: $(OBJS_TEST)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib -std=c++17 -o "./slippc-tests" $(OBJS_TEST) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

build/tests.o: ./src/tests.cpp $(HEADERS_TEST)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(LINK.c) $< -c -o $@
	g++ $(DEFINES) $(GUI) $(INCLUDES) $(OLEVEL) -g3 -Wall -c -fmessage-length=0 -std=c++17 $(UNUSED) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

build/%.o: ./src/%.cpp $(HEADERS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(LINK.c) $< -c -o $@
	g++ $(DEFINES) $(GUI) $(INCLUDES) $(OLEVEL) -g3 -Wall -c -fmessage-length=0 -std=c++17 $(UNUSED) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

clean:
	-$(RM) $(OBJS_MAIN) $(OBJS_TEST) $(C++_DEPS) ./slippc ./slippc-tests
	-@echo ' '

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

.PHONY: all clean dependents directories
.SECONDARY:
