#pragma once

#include <cctype>
#include <string_view>

namespace teximp
{
struct CaseInsensitiveCharTraits : public std::char_traits<char>
{
    [[nodiscard]] static char toUpper(char ch) { return (char)std::toupper((unsigned char)ch); }

    [[nodiscard]] static bool equal(char c1, char c2) { return toUpper(c1) == toUpper(c2); }

    [[nodiscard]] static bool lessThan(char c1, char c2) { return toUpper(c1) < toUpper(c2); }

    [[nodiscard]] static int compare(const char* s1, const char* s2, std::size_t n)
    {
        while(n-- != 0)
        {
            if(toUpper(*s1) < toUpper(*s2)) return -1;
            if(toUpper(*s1) > toUpper(*s2)) return 1;
            ++s1;
            ++s2;
        }
        return 0;
    }

    [[nodiscard]] static const char* find(const char* s, std::size_t n, char a)
    {
        auto const ua(toUpper(a));
        while(n-- != 0)
        {
            if(toUpper(*s) == ua) return s;
            s++;
        }
        return nullptr;
    }
};

template<class DstTraits, class CharT, class SrcTraits>
[[nodiscard]] constexpr std::basic_string_view<CharT, DstTraits>
charTraitsCast(const std::basic_string_view<CharT, SrcTraits> src) noexcept
{
    return {src.data(), src.size()};
}

template<class CharTraitsT>
[[nodiscard]] int caseInsensitiveCompare(std::basic_string_view<char, CharTraitsT> a,
                                         std::basic_string_view<char, CharTraitsT> b)
{
    return charTraitsCast<CaseInsensitiveCharTraits>(a).compare(charTraitsCast<CaseInsensitiveCharTraits>(b));
}

template<class CharTraitsT>
[[nodiscard]] bool caseInsensitiveEqual(std::basic_string_view<char, CharTraitsT> a,
                                        std::basic_string_view<char, CharTraitsT> b)
{
    return charTraitsCast<CaseInsensitiveCharTraits>(a) == charTraitsCast<CaseInsensitiveCharTraits>(b);
}

template<class CharTraitsT>
[[nodiscard]] bool caseInsensitiveEqual(std::basic_string_view<char, CharTraitsT> a, const char* b)
{
    return charTraitsCast<CaseInsensitiveCharTraits>(a) == b;
}

template<class EnumT>
[[nodiscard]] bool enumMaskTest(EnumT mask, EnumT value) noexcept
{
    static_assert(std::is_enum_v<EnumT>);

    return (static_cast<std::underlying_type_t<EnumT>>(mask) & static_cast<std::underlying_type_t<EnumT>>(value)) !=
           std::underlying_type_t<EnumT>(0);
}
} // namespace teximp