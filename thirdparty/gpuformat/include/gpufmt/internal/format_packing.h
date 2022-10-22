#pragma once

#include <gpufmt/internal/byte_packing.h>
#include <gpufmt/internal/format_traits_common.h>

namespace gpufmt::internal {
    //----------------------------------
    // BITS
    //----------------------------------
    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsUNorm(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsUNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsUNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsUNorm<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsUNorm<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsUNorm<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsSNorm(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsSNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsSNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsSNorm<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsSNorm<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsSNorm<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }

#ifndef GF_EXCLUDE_SCALED_FORMATS
    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsUScaled(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsUScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsUScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsUScaled<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsUScaled<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsUScaled<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsSScaled(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsSScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsSScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsSScaled<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsSScaled<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsSScaled<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }
#endif // GF_EXCLUDE_SCALED_FORMATS

    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsUInt(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_integral_v<typename Traits::ValueType>);
        static_assert(std::is_unsigned_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsUInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsUInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsUInt<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsUInt<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsUInt<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBitsSInt(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_integral_v<typename Traits::ValueType>);
        static_assert(std::is_signed_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackBitsSInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            unpackedValue.r = internal::unpackBitsSInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackBitsSInt<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackBitsSInt<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackBitsSInt<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value);
        }

        return unpackedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsUNorm(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsUNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsUNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsUNorm<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsUNorm<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsUNorm<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsSNorm(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);
        
        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsSNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsSNorm<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsSNorm<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsSNorm<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsSNorm<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }

#ifndef GF_EXCLUDE_SCALED_FORMATS
    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsUScaled(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsUScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsUScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsUScaled<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsUScaled<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsUScaled<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsSScaled(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsSScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsSScaled<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsSScaled<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsSScaled<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsSScaled<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }
#endif // GF_EXCLUDE_SCALED_FORMATS

    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsUInt(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_integral_v<typename Traits::ValueType>);
        static_assert(std::is_unsigned_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsUInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsUInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsUInt<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsUInt<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsUInt<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    constexpr typename FormatTraits<F>::BlockType formatPackBitsSInt(typename FormatTraits<F>::NarrowSampleType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_integral_v<typename Traits::ValueType>);
        static_assert(std::is_signed_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packBitsSInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value);
        } else {
            packedValue = internal::packBitsSInt<typename Traits::BlockType, Traits::info.redBitMask.width, Traits::info.redBitMask.offset>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue |= internal::packBitsSInt<typename Traits::BlockType, Traits::info.greenBitMask.width, Traits::info.greenBitMask.offset>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue |= internal::packBitsSInt<typename Traits::BlockType, Traits::info.blueBitMask.width, Traits::info.blueBitMask.offset>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue |= internal::packBitsSInt<typename Traits::BlockType, Traits::info.alphaBitMask.width, Traits::info.alphaBitMask.offset>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBits(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);

        if constexpr(Traits::info.normalized)
        {
            if constexpr(!Traits::info.isSigned) {
                //unorm
                return formatUnpackBitsUNorm<F>(value);
            } else {
                //snorm
                return formatUnpackBitsSNorm<F>(value);
            }
        }
#ifndef GF_EXCLUDE_SCALED_FORMATS
        else if constexpr(Traits::info.scaled) {
            if constexpr(!Traits::info.isSigned) {
                //uscaled
                return formatUnpackBitsUScaled<F>(value);
            } else {
                //sscaled
                return formatUnpackBitsSScaled<F>(value);
            }
        }
#endif // GF_EXCLUDE_SCALED_FORMATS
        else if constexpr(Traits::info.integer) {
            if constexpr(!Traits::info.isSigned) {
                //uint
                return formatUnpackBitsUInt<F>(value);
            } else {
                //sint
                return formatUnpackBitsSInt<F>(value);
            }
        }
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBits(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(Traits::info.packed);

        if constexpr(Traits::info.normalized)
        {
            if constexpr(!Traits::info.isSigned) {
                //unorm
                return formatPackBitsUNorm<F>(value);
            } else {
                //snorm
                return formatPackBitsSNorm<F>(value);
            }
        }
#ifndef GF_EXCLUDE_SCALED_FORMATS
        else if constexpr(Traits::info.scaled) {
            if constexpr(!Traits::info.isSigned) {
                //uscaled
                return formatPackBitsUScaled<F>(value);
            } else {
                //sscaled
                return formatPackBitsSScaled<F>(value);
            }
        }
#endif
        else if constexpr(Traits::info.integer) {
            if constexpr(!Traits::info.isSigned) {
                //uint
                return formatPackBitsUInt<F>(value);
            } else {
                //sint
                return formatPackBitsSInt<F>(value);
            }
        }
    }

    //----------------------------
    // BYTES
    //----------------------------
    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesUNorm(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackUNorm<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackUNorm<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackUNorm<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackUNorm<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackUNorm<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesSNorm(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackSNorm<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackSNorm<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackSNorm<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackSNorm<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackSNorm<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

#ifndef GF_EXCLUDE_SCALED_FORMATS
    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesUScaled(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackUScaled<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackUScaled<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackUScaled<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackUScaled<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackUScaled<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesSScaled(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackSScaled<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackSScaled<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackSScaled<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackSScaled<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackSScaled<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }
#endif

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesUInt(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_unsigned_v<typename Traits::ValueType>);
        static_assert(std::is_integral_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackUInt<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackUInt<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackUInt<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackUInt<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackUInt<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesSInt(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_signed_v<typename Traits::ValueType>);
        static_assert(std::is_integral_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackSInt<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackSInt<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackSInt<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackSInt<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackSInt<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytesFloat(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.floatingPoint);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::NarrowSampleType unpackedValue;

        if constexpr(Traits::info.componentCount == 1) {
            unpackedValue = internal::unpackFloat<typename Traits::BlockType>(value);
        } else {
            unpackedValue.r = internal::unpackFloat<typename Traits::BlockType::value_type>(value[Traits::info.redIndex]);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            unpackedValue.g = internal::unpackFloat<typename Traits::BlockType::value_type>(value[Traits::info.greenIndex]);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            unpackedValue.b = internal::unpackFloat<typename Traits::BlockType::value_type>(value[Traits::info.blueIndex]);
        }
        if constexpr(Traits::info.componentCount == 4) {
            unpackedValue.a = internal::unpackFloat<typename Traits::BlockType::value_type>(value[Traits::info.alphaIndex]);
        }

        return unpackedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesUNorm(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packUNorm<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packUNorm<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packUNorm<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packUNorm<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packUNorm<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesSNorm(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.normalized);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packSNorm<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packSNorm<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packSNorm<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packSNorm<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packSNorm<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

#ifndef GF_EXCLUDE_SCALED_FORMATS
    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesUScaled(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packUScaled<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packUScaled<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packUScaled<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packUScaled<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packUScaled<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesSScaled(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.scaled);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packSScaled<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packSScaled<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packSScaled<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packSScaled<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packSScaled<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }
#endif

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesUInt(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(!Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_unsigned_v<typename Traits::ValueType>);
        static_assert(std::is_integral_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packUInt<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packUInt<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packUInt<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packUInt<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packUInt<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesSInt(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.integer);
        static_assert(std::is_signed_v<typename Traits::ValueType>);
        static_assert(std::is_integral_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packSInt<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packSInt<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packSInt<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packSInt<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packSInt<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytesFloat(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);
        static_assert(Traits::info.isSigned);
        static_assert(Traits::info.floatingPoint);
        static_assert(std::is_floating_point_v<typename Traits::ValueType>);

        typename Traits::BlockType packedValue;

        if constexpr(Traits::info.componentCount == 1) {
            packedValue = internal::packFloat<typename Traits::BlockType>(value);
        } else {
            packedValue[Traits::info.redIndex] = internal::packFloat<typename Traits::BlockType::value_type>(value.r);
        }
        if constexpr(Traits::info.componentCount >= 2) {
            packedValue[Traits::info.greenIndex] = internal::packFloat<typename Traits::BlockType::value_type>(value.g);
        }
        if constexpr(Traits::info.componentCount >= 3) {
            packedValue[Traits::info.blueIndex] = internal::packFloat<typename Traits::BlockType::value_type>(value.b);
        }
        if constexpr(Traits::info.componentCount == 4) {
            packedValue[Traits::info.alphaIndex] = internal::packFloat<typename Traits::BlockType::value_type>(value.a);
        }

        return packedValue;
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpackBytes(typename FormatTraits<F>::BlockType value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);

        if constexpr(Traits::info.normalized)
        {
            if constexpr(!Traits::info.isSigned) {
                //unorm
                return formatUnpackBytesUNorm<F>(value);
            } else {
                //snorm
                return formatUnpackBytesSNorm<F>(value);
            }
        }
#ifndef GF_EXCLUDE_SCALED_FORMATS
        else if constexpr(Traits::info.scaled)
        {
            if constexpr(!Traits::info.isSigned) {
                //uscaled
                return formatUnpackBytesUScaled<F>(value);
            } else {
                //sscaled
                return formatUnpackBytesSScaled<F>(value);
            }
        }
#endif
        else if constexpr(Traits::info.integer) {
            if constexpr(!Traits::info.isSigned) {
                //uint
                return formatUnpackBytesUInt<F>(value);
            } else {
                //sint
                return formatUnpackBytesSInt<F>(value);
            }
        } else if constexpr(Traits::info.floatingPoint) {
            return formatUnpackBytesFloat<F>(value);
        }
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPackBytes(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;
        static_assert(!Traits::info.packed);

        if constexpr(Traits::info.normalized)
        {
            if constexpr(!Traits::info.isSigned) {
                //unorm
                return formatPackBytesUNorm<F>(value);
            } else {
                //snorm
                return formatPackBytesSNorm<F>(value);
            }
        }
#ifndef GF_EXCLUDE_SCALED_FORMATS
        else if constexpr(Traits::info.scaled)
        {
            if constexpr(!Traits::info.isSigned) {
                //uscaled
                return formatPackBytesUScaled<F>(value);
            } else {
                //sscaled
                return formatPackBytesSScaled<F>(value);
            }
        }
#endif
        else if constexpr(Traits::info.integer)
        {
            if constexpr(!Traits::info.isSigned) {
                //uint
                return formatPackBytesUInt<F>(value);
            } else {
                //sint
                return formatPackBytesSInt<F>(value);
            }
        }
        else if constexpr(Traits::info.floatingPoint) {
            return formatPackBytesFloat<F>(value);
        }
    }


    //---------------------
    // MAIN FUNCTIONS
    //---------------------
    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::NarrowSampleType formatUnpack(const typename FormatTraits<F>::BlockType &value) noexcept {
        using Traits = FormatTraits<F>;

        if constexpr(Traits::info.packed) {
            return formatUnpackBits<F>(value);
        } else {
            return formatUnpackBytes<F>(value);
        }
    }

    template<Format F>
    [[nodiscard]]
    constexpr typename FormatTraits<F>::BlockType formatPack(const typename FormatTraits<F>::NarrowSampleType &value) noexcept {
        using Traits = FormatTraits<F>;

        if constexpr(Traits::info.packed) {
            return formatPackBits<F>(value);
        } else {
            return formatPackBytes<F>(value);
        }
    }
}