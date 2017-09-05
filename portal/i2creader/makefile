OBJs := $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
DEPs := $(OBJs:%.o=%.d)
BIN  := i2creader

# Use a modern language
CPPFLAGS += --std=c++0x

# Turn on "all" warnings
CPPFLAGS += -Wall

# Generate .d files with dependency info
CPPFLAGS += -MD -MP

# Debuggable and optimized.
CPPFLAGS += -O2

LDFLAGS += -lwiringPi -lpthread -lm

.PHONY: all
all: subprocesses i2creader

i2creader:	$(OBJs)

.PHONY: clean
clean:
	@rm -fv i2creader $(OBJs) $(DEPs) *.o

	
.PHONY: subprocesses
subprocesses:
