
mcc -B csharedlib:libquad quad.m -v

mbuild quaddriver.c -lquad -L. -I. -I../../../PiOS.osx/inc/ -v

$CC -O -Wl,-twolevel_namespace -undefined error -dynamiclib -arch x86_64 -Wl,-syslibroot,/Developer/SDKs/MacOSX10.6.sdk -mmacosx-version-min=10.5 -Wl,-exported_symbols_list,/tmp/5462processed_export_list -install_name  "@loader_path/libquad.dylib" -framework CoreFoundation  -o libquad.dylib  libquad.o -L/Applications/MATLAB_R2011a.app/runtime/maci64  -lmwmclmcrrt 