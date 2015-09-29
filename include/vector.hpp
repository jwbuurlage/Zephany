namespace zephany {

using namespace Zee;


/*** VECTOR ***/
template <typename TVal = default_scalar_type,
         typename TIdx = default_index_type>
class DStreamingVector
    : public DVectorBase<DStreamingVector<TVal, TIdx>, TVal, TIdx>
{
    public:
        using Base = DVectorBase<DStreamingVector<TVal, TIdx>, TVal, TIdx>;
        using Base::operator=;

        DStreamingVector(TIdx size, TVal defaultValue = 0.0)
            : Base(size, defaultValue)
        { }
};

} // namespace zephany
