#include "../Coprocess.h"

char buf[2048];

int main() {
    using namespace System;
    using namespace Stream;

    BufferedWriter<PosixFile> sout(PosixFile(1));

    PosixFile a;
    Coprocess proc( "/bin/sh", { "sh", "-c", "ls -alF /" }, { 1_FD >>a }, { } );

    BufferedReader<PosixFile> ra( std::move(a) );
    
    size_t n = 0;
    while (0 != (n = ra.readline(buf, 2048, "\n")) ) {
        buf[n-1] = '\0';
        print(sout, "[stdout ", n, " ] ", buf, " [EOF]\n");
    }

    sout.flush();

    print(sout, "Process exit with: ", proc.wait(), ".\n");
    sout.flush();
    sout.close();

    return 0;
}

