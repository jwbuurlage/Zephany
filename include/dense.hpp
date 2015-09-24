#include <zee.hpp>

namespace Zephany {

using namespace Zee;

template <typename TVal = default_scalar_type,
         typename TIdx = default_index_type>
class DStreamingMatrix
    : public DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx>
{
    public:
        using Base = DDenseMatrixBase<DStreamingMatrix<TVal, TIdx>, TVal, TIdx>;
        using Base::operator=;

        DStreamingMatrix(TIdx rows, TIdx cols)
            : Base(rows, cols) {
            resize(rows, cols);
        }

        explicit DStreamingMatrix(std::string file)
            : Base(0, 0)
        {
            matrix_market::load(file, *this);
        }

        //////////////////////////////////////////////////////////////////////////////
        // FIXME: this repeats DMatrix, do we actually want different storage here??
        // maybe inherit from DMatrix after all
        void resize(TIdx rows, TIdx cols) override
        {
            Base::resize(rows, cols);

            elements_.resize(rows);
            for (auto& row : elements_) {
                row.resize(cols);
            }
        }

        TVal& at(TIdx i, TIdx j)
        {
            ZeeAssert(i < this->rows_);
            ZeeAssert(j < this->cols_);
            return elements_[i][j];
        }
        //////////////////////////////////////////////////////////////////////////////

    private:
        // stored column major
        std::vector<std::vector<TVal>> elements_;
};

} // namespace Zephany
