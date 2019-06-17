FILE=skeleton
GLMDIR=../glm/
CLDIR=../

########
#   Directories
S_DIR=Source
B_DIR=Build

########
#   Output
EXEC=$(B_DIR)/$(FILE)

# default build settings
CC_OPTS=-c -pipe -Wno-switch -ggdb -g3 -fopenmp -lpthread -Wfatal-errors -lm
LN_OPTS= -fopenmp -lpthread  -lm
CC=g++-8

PLATFORM = $(shell uname -s)
ifeq ($(PLATFORM), Darwin)
	CC_OPTS += -framework OpenCL
	LN_OPTS += -framework OpenCL
else
	CC_OPTS += -lOpenCL
	LN_OPTS += -lOpenCL
endif

########
#       SDL options
SDL_CFLAGS := $(shell sdl2-config --cflags)
GLM_CFLAGS := -I$(GLMDIR)
CL_CFLAGS := -I$(CLDIR)
SDL_LDFLAGS := $(shell sdl2-config --libs)

########
#   This is the default action
all:Build


########
#   Object list
#
OBJ = $(B_DIR)/$(FILE).o


########
#   Objects
$(B_DIR)/$(FILE).o : $(S_DIR)/$(FILE).cpp $(S_DIR)/SDLauxiliary.h $(S_DIR)/TestModelH.h
	$(CC) $(CC_OPTS) -o $(B_DIR)/$(FILE).o $(S_DIR)/$(FILE).cpp $(SDL_CFLAGS) $(GLM_CFLAGS) $(CL_CFLAGS)


########
#   Main build rule
Build : $(OBJ) Makefile
	$(CC) $(LN_OPTS) -o $(EXEC) $(OBJ) $(SDL_LDFLAGS)


clean:
	rm -f $(B_DIR)/*
