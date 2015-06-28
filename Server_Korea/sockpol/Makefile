
all : sockpol.exe
sockpol.exe : sockpol.cs
	gmcs sockpol.cs -out:sockpol.exe

.PHONY : clean
clean :
	rm -f sockpol.exe

.PHONY : rebuild
rebuild : clean all
