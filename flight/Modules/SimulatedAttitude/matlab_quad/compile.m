
To build the library, from matlab call
mcc -B csharedlib:libquad quad.m -v

To compile the driver:
gcc-4.2 -c  -I. -I../../../PiOS.osx/inc/ -fno-common -no-cpp-precomp -arch x86_64 -isysroot /Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.5 -I/Applications/MATLAB_R2011a.app/extern/include -DUNIX -DX11  -O2 -DNDEBUG matlabdriver.c
gcc-4.2 -O -Wl,-twolevel_namespace -undefined error -dynamiclib -arch x86_64 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.5 -framework CoreFoundation  -L. -o libsimmodel.dylib  matlabdriver.o -L/Applications/MATLAB_R2011a.app/runtime/maci64  -lmwmclmcrrt  -lquad

To test the driver:
gcc test_lib.c -L . -lsimmodel -I ../../../PiOS.osx/inc/

To run:
DYLD_LIBRARY_PATH=.:/Applications/MATLAB_R2011a.app//runtime/maci64:/Applications/MATLAB_R2011a.app//bin/maci64:/Applications/MATLAB_R2011a.app//sys/os/maci64 ./a.out

To use with OpenPilot.elf
copy libsimmodel.dylib and libquad.dylib to build/sitl_osx
DYLD_LIBRARY_PATH=.:./build/sitl_osx/:/Applications/MATLAB_R2011a.app//runtime/maci64:/Applications/MATLAB_R2011a.app//bin/maci64:/Applications/MATLAB_R2011a.app//sys/os/maci64 ./build/sitl_osx/OpenPilot.elf