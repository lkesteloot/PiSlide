
SRC = $(wildcard *.cpp)
APP = pislide
RAYLIB_SRC = $(HOME)/others/raylib/src
CXXFLAGS = -std=c++23 -I $(RAYLIB_SRC)
LIBS = $(RAYLIB_SRC)/libraylib.a
RUN_PREFIX = 

ifeq ($(HOSTTYPE), Linux-aarch64)
	RUN_PREFIX = sudo xinit
endif
ifeq ($(HOSTTYPE), Darwin-arm64)
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreAudio -framework CoreVideo -framework AudioToolbox
endif

.PHONY: run
run: $(APP)
	$(RUN_PREFIX) ./$(APP)

$(APP): $(SRC) Makefile
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(APP)

.PHONY: clean
clean:
	/bin/rm -f $(APP)

