CXX := g++
LD := g++

BIN := $(addprefix Build/Bin/, $(notdir $(shell pwd)))

INCDIR := -I/usr/local/include/thrift -I/usr/local/include/thrift/fb303
LIBDIR := 
LIBS := -lrt \
	-lpthread \
	-lprotobuf \
	-lzmq \
	-lmysqlclient \
	-lcurl \
	-lMD5 \
	-lboost_filesystem\
	-lthrift \
	-lscribe \
	-lfb303 \
#	-lIL \
	-ltinyxml2 \
	-lsoci_core \
	-lsoci_mysql \
	-lmemcached \
	-lcryptopp \
	-lboost-coroutine \
	-lConvertUTF

#CXXFLAGS := -std=c++0x -c -Wall -O3 $(INCDIR)
#CXXFLAGS := -c -Wall -g3 -pg $(INCDIR)
CXXFLAGS := -c -Wall -g3 $(INCDIR)
#LDFLAGS := $(LIBDIR) -rdynamic -Wl,-Map,$(BIN).map $(LIBS)
#LDFLAGS := $(LIBDIR) -rdynamic -pg -Wl,$(LIBS)
LDFLAGS := $(LIBDIR) -rdynamic -Wl,$(LIBS)

#SRCS := $(addprefix Src/, $(filter %.cc, $(shell ls Src)))
SRCS := $(shell find Src -name *.cc)
OBJS := $(addprefix Build/Obj/, $(notdir $(patsubst %.cc, %.o, $(SRCS))))

all : $(BIN)

$(BIN) : $(OBJS)
	$(LD) $^ $(LDFLAGS) -o $@
#	cp $(BIN) Build/Bin-Ours/

define BUILDO
OBJ := $(addprefix Build/Obj/, $(notdir $(patsubst %.cc, %.o, $(1))))
REQS := $(filter %.h %.hpp, $(shell gcc $(1) -MM $(CXXFLAGS)))
$$(OBJ) : $(1) $$(REQS)
	$(CXX) $$< $(CXXFLAGS) -o $$@
endef

$(foreach s, $(SRCS), $(eval $(call BUILDO, $(s))))


.PHONY : clean
clean :
	rm -f $(OBJS) $(BIN)

#.PHONY : install
#install :
#	cp -r $(BIN) /usr/local/bin
#	ldconfig

.PHONY : rebuild
rebuild : clean all
