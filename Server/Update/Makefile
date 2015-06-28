all : Scan Upload Server

Scan : Src/Scan.cc Src/ConfigUtil.cc Src/VersionInfo.pb.cc
	g++ $^ -lprotobuf -lMD5 -o $@

Upload : Src/Upload.cc Src/ConfigUtil.cc Src/VersionInfo.pb.cc
	g++ $^ -lzmq -lprotobuf -l:/usr/local/lib/libuuid.a -lzip -lMD5 -o $@

Server : Src/Server.cc Src/ConfigUtil.cc Src/VersionInfo.pb.cc Src/CheckVersion.pb.cc
	g++ $^ -lzmq -lprotobuf -lMD5 -o $@
