extern "C" {
#include <host_bsp.h>
}


#include <zee.hpp>
using namespace Zee;

int main(int argc, char **argv)
{
    // Initialize the BSP system
    bsp_init("kernels/k_hello_world.srec", argc, argv);

    // Initialize the Epiphany system and load the binary
    bsp_begin(bsp_nprocs());

    // Run the program on the Epiphany cores
    ebsp_spmd();

    // Finalize
    bsp_end();

    ZeeLogInfo << "---" << endLog;

    int n = 16;

    DMatrix<> A(n, n);
    DMatrix<> B(n, n);

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            A.at(i, j) = (float)i;
            B.at(i, j) = (float)j;
        }

    DMatrix<> C(n, n);
    C = A * B;
    ZeeLogVar(C);

    return 0;
}
