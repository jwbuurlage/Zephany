#include <array>

#include "../streams/streams.hpp"

namespace Zephany
{

// TODO: deocuple partitioner and this guy which should simply construct a stream
// from a given partitioning. Assume global gives rise to good partitioning locally
template <typename TMatrix>
class StripPartitioner : public Zee::IterativePartitioner<TMatrix> {
  public:
    using TVal = TMatrix::value_type;
    using TIdx = TMatrix::index_type;

    StripPartitioner() : Zee::Partitioner() {}

    void partition(TMatrix& A) {
        A_ = A;

        // find good partitioning using some method -- not using this now
    }

    TMatrix& refine(TMatrix& A) override { ZeeAssert(A_ = A); }

  private:
    TMatrix& A_;
}:

} // namespace Zephany
