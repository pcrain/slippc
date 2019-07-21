RM := rm -rf
MKDIR_P = mkdir -p
# INCLUDES := -I/usr/include/jsoncpp
# LIBS := -ljsoncpp

UNUSED := -Wno-unused-variable

HEADERS += \
src/parser.h \
src/replay.h \
src/util.h

OBJS += \
build/parser.o \
src/replay.o \
build/main.o

CPP_DEPS += \
build/parser.d \
src/replay.d \
build/main.d

OUT_DIR = build

# OLEVEL := -O0
# OLEVEL := -O3
OLEVEL := -Ofast -march=native -frename-registers -fno-signed-zeros -fno-trapping-math

all: directories slippc

slippc: $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/usr/lib -std=c++17 -o "./slippc" $(OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

build/%.o: ./src/%.cpp $(HEADERS)
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	$(LINK.c) $< -c -o $@
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ $(INCLUDES) $(OLEVEL) -g3 -Wall -c -fmessage-length=0 -std=c++17 $(UNUSED) -o "$@" "$<"
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
