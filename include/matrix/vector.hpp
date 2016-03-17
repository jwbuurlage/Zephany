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
            : Base(size, defaultValue), owners_(size), elements_(size, defaultValue)
        { }

        // FIXME copied code again
        void reassign(TIdx element, TIdx processor) override {
            owners_[element] = processor;
        }

        const std::vector<TIdx>& getOwners() const { return owners_; }

        TVal operator[](TIdx i) const { return elements_[i]; }

        TVal& at(TIdx i) { return elements_[i]; }

    private:
        std::vector<TIdx> owners_;
        std::vector<TVal> elements_;
};

} // namespace zephany
