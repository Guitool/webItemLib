CFLAGS=	-W -Wall -I./mongoose -pthread -g

all:
	OS=`uname`; \
	  test "$$OS" = Linux && LIBS="-ldl" ; \
	  $(CC) $(CFLAGS) webItemLib.c ./mongoose/mongoose.c  $$LIBS $(ADD) -o webItemLib;

MSVC    = e:/vc6
CL      = $(MSVC)/bin/cl
CLFLAGS = /MD /TC /nologo $(DBG) /W3 /DNO_SSL \
        /I$(MSVC)/include /I.. /Dsnprintf=_snprintf \
        /link /incremental:no /libpath:$(MSVC)/lib /machine:IX86

windows:
	$(CL) webItemLib.c ./mongoose/mongoose.c $(CLFLAGS)

clean:
	rm -rf hello *.exe *.dSYM *.obj
