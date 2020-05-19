RM := rm -rf
MKDIR_P = mkdir -p
# INCLUDES := -I/usr/include/jsoncpp
# LIBS := -ljsoncpp

UNUSED := -Wno-unused-variable

HEADERS += \
src/parser.h \
src/replay.h \
src/analyzer.h \
src/analysis.h \
src/enums.h \
src/util.h

OBJS += \
build-win/parser.o \
build-win/replay.o \
build-win/analyzer.o \
build-win/analysis.o \
build-win/main.o

CPP_DEPS += \
build-win/parser.d \
build-win/replay.d \
build-win/analyzer.d \
build-win/analysis.d \
build-win/main.d

OUT_DIR = build-win

OLEVEL := -O0
# OLEVEL := -O3
# OLEVEL := -Ofast -march=native -frename-registers -fno-signed-zeros -fno-trapping-math

all: directories slippc

slippc: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	x86_64-w64-mingw32-g++ -L/usr/x86_64-w64-mingw32/lib/ -std=c++17 -o "./slippc.exe" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

build-win/%.o: ./src/%.cpp $(HEADERS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(LINK.c) $< -c -o $@
	x86_64-w64-mingw32-g++ -D__GXX_EXPERIMENTAL_CXX0X__ $(INCLUDES) $(OLEVEL) -g3 -Wall -c -fmessage-length=0 -std=c++17 $(UNUSED) -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

clean:
	-$(RM) $(OBJS)$(C++_DEPS) ./slippc
	-@echo ' '

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

.PHONY: all clean dependents directories
.SECONDARY:
