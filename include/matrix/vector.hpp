namespace Zephany {

using namespace Zee;

template <typename TVal = default_scalar_type,
         typename TIdx = default_index_type>
class DStreamingVector
    : public DVectorBase<DStreamingVector<TVal, TIdx>, TVal, TIdx>
{
    public:
        using Base = DVectorBase<DStreamingVector<TVal, TIdx>, TVal, TIdx>;
        using Base::operator=;

        DStreamingVector(TIdx size, TVal defaultValue = 0.0)
            : Base(size, defaultValue), owners_(size)
        { }

        void reassign(TIdx element, TIdx processor) override {
            owners_[element] = processor;
        }

        const std::vector<TIdx>& getOwners() const { return owners_; }

    private:
        std::vector<TIdx> owners_;
};

} // namespace zephany
