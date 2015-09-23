#include <zee.hpp>

using namespace Zee;

int main()
{
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
