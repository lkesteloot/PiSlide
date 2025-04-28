
SRC = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
APP = pislide
RAYLIB_SRC = $(HOME)/others/raylib/src
CXXFLAGS = -std=c++23 -I $(RAYLIB_SRC) -Wmissing-field-initializers -g
LIBS = $(RAYLIB_SRC)/libraylib.a -lsqlite3
RUN_PREFIX = 

ROOT_DIR = /Users/lk/tmp/pislide-root
ROOT_DIR = /Users/lk/tmp/pislide-root-small

ifeq ($(HOSTTYPE), Linux-aarch64)
	RUN_PREFIX = sudo xinit
endif
ifeq ($(HOSTTYPE), Darwin-arm64)
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreFoundation -framework CoreAudio -framework CoreVideo -framework AudioToolbox
endif

.PHONY: run
run: $(APP)
	$(RUN_PREFIX) ./$(APP) --root-dir "$(ROOT_DIR)"

$(APP): $(SRC) $(HEADERS) Makefile
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(APP)

.PHONY: clean
clean:
	/bin/rm -f $(APP)

