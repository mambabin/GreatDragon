.PHONY : platform
platform :
	echo "make [install][install64][uninstall][uninstall64]"

.PHONY : install
install :
	cp -r Include/* /usr/local/include
	cp -r -P Lib/* /usr/local/lib
	cp -r -P Bin/* /usr/local/bin
	ldconfig

.PHONY : uninstall
uninstall :
	rm -r -f $(addprefix /usr/local/include/, $(shell ls Include))
	rm -r -f $(addprefix /usr/local/lib/, $(shell ls Lib))
	rm -r -f $(addprefix /usr/local/bin/, $(shell ls Bin))
	ldconfig

.PHONY : install64
install64 :
	cp -r Include/* /usr/local/include
	cp -r -P Lib64/* /usr/local/lib
	cp -r -P Bin64/* /usr/local/bin
	ldconfig

.PHONY : uninstall64
uninstall64 :
	rm -r -f $(addprefix /usr/local/include/, $(shell ls Include))
	rm -r -f $(addprefix /usr/local/lib/, $(shell ls Lib64))
	rm -r -f $(addprefix /usr/local/bin/, $(shell ls Bin64))
	ldconfig
