#pragma once

#include <gpufmt/definitions.h>
#include <gpufmt/format.h>
#include <gpufmt/internal/format_traits_common.h>

namespace gpufmt {
[[nodiscard]] const FormatInfo& formatInfo(Format format) noexcept;

[[nodiscard]] SampleType formatSampleType(Format format) noexcept;
}