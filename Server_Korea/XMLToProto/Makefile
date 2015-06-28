GAMECORESRC := ../GameCore/Src

PBOBJS := $(shell find $(GAMECORESRC) -name *.pb.cc)

XMLToProto: Main.cc $(PBOBJS)
	g++ $^ -g -I $(GAMECORESRC) -lprotobuf -ltinyxml2 -o $@

.PHONY : install
install:
	cp ./XMLToProto /usr/local/bin/

install_local:
	cp ./XMLToProto /home/ghl/Project-1/Document/XML/
