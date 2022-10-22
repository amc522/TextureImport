#include <gpufmt/traits.h>
#include <gpufmt/visit.h>

namespace gpufmt {
namespace internal {
template<Format FormatValue>
class GetFormatInfoHelper {
public:
    [[nodiscard]]
    constexpr const FormatInfo &operator()() const noexcept {
        return FormatTraits<FormatValue>().info;
    }
};

template<Format FormatValue>
class GetFormatSampleTypeHelper {
public:
    using Traits = FormatTraits<FormatValue>;

    [[nodiscard]]
    constexpr SampleType operator()() const noexcept {
        if constexpr(std::is_same_v<Traits::ValueType, int32_t>) {
            return SampleType::Int32;
        }
        else if constexpr(std::is_same_v<Traits::ValueType, uint32_t>) {
            return SampleType::UInt32;
        }
        else if constexpr(std::is_same_v<Traits::ValueType, float>) {
            return SampleType::Float;
        }
#ifndef GF_EXCLUDE_64BIT_FORMATS
        else if constexpr(std::is_same_v<Traits::ValueType, int64_t>) {
            return SampleType::Int64;
        }
        else if constexpr(std::is_same_v<Traits::ValueType, uint64_t>) {
            return SampleType::UInt64;
        }
        else if constexpr(std::is_same_v<Traits::ValueType, double>) {
            return SampleType::Double;
        }
#endif
        else {
            return SampleType::None;
        }
    }
};
}

const FormatInfo& formatInfo(Format format) noexcept {
    return visitFormat<internal::GetFormatInfoHelper>(format);
}

SampleType formatSampleType(Format format) noexcept {
    return visitFormat<internal::GetFormatSampleTypeHelper>(format);
}
}