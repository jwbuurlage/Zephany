namespace Zephany {

using namespace Zee;

template <typename TVal, typename TIdx>
class DStreamingSparseMatrix :
    public DSparseMatrixBase<
        DStreamingSparseMatrix<TVal, TIdx>, TVal, TIdx>
{
    public:
        using Base = DSparseMatrixBase<
            DStreamingSparseMatrix<TVal, TIdx>, TVal, TIdx>;

        DStreamingSparseMatrix(std::string file,
                TIdx procs = 0) :
            Base(file, procs)
        { }

        // STREAMS STRIPS WINDOWS...
};

} // namespace zephany
