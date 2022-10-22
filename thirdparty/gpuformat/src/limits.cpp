#include <gpufmt/limits.h>
#include <gpufmt/visit.h>

namespace gpufmt {
    namespace internal {
        template<Format FormatValue>
        class ClampToLimitsHelper {
        public:
            using Traits = FormatTraits<FormatValue>;
            using Limits = FormatLimits<FormatValue>;

            template<class T>
            [[nodiscard]]
            static constexpr T clamp(Channel channel, T value) noexcept {
                if constexpr(std::is_same_v<Traits::ValueType, void>) {
                    return T(0);
                } else {
                    const auto castValue = static_cast<Traits::ValueType>(value);

                    if(castValue > Limits::max(channel)) {
                        if constexpr(sizeof(T) >= sizeof(Traits::ValueType)) {
                            return static_cast<T>(Limits::max(channel));
                        } else {
                            return static_cast<T>(std::min(Limits::max(channel), static_cast<Traits::ValueType>(std::numeric_limits<T>::max())));
                        }
                    } else if(castValue < Limits::lowest(channel)) {
                        if constexpr(sizeof(T) >= sizeof(Traits::ValueType)) {
                            return static_cast<T>(Limits::lowest(channel));
                        } else {
                            return static_cast<T>(std::max(Limits::lowest(channel), static_cast<Traits::ValueType>(std::numeric_limits<T>::lowest())));
                        }
                    } else {
                        return value;
                    }
                }
            }

            [[nodiscard]]
            constexpr float operator()(Channel channel, float value) const noexcept {
                return clamp(channel, value);
            }

            [[nodiscard]]
            constexpr uint32_t operator()(Channel channel, uint32_t value) const noexcept {
                return clamp(channel, value);
            }

            [[nodiscard]]
            constexpr int32_t operator()(Channel channel, int32_t value) const noexcept {
                return clamp(channel, value);
            }

        #ifndef GF_EXCLUDE_64BIT_FORMATS
            [[nodiscard]]
            constexpr double operator()(Channel channel, double value) const noexcept {
                return clamp(channel, value);
            }

            [[nodiscard]]
            constexpr uint64_t operator()(Channel channel, uint64_t value) const noexcept {
                return clamp(channel, value);
            }

            [[nodiscard]]
            constexpr int64_t operator()(Channel channel, int64_t value) const noexcept {
                return clamp(channel, value);
            }
        #endif
        };
    } // namespace internal

    float clampToFormatLimits(Format format, Channel channel, float value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }

    int32_t clampToFormatLimits(Format format, Channel channel, int32_t value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }

    uint32_t clampToFormatLimits(Format format, Channel channel, uint32_t value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }

#ifndef GF_EXCLUDE_64BIT_FORMATS
    double clampToFormatLimits(Format format, Channel channel, double value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }

    int64_t clampToFormatLimits(Format format, Channel channel, int64_t value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }

    uint64_t clampToFormatLimits(Format format, Channel channel, uint64_t value) noexcept {
        return visitFormat<internal::ClampToLimitsHelper>(format, channel, value);
    }
#endif
}