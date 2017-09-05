OBJs := $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
DEPs := $(OBJs:%.o=%.d)
BIN  := portal

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
all: portal

portal:	$(OBJs)

.PHONY: clean
clean:
	@rm -fv portal $(OBJs) $(DEPs) *.o
	$(MAKE) -C gstvideo clean
#	$(MAKE) -C ahrs-visualizer clean
	