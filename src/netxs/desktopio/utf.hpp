// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

#include "intmath.hpp"
#include "unidata.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <charconv>
#include <optional>
#include <sstream>
#include <span>
#include <bitset>

namespace netxs
{
    using view = std::string_view;
    using text = std::string;
    using wide = std::wstring;
    using wiew = std::wstring_view;
    using wchr = wchar_t;
    using flux = std::stringstream;
    using utfx = uint32_t;
    using namespace std::literals;

    static constexpr auto whitespaces = " \n\r\t"sv;
    static constexpr auto alphabetic  = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"sv;
    static constexpr auto base64code  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr auto whitespace  = ' '; // '.';
    static constexpr auto emptyspace  = "\0"sv;
}

namespace netxs::utf
{
    using ctrl = unidata::cntrls::type;

    static constexpr auto replacement            = "\xEF\xBF\xBD"sv; // \uFFFD = 0xEF 0xBF 0xBD (efbfbd) "�"
    static constexpr auto replacement_code       = utfx{ 0x0000FFFD };
    static constexpr auto grapheme_cluster_limit = 31; // Limits the number of code points in a grapheme cluster to a number sufficient for any possible linguistic situation.
    static constexpr auto cluster_field_size     = 5;
    static constexpr auto wcwidth_field_size     = 2;

    // utf: Grapheme cluster decoded from UTF-8.
    struct prop : public unidata::unidata
    {
        size_t utf8len;
        bool   correct;
        size_t cpcount;
        utfx   cdpoint;

        constexpr
        prop(size_t size)
            : unidata(),
              utf8len{ size },
              correct{ faux },
              cpcount{ 0    },
              cdpoint{ 0    }
        { }
        prop(utfx code, size_t size)
            : unidata( code ),
              utf8len{ size },
              correct{ true },
              cpcount{ 0    },
              cdpoint{ code }
        { }
        constexpr
        prop(prop const& attr)
            : unidata(attr),
              utf8len{attr.utf8len},
              correct{attr.correct},
              cpcount{attr.cpcount},
              cdpoint{attr.cdpoint}
        { }

        auto combine(prop const& next)
        {
            if (next.utf8len && cpcount < grapheme_cluster_limit && next.allied(brgroup))
            {
                ucwidth  = std::max(ucwidth, next.ucwidth);
                utf8len += next.utf8len;
                cpcount += 1;
                return 0_sz;
            }
            else
            {
                return utf8len;
            }
        }
    };

    // utf: First byte based UTF-8 codepoint lengths.
    static constexpr int utf8lengths[] =
    {	//      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
        /* 0 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 1 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 2 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 3 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 4 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 5 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 6 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 7 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 8 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 9 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* A */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* B */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* C */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* D */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* E */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* F */ 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    } ;

    // utf: Codepoint iterator.
    struct cpit
    {
        using chrptr = view::const_pointer;

        chrptr textptr;
        size_t balance;
        size_t utf8len;

        cpit(view utf8)
            : textptr{ utf8.data() },
              balance{ utf8.size() },
              utf8len{ 0           }
        { }

        void redo(view utf8)
        {
            textptr = utf8.data();
            balance = utf8.size();
        }

        view rest()
        {
            return view{ textptr, balance };
        }

        void step()
        {
            textptr += utf8len;
            balance -= utf8len;
            utf8len = 0;
        }

        prop take()
        {
            utfx cp;
            utfx c2;
            utfx c3;
            utfx c4;

            if (balance)
            {
                auto data = reinterpret_cast<unsigned char const*>(textptr);
                cp = *data;
                switch (utf8lengths[cp])
                {
                    case 1:
                        utf8len = 1U;
                        break;
                    case 0:
                        return prop(utf8len = 1);
                        break;
                    case 2:
                        if (balance > 1)
                        {
                            if (c2 = *++data; (c2 & 0xC0) == 0x80)
                            {
                                utf8len = 2U;
                                cp = ((cp & 0b00011111) << 6)
                                    | (c2 & 0b00111111);
                            }
                            else return prop(utf8len = 1);
                        }
                        else return prop(utf8len = 1);

                        break;
                    case 3:
                        if (balance > 2)
                        {
                            if (c2 = *++data; (c2 & 0xC0) == 0x80)
                            {
                                if (c3 = *++data; (c3 & 0xC0) == 0x80)
                                {
                                    utf8len = 3U;
                                    cp = ((cp & 0b00001111) << 12)
                                        |((c2 & 0b00111111) << 6)
                                        | (c3 & 0b00111111);

                                    if (cp >= 0xfdd0 && (cp < 0xfdf0 || ((cp & 0xfffe) == 0xfffe)))
                                    {
                                        return prop(utf8len);
                                    }
                                }
                                else return prop(utf8len = 2);
                            }
                            else return prop(utf8len = 1);
                        }
                        else return prop(utf8len = balance);

                        break;
                    default: // case 4
                        if (balance > 3)
                        {
                            if (c2 = *++data; (c2 & 0xC0) == 0x80)
                            {
                                if (c3 = *++data; (c3 & 0xC0) == 0x80)
                                {
                                    if (c4 = *++data; (c4 & 0xC0) == 0x80)
                                    {
                                        utf8len = 4U;
                                        cp = ((cp & 0b00000111) << 18)
                                            |((c2 & 0b00111111) << 12)
                                            |((c3 & 0b00111111) << 6)
                                            | (c4 & 0b00111111);

                                        if (cp > 0x10ffff)
                                        {
                                            return prop(utf8len);
                                        }
                                    }
                                    else return prop(utf8len = 3);
                                }
                                else return prop(utf8len = 2);
                            }
                            else return prop(utf8len = 1);
                        }
                        else return	prop(utf8len = balance);
                        break;
                }
            }
            else return prop(utf8len = 0);

            return prop(cp, utf8len);
        }

        auto next()
        {
            auto cpoint = take();
                          step();
            return cpoint;
        }

        operator bool ()
        {
            return balance > 0;
        }
    };

    // utf: Grapheme cluster with properties.
    struct frag
    {
        view text;
        prop attr;

        auto letter(view utf8)
        {
            if (auto code = cpit{ utf8 })
            {
                auto next = code.take();
                do
                {
                    if (next.is_cmd())
                    {
                        //todo revise
                        //code.step();
                        return std::pair{ replacement, next };
                    }
                    else
                    {
                        auto head = code.textptr;
                        auto left = next;
                        do
                        {
                            code.step();
                            if (next.correct)
                            {
                                next = code.take();
                                if (auto size = left.combine(next))
                                {
                                    return std::pair{ view(head, size), left };
                                }
                            }
                            else
                            {
                                next.utf8len = left.utf8len;
                                return std::pair{ replacement, next };
                            }
                        }
                        while (true);
                    }
                }
                while (code);
            }
            return std::pair{ replacement, prop{ 0 } };
        }

        frag(view utf8, prop const& attr)
            : text{ utf8 },
              attr{ attr }
        { }
        frag(view utf8, utfx code)
            : text{ utf8 },
              attr{ code, utf8.size() }
        { }
        frag(std::pair<view, prop> p)
            : frag{ p.first, p.second }
        { }
        frag(view utf8)
            : frag{ letter(utf8) }
        { }
    };

    // utf: Return the first grapheme cluster and its Unicode attributes.
    auto letter(view utf8)
    {
        return frag{ utf8 };
    }

    // utf: Break the text into the enriched grapheme clusters.
    //      Forward the result to the dest using the "serve" and "yield" lambdas.
    //      serve: Handle escaped control sequences.
    //             auto s = [&](utf::prop const& traits, view utf8) -> view;
    //      yield: Handle grapheme clusters.
    //             auto y = [&](frag const& cluster){};
    //      Clusterize: parse by grapheme clusters (true) or codepoints (faux)
    template<bool Clusterize = true, class S, class Y>
    void decode(S serve, Y yield, view utf8, si32& decsg)
    {
        static const auto dec_sgm_lookup = std::vector<frag> // DEC Special Graphics mode lookup table.
        {//  _      `      a      b       c       d       e      f      g      h       i       j      k      l      m      n     o       p      q      r      s      t      u      v      w      x      y      z      {      |      }      ~                                 };
            "w"sv, "♦"sv, "▒"sv, "␉"sv, "␌"sv, "␍"sv, "␊"sv, "°"sv, "±"sv, "␤"sv, "␋"sv, "┘"sv, "┐"sv, "┌"sv, "└"sv, "┼"sv, "⎺"sv, "⎻"sv, "─"sv, "⎼"sv, "⎽"sv, "├"sv, "┤"sv, "┴"sv, "┬"sv, "│"sv, "≤"sv, "≥"sv, "π"sv, "≠"sv, "£"sv, "·"sv,
        };
        if (auto code = cpit{ utf8 })
        {
            auto next = code.take();
            do
            {
                if (next.is_cmd())
                {
                    code.step();
                    auto rest = code.rest();
                    auto chars = serve(next, rest);
                    code.redo(chars);
                    next = code.take();
                }
                else
                {
                    if (decsg && next.cdpoint <= 0x7e && next.cdpoint >= 0x5f) // ['_' - '~']
                    {
                        yield(dec_sgm_lookup[next.cdpoint - 0x5f]);
                        code.step();
                        next = code.take();
                    }
                    else if constexpr (Clusterize)
                    {
                        auto head = code.textptr;
                        auto left = next;
                        do
                        {
                            code.step();
                            if (next.correct)
                            {
                                next = code.take();
                                if (auto size = left.combine(next))
                                {
                                    auto crop = frag{ view(head, size), left };
                                    yield(crop);
                                    break;
                                }
                            }
                            else
                            {
                                next.utf8len = left.utf8len;
                                auto crop = frag{ replacement, next };
                                yield(crop);
                                next = code.take();
                                break;
                            }
                        }
                        while (true);
                    }
                    else
                    {
                        if (next.correct)
                        {
                            auto crop = frag{ view(code.textptr,next.utf8len), next };
                            yield(crop);
                        }
                        else
                        {
                            auto crop = frag{ replacement, next };
                            yield(crop);
                        }
                        code.step();
                        next = code.take();
                    }
                }
            }
            while (code);
        }
    }

    struct qiew : public view
    {
        using span = std::span<char>;

        struct hash
        {
            auto operator()(qiew key) const { return std::hash<view>{}(key); }
        };
        struct equal
        {
            auto operator()(qiew lhs, qiew rhs) const { return lhs.compare(rhs) == 0; }
        };

        constexpr qiew() noexcept : view() { }
        constexpr qiew(span const& v) noexcept : view(v.data(), v.size()) { }
        constexpr qiew(view const& v) noexcept : view(v) { }
                  qiew(text const& v) noexcept : view(v) { }
                  qiew(char const& v) noexcept : view(&v, 1) { }
        template<class T, class ...Args>
        constexpr qiew(T* ptr, Args&&... len) noexcept : view(ptr, std::forward<Args>(len)...) { }
        constexpr qiew& operator = (qiew const&) noexcept = default;

                 operator text () const { return text{ data(), size() }; }
        explicit operator bool () const { return view::length(); }

        // qiew: Clear.
        constexpr auto clear()
        {
            view::operator=({});
        }
        // qiew: Return substring.
        constexpr auto substr(size_t offset = 0, size_t count = npos) const
        {
            return qiew{ view::substr(offset, count) };
        }
        // qiew: Convert to text.
        auto str() const { return operator text(); }
        // qiew: Peek front char.
        si32 front() const { return static_cast<unsigned char>(view::front()); }
        // qiew: Pop front.
        auto pop_front()
        {
            auto c = view::front();
            view::remove_prefix(1);
            return c;
        }
        // qiew: Pop the front sequence of the same control points and return their count + 1.
        auto pop_all(ctrl cmd)
        {
            auto n = si32{ 1 };
            auto next = utf::letter(*this);
            while (next.attr.control == cmd)
            {
                view::remove_prefix(next.attr.utf8len);
                next = utf::letter(*this);
                n++;
            }
            return n;
        }
        // qiew: Pop the front sequence of the same chars and return their count + 1.
        auto pop_all(char c)
        {
            auto n = si32{ 1 };
            while (length() && view::front() == c)
            {
                view::remove_prefix(1);
                n++;
            }
            return n;
        }
        // qiew: Return true and pop the front control point when it is equal to cmd.
        auto pop_if(ctrl cmd)
        {
            auto next = utf::letter(*this);
            if (next.attr.control == cmd)
            {
                view::remove_prefix(next.attr.utf8len);
                return true;
            }
            return faux;
        }
        // qiew: Return true and pop the front char when it is equal to c.
        auto pop_if(char c)
        {
            if (length() && view::front() == c)
            {
                view::remove_prefix(1);
                return true;
            }
            return faux;
        }
    };

    template<class A = si32, si32 Base = 10, class View, class = std::enable_if_t<std::is_base_of<view, View>::value == true, View>>
    inline std::optional<A> to_int(View& ascii)
    {
        auto num = A{};
        auto top = ascii.data();
        auto end = ascii.length() + top;

        if (auto [pos, err] = std::from_chars(top, end, num, Base); err == std::errc())
        {
            ascii.remove_prefix(pos - top);
            return num;
        }
        else return std::nullopt;
    }
    template<class A = si32, si32 Base = 10, class T, class = std::enable_if_t<std::is_base_of<view, T>::value == faux, T>>
    inline auto to_int(T&& utf8)
    {
        auto shadow = view{ std::forward<T>(utf8) };
        return to_int<A, Base>(shadow);
    }
    template<si32 Base = 10, class T, class A>
    inline auto to_int(T&& utf8, A fallback)
    {
        auto result = to_int<A, Base>(std::forward<T>(utf8));
        return result ? result.value() : fallback;
    }

    struct letter_sync
    {
        int lb;
        int rb;
        int cp;
    };
    void capacity(auto& target, auto additional)
    {
        auto required = target.size() + additional;
        if (target.capacity() < required) target.reserve(std::max(target.size(), additional));
    }
    void to_utf(char const* utf8, size_t size, wide& wide_text)
    {
        // � The standard also recommends replacing each error with the replacement character.
        //
        // In terms of the newline, Unicode introduced U+2028 LINE SEPARATOR
        // and U+2029 PARAGRAPH SEPARATOR.
        //  c̳̻͚̻̩̻͉̯̄̏͑̋͆̎͐ͬ͑͌́͢h̵͔͈͍͇̪̯͇̞͖͇̜͉̪̪̤̙ͧͣ̓̐̓ͤ͋͒ͥ͑̆͒̓͋̑́͞ǎ̡̮̤̤̬͚̝͙̞͎̇ͧ͆͊ͅo̴̲̺͓̖͖͉̜̟̗̮̳͉̻͉̫̯̫̍̋̿̒͌̃̂͊̏̈̏̿ͧ́ͬ̌ͥ̇̓̀͢͜s̵̵̘̹̜̝̘̺̙̻̠̱͚̤͓͚̠͙̝͕͆̿̽ͥ̃͠͡
        capacity(wide_text, size);
        auto code = utfx{};
        auto tail = utf8 + size;
        while (utf8 < tail)
        {
            auto c = static_cast<unsigned char>(*utf8++);

                 if (c < 0x80) code = c;
            else if (c < 0xc0) code =(c & 0x3f) | code << 6;
            else if (c < 0xe0) code = c & 0x1f;
            else if (c < 0xf0) code = c & 0x0f;
            else               code = c & 0x07;

            if (code <= 0x10ffff)
            {
                if (utf8 == tail || (*utf8 & 0xc0) != 0x80)
                {
                    if (code < 0xd800 || (code >= 0xe000 && code <= 0xffff) || sizeof(wchr) > 2) // single | wchr == char32_t
                    {
                        wide_text.push_back(static_cast<wchr>(code));
                    }
                    else if (code > 0xffff) // surrogate pair
                    {
                        wide_text.append({ static_cast<wchr>(0xd800 + ((code - 0x10000) >> 10)),
                                           static_cast<wchr>(0xdc00 + (code & 0x03ff)) });
                    }
                }
            }
            else wide_text.push_back(replacement_code);
        }
        //wide_text.shrink_to_fit();
    }
    template<template<class...> class TextOrView, class ...Args>
    void to_utf(TextOrView<char, Args...> const& utf8, wide& wide_text)
    {
        to_utf(utf8.data(), utf8.size(), wide_text);
    }
    wide to_utf(char const* utf8, size_t size)
    {
        auto wide_text = wide{};
        to_utf(utf8, size, wide_text);
        return wide_text;
    }
    inline utfx tocode(wchr c)
    {
        utfx code;
        if (c >= 0xd800 && c <= 0xdbff)
        {
            code = ((c - 0xd800) << 10) + 0x10000;
        }
        else
        {
            if (c >= 0xdc00 && c <= 0xdfff) code = c - 0xdc00;
            else                            code = c;
        }
        return code;
    }
    // Return faux only on first part of surrogate pair.
    inline bool tocode(wchr c, utfx& code)
    {
        auto first_part = c >= 0xd800 && c <= 0xdbff;
        if (first_part) // First part of surrogate pair.
        {
            code = ((c - 0xd800) << 10) + 0x10000;
        }
        else
        {
            if (c >= 0xdc00 && c <= 0xdfff) // Second part of surrogate pair.
            {
                if (code) code |= c - 0xdc00;
                else      code = utf::replacement_code; // Broken pair.
            }
            else code = c;
        }
        return !first_part;
    }

    namespace
    {
        inline void _to_utf(text& utf8, utfx code)
        {
            if (code <= 0x007f)
            {
                utf8.push_back(static_cast<char>(code));
            }
            else if (code <= 0x07ff)
            {
                utf8.push_back(static_cast<char>(0xc0 | ((code >> 0x06) & 0x1f)));
                utf8.push_back(static_cast<char>(0x80 | ( code          & 0x3f)));
            }
            else if (code <= 0xffff)
            {
                utf8.push_back(static_cast<char>(0xe0 | ((code >> 0x0c) & 0x0f)));
                utf8.push_back(static_cast<char>(0x80 | ((code >> 0x06) & 0x3f)));
                utf8.push_back(static_cast<char>(0x80 | ( code          & 0x3f)));
            }
            else
            {
                utf8.push_back(static_cast<char>(0xf0 | ((code >> 0x12) & 0x07)));
                utf8.push_back(static_cast<char>(0x80 | ((code >> 0x0c) & 0x3f)));
                utf8.push_back(static_cast<char>(0x80 | ((code >> 0x06) & 0x3f)));
                utf8.push_back(static_cast<char>(0x80 | ( code          & 0x3f)));
            }
        }
    }
    auto to_utf_from_code(utfx code)
    {
        auto utf8 = text{};
        _to_utf(utf8, code);
        return utf8;
    }
    void to_utf_from_code(utfx code, text& utf8_out)
    {
        _to_utf(utf8_out, code);
    }
    void to_utf(wchr const* wide_text, size_t size, text& utf8)
    {
        capacity(utf8, size * 3/*worst case*/);
        auto code = utfx{ 0 };
        auto tail = wide_text + size;
        while (wide_text < tail)
        {
            auto c = *wide_text++;
            if (c >= 0xd800 && c <= 0xdbff)
            {
                code = ((c - 0xd800) << 10) + 0x10000;
            }
            else
            {
                if (c >= 0xdc00 && c <= 0xdfff) code |= c - 0xdc00;
                else                            code  = c;

                _to_utf(utf8, code);

                code = 0;
            }
        }
    }
    template<template<class...> class WideTextOrView, class ...Args>
    void to_utf(WideTextOrView<wchr, Args...> const& wide_text, text& utf8)
    {
        to_utf(wide_text.data(), wide_text.size(), utf8);
    }
    void to_utf(std::span<wchr> wide_text, text& utf8)
    {
        to_utf(wide_text.data(), wide_text.size(), utf8);
    }
    text to_utf(wchr const* wide_text, size_t size)
    {
        auto utf8 = text{};
        to_utf(wide_text, size, utf8);
        return utf8;
    }
    template<class WideOrUTF8>
    auto to_utf(WideOrUTF8&& str)
    {
        return to_utf(str.data(), str.size());
    }
    template<class T>
    auto to_utf(T* c_str)
    {
        auto iter = c_str;
        while (*iter != 0) { ++iter; }
        return to_utf(c_str, iter - c_str);
    }
    auto to_utf(wchr wc)
    {
        return to_utf(&wc, 1);
    }
    void to_utf(wchr wc, text& utf8)
    {
        to_utf(&wc, 1, utf8);
    }
    auto to_utf(wchr wc, wide& pair, text& utf8)
    {
        if (wc >= 0xd800 && wc <= 0xdbff) // First part of surrogate pair.
        {
            pair.clear();
            pair.push_back(wc);
            return 0_sz;
        }
        else
        {
            auto step = utf8.size();
            if (wc >= 0xdc00 && wc <= 0xdfff) // Second part of surrogate pair.
            {
                if (pair.empty()) // Broken surrogate pair.
                {
                    utf8 += utf::replacement;
                }
                else
                {
                    pair.push_back(wc);
                    to_utf(pair, utf8);
                    pair.clear();
                }
            }
            else to_utf(wc, utf8);

            return utf8.size() - step;
        }
    }
    template<class Result = si32>
    auto length(view utf8)
    {
        auto length = Result{ 0 };
        for (auto c : utf8)
        {
            length += (c & 0xc0) != 0x80;
        }
        return length;
    }
    // utf: Check utf-8 integrity (last codepoint) and cut off the invalid bytes at the end.
    void purify(view& utf8)
    {
        if (auto size = utf8.size())
        {
            auto is_first = [](auto c) { return (c & 0xc0) != 0x80; };
            auto first = faux;

            while (size && !(first = is_first(utf8[--size]))) // Find first byte.
            { }

            if (first) // Check codepoint.
            {
                auto l = utf::letter(utf8.substr(size));
                if (!l.attr.correct)
                {
                    utf8 = utf8.substr(0, size);
                }
            }
            else // Bad UTF-8 encoding (size == 0).
            {
                //Recycle all bad bytes (log?).
            }
        }
    }
    auto substr(qiew utf8, size_t start, size_t length = text::npos)
    {
        if (length == 0) return qiew{};
        auto head = utf8.data();
        auto stop = head + utf8.size();
        auto calc = [](auto& it, auto& count, auto limit)
        {
            while (it != limit)
            {
                if ((*it & 0xc0) != 0x80)
                {
                    if (!count--) break;
                }
                ++it;
            }
        };

        if (start) calc(head, start, stop);

        start = head - utf8.data();
        if (length != text::npos)
        {
            auto tail = head;
            calc(tail, length, stop);
            return utf8.substr(start, tail - head);
        }
        else return utf8.substr(start);
    }
    auto repeat(view what, size_t count)
    {
        auto result = text{};
        result.reserve(what.length() * count);
        while (count--)
        {
            result += what;
        }
        return result;
    }
    auto repeat(char letter, size_t count)
    {
        return text(count, letter);
    }
    template<class TextOrView, class C>
    auto remove(TextOrView&& from, C&& what)
    {
        auto _what = view{ what };
        auto s_size = from.size();
        auto c_size =_what.size();
        if (c_size)
        {
            while (s_size >= c_size && from.substr(s_size - c_size, c_size) == what)
            {
                s_size -= c_size;
            }
        }
        return from.substr(0, s_size);
    }
    template<class W, class R>
    void change(text& utf8, W const& what, R const& replace)
    {
        auto frag = view{ what };
        auto fill = view{ replace };
        auto spot = 0_sz;
        auto line_sz = utf8.length();
        auto what_sz = frag.length();
        auto repl_sz = fill.length();

        if (!what_sz || line_sz < what_sz) return;

        if (what_sz == repl_sz)
        {
            while ((spot = utf8.find(frag, spot)) != text::npos)
            {
                utf8.replace(spot, what_sz, fill);
                spot += what_sz;
            }
        }
        else
        {
            auto last = 0_sz;
            if (what_sz < repl_sz)
            {
                auto temp = text{};
                temp.reserve((line_sz / what_sz + 1) * repl_sz); // In order to avoid reallocations.
                auto shadow = view{ utf8 };
                while ((spot = utf8.find(frag, last)) != text::npos)
                {
                    temp += shadow.substr(last, spot - last);
                    temp += fill;
                    spot += what_sz;
                    last  = spot;
                }
                temp += shadow.substr(last);

                utf8 = temp; // Assign to perform simultaneous shrinking and copying.
            }
            else
            {
                auto base = utf8.data();
                auto repl = fill.data();
                auto dest = base;
                auto copy = [](auto base, auto& dest, auto size)
                {
                    auto stop = base + size;
                    while (base != stop) { *dest++ = *base++; }
                };
                while ((spot = utf8.find(frag, last)) != text::npos)
                {
                    if (last) copy(base + last, dest, spot - last);
                    else      dest += spot;

                    copy(repl, dest, repl_sz);

                    spot += what_sz;
                    last  = spot;
                }

                copy(base + last, dest, line_sz - last);

                utf8.resize(dest - base);
            }
        }
    }
    template<class W, class R>
    auto change(qiew utf8, W const& what, R const& replace)
    {
        auto crop = utf8.str();
        change(crop, what, replace);
        return crop;
    }
    struct filler
    {
        std::map<text, text> dict;

        auto& operator [] (text const& s)
        {
            return dict[s];
        }
        auto operator () (text s)
        {
            for (auto& var : dict) utf::change(s, var.first, var.second);
            for (auto& var : dict) utf::change(s, var.first, var.second);
            return s;
        }
    };
    template<class TextOrView, class T>
    auto remain(TextOrView&& utf8, T const& delimiter, bool lazy = true)
    {
        auto crop = std::remove_cvref_t<TextOrView>{};
        auto what = view{ delimiter };
        auto coor = lazy ? utf8.find(what) : utf8.rfind(what);
        if (coor != text::npos)
        {
            crop = utf8.substr(coor + what.size(), text::npos);
        }
        return crop;
    }
    template<class TextOrView>
    auto remain(TextOrView&& utf8, char delimiter = '.', bool lazy = true)
    {
        auto what = view{ &delimiter, 1 };
        return remain(std::forward<TextOrView>(utf8), what, lazy);
    }
    // utf: Return left substring (from begin) until delimeter (lazy=faux: from left, true: from right).
    template<class T>
    T cutoff(T const& txt, T const& delimiter = T{ '.' }, bool lazy = true)
    {
        return txt.substr(0, lazy ? txt.find(delimiter) : txt.rfind(delimiter));
    }
    template<class T>
    T cutoff(T const& txt, char delimiter, bool lazy = true)
    {
        return txt.substr(0, lazy ? txt.find(delimiter) : txt.rfind(delimiter));
    }
    template<class T>
    inline T domain(T const& txt)
    {
        return remain(txt);
    }
    template<class TextOrView, class F>
    auto adjust(TextOrView&& utf8, size_t required_width, F const& fill_char, bool right_aligned = faux)
    {
        auto crop = text{};
        auto data = view{ utf8 };
        if (data.empty())
        {
            crop = repeat(fill_char, required_width);
        }
        else
        {
            if (required_width > 0)
            {
                crop = substr(data, 0, required_width);
            }
            auto size = length<size_t>(crop);
            if (required_width > size)
            {
                if (right_aligned) crop = repeat(fill_char, required_width - size) + crop;
                else               crop = crop + repeat(fill_char, required_width - size);
            }
        }
        return crop;
    }
    template<class Int_t, class T = char>
    text format(Int_t number, size_t by_group = 3, T const& delimiter = ' ')
    {
        if (by_group)
        {
            auto utf8 = std::to_string(std::abs(number));
            auto size = utf8.length();
            auto coor = (size - 1) % by_group + 1;
            auto crop = utf8.substr(0, coor);

            for (; coor < size; coor += by_group)
            {
                crop += delimiter;
                crop += utf8.substr(coor, by_group);
            }

            return number < 0 ? '-' + crop : crop;
        }
        else return std::to_string(number);
    }
    template<class T, int L = std::numeric_limits<T>::digits>
    auto to_bin(T n)
    {
        return std::bitset<L>(n).to_string();
    }
    template<bool UpperCase = faux, class V, class = typename std::enable_if<std::is_integral<V>::value>::type>
    auto to_hex(V number, size_t width = sizeof(V) * 2, char filler = '0')
    {
        static constexpr auto nums = UpperCase ? "0123456789ABCDEF"
                                               : "0123456789abcdef";
        auto crop = text(width, filler);
        auto head = crop.begin();
        auto tail = head + width;
        auto part = -4 + 4*width;
        while (head != tail)
        {
            *head++ = nums[(number >> part) & 0x0f];
             part  -= 4;
        }
        return crop;
    }
    template<class T, class ...Args>
    auto to_hex(T* ptr, Args&&... args)
    {
        return to_hex(reinterpret_cast<std::uintptr_t>(ptr), std::forward<Args>(args)...);
    }
    // utf: to_hex without allocations (the crop should has a reserved capacity).
    template<bool UpperCase = faux, class V, class = typename std::enable_if<std::is_integral<V>::value>::type>
    auto to_hex(text& crop, V number, size_t width = sizeof(V) * 2)
    {
        static constexpr auto nums = UpperCase ? "0123456789ABCDEF"
                                               : "0123456789abcdef";
        auto part = -4 + 4 * width;
        while (width--)
        {
            crop.push_back(nums[(number >> part) & 0x0f]);
            part -= 4;
        }
        return crop;
    }
    template<bool UpperCase = faux>
    auto to_hex(view buffer, bool formatted = faux)
    {
        if (formatted)
        {
            auto size = buffer.size();
            auto addr = 0_sz;
            auto crop = text{};

            while (addr < size)
            {
                auto frag = (size - addr > 16) ? 16
                                               : size - addr;
                crop += adjust(std::to_string(addr), 4, '0', true);

                for (auto i = 0_sz; i < 16; i++)
                {
                    if (i % 8 == 0)
                    {
                        crop += i < frag ? " -" : "  ";
                    }

                    crop += i < frag ? ' ' + to_hex<UpperCase>(buffer[addr + i], 2, true)
                                     : "   ";
                }

                crop += "   ";

                for (auto i = addr; i < addr + frag; i++)
                {
                    auto c = buffer[i];
                    crop += (c < 33) ? '.' : c;
                }

                crop += '\n';
                addr += 16;
            }
            return crop;
        }
        else
        {
            static constexpr auto nums = UpperCase ? "0123456789ABCDEF"
                                                   : "0123456789abcdef";
            auto size = buffer.size() << 1;
            auto buff = buffer.begin();
            auto crop = text(size, '0');
            auto head = crop.begin();
            auto tail = head + size;
            while (head != tail)
            {
                *head++ = nums[(*buff >> 4) & 0x0f];
                *head++ = nums[(*buff)      & 0x0f];
                buff++;
            }
            return crop;
        }
    }
    template<feed Dir = feed::fwd, bool SkipEmpty = faux, class V1, class P, bool Plain = std::is_same_v<void, std::invoke_result_t<P, view>>>
    auto divide(V1 const& utf8, char delimiter, P proc)
    {
        if constexpr (Dir == feed::fwd)
        {
            auto cur = 0_sz;
            auto pos = 0_sz;
            while ((pos = utf8.find(delimiter, cur)) != V1::npos)
            {
                auto frag = view{ utf8.data() + cur, pos - cur };
                if constexpr (SkipEmpty) if (frag.empty()) { cur = pos + 1; continue; }
                if constexpr (Plain) proc(frag);
                else            if (!proc(frag)) return;
                cur = pos + 1;
            }
            auto end = view{ utf8.data() + cur, utf8.size() - cur };
            if constexpr (SkipEmpty) if (end.empty()) return;
            proc(end);
        }
        else
        {
            auto cur = utf8.size();
            auto pos = utf8.size();
            while (cur && (pos = utf8.rfind(delimiter, cur - 1)) != V1::npos)
            {
                auto next = pos + 1;
                auto frag = view{ utf8.data() + next, cur - next };
                if constexpr (SkipEmpty) if (frag.empty()) { cur = pos; continue; }
                if constexpr (Plain) proc(frag);
                else            if (!proc(frag)) return;
                cur = pos;
            }
            auto end = view{ utf8.data(), cur };
            if constexpr (SkipEmpty) if (end.empty()) return;
            proc(end);
        }
    }
    template<feed Dir = feed::fwd, bool SkipEmpty = faux, class V1, class V2>
    auto divide(V1 const& utf8, V2 const& delimiter, std::vector<view>& crop)
    {
        auto mark = qiew(delimiter);
        if (auto len = mark.size())
        {
            auto num = 0_sz;
            auto cur = 0_sz;
            auto pos = 0_sz;
            while ((pos = utf8.find(mark, cur)) != V1::npos)
            {
                ++num;
                cur = pos + len;
            }
            crop.reserve(++num);
            cur = 0_sz;
            pos = 0_sz;
            while ((pos = utf8.find(mark, cur)) != V1::npos)
            {
                auto frag = view{ utf8.data() + cur, pos - cur };
                if (!SkipEmpty || !frag.empty()) crop.push_back(frag);
                cur = pos + len;
            }
            auto end = view{ utf8.data() + cur, utf8.size() - cur };
            if (!SkipEmpty || !end.empty()) crop.push_back(end);
        }
        return crop;
    }
    template<feed Dir = feed::fwd, bool SkipEmpty = faux, class V1, class V2>
    auto divide(V1 const& utf8, V2 const& delimiter)
    {
        auto crop = std::vector<view>{};
        divide<Dir, SkipEmpty>(utf8, delimiter, crop);
        return crop;
    }
    template<class Container>
    auto maxlen(Container const& set)
    {
        auto len = 0_sz;
        for (auto& obj : set)
        {
            auto val = utf::length(obj);
            if (val > len) len = val;
        }
        return len;
    }
    template<class ...Args>
    auto concat(Args&&... args)
    {
        return (flux{} << ... << std::forward<Args>(args)).str();
    }
    auto base64(view utf8)
    {
        auto code = base64code;
        auto data = text{};
        if (auto size = utf8.size())
        {
            data.resize(((size + 2) / 3) << 2);
            auto iter = utf8.begin();
            auto tail = utf8.end();
            auto dest = data.begin();
            do
            {
                auto crop = (unsigned char)*iter++ << 16;
                if (iter != tail) //todo move ifs to the outside of loop (optimization)
                {
                    crop += (unsigned char)*iter++ << 8;
                }
                else
                {
                    *dest++ = code[0x3F & crop >> 18];
                    *dest++ = code[0x3F & crop >> 12];
                    *dest++ = '=';
                    *dest++ = '=';
                    break;
                }
                if (iter != tail)
                {
                    crop += (unsigned char)*iter++;
                }
                else
                {
                    *dest++ = code[0x3F & crop >> 18];
                    *dest++ = code[0x3F & crop >> 12];
                    *dest++ = code[0x3F & crop >>  6];
                    *dest++ = '=';
                    break;
                }
                *dest++ = code[0x3F & crop >> 18];
                *dest++ = code[0x3F & crop >> 12];
                *dest++ = code[0x3F & crop >>  6];
                *dest++ = code[0x3F & crop      ];
            }
            while (iter != tail);
        }
        return data;
    }
    auto unbase64(view bs64, text& data)
    {
        auto code = base64code;
        auto is64 = [](auto c) { return (c > 0x2E && c < 0x3A) // '/' and digits
                                     || (c > 0x40 && c < 0x5B) // Uppercase letters
                                     || (c > 0x60 && c < 0x7B) // Lowercase letters
                                     || (c == 0x2B); };        // '+'
        auto look = view{ code };
        //todo reserv data
        if (auto size = bs64.size())
        {
            byte buff[4];
            auto head = bs64.begin();
            auto tail = head + size;
            auto step = 0;
            while (head != tail)
            {
                auto c = *head++;
                if (!is64(c) || c == '=') break;

                buff[step] = c;
                if (++step == 4)
                {
                    step = 0;
                    for (auto& a : buff) a = static_cast<byte>(look.find(a));
                    data.push_back(( buff[0]         << 2) + ((buff[1] & 0x30) >> 4));
                    data.push_back(((buff[1] & 0x0F) << 4) + ((buff[2] & 0x3C) >> 2));
                    data.push_back(((buff[2] & 0x03) << 6) +   buff[3]);
                }
            }

            if (step != 0)
            {
                auto temp = step;
                while (temp < 4) buff[temp++] = 0;
                for (auto& a : buff) a = static_cast<byte>(look.find(a));
                if (step > 1) data.push_back(( buff[0]         << 2) + ((buff[1] & 0x30) >> 4));
                if (step > 2) data.push_back(((buff[1] & 0x0F) << 4) + ((buff[2] & 0x3C) >> 2));
            }
        }
    }
    auto unbase64(view bs64)
    {
        auto data = text{};
        unbase64(bs64, data);
        return data;
    }
    // utf: Escape control chars (replace all ctrls with printables), put it to the buff, and return the buff's delta.
    template<bool Split = true, bool Multiline = true>
    auto debase(qiew utf8, text& buff)
    {
        auto mode = si32{};
        auto init = buff.size();
        auto size = utf8.size();
        capacity(buff, size * 2);
        auto head  = size - 1; // Begining with ESC is a special case.
        auto s = [&](prop const& traits, view& utf8)
        {
            switch (traits.cdpoint)
            {
                case 033:
                    if constexpr (Split)
                    {
                        if (head == utf8.size()) buff += "\\e";
                        else                     buff += "\n\\e";
                    }
                    else buff += "\\e";
                    break;
                case '\n':
                    if constexpr (Split)
                    {
                        if (utf8.size() && utf8.front() == '\033')
                        {
                            if constexpr (Multiline) buff += "\\n\n\\e";
                            else                     buff += "\\n\\e";
                            utf8.remove_prefix(1);
                            break;
                        }
                    }
                    if constexpr (Multiline) buff += "\\n\n";
                    else                     buff += "\\n";
                    break;
                case '\r': buff += "\\r"; break;
                case 8:    buff += "\\b"; break;
                case 9:    buff += "\\t"; break;
                default:
                {
                    auto cp = traits.cdpoint;
                    if (cp < 0x100000) { buff += "\\u"; to_hex<true>(buff, cp, 4); }
                    else               { buff += "\\U"; to_hex<true>(buff, cp, 8); }
                }
            }
            return utf8;
        };
        auto y = [&](frag const& cluster)
        {
                 if (cluster.text.front() == '\\') buff += "\\\\";
            else if (cluster.text.front() == '\0') buff += "\\0";
            //else if (cluster.text.front() == ' ') buff += "\x20";
            else                                   buff += cluster.text;
        };
        decode<faux>(s, y, utf8, mode);
        return static_cast<si32>(buff.length() - init);
    }
    // utf: Return a string without control chars (replace all ctrls with printables).
    template<bool Split = true, bool Multiline = true>
    auto debase(qiew utf8)
    {
        auto buff = text{};
        debase<Split, Multiline>(utf8, buff);
        return buff;
    }
    template<class Iter>
    auto find_char(Iter head, Iter tail, view delims)
    {
        while (head != tail)
        {
            auto c = *head;
                 if (delims.find(c) != view::npos) break;
            else if (c == '\\' && head != tail) ++head;
            ++head;
        }
        return head;
    }
    template<class Iter>
    auto find_char(Iter head, Iter tail, char delim)
    {
        while (head != tail)
        {
            auto c = *head;
                 if (delim == c) break;
            else if (c == '\\' && head != tail) ++head;
            ++head;
        }
        return head;
    }
    auto check_any(view shadow, view delims)
    {
        auto p = utf::find_char(shadow.begin(), shadow.end(), delims);
        return p != shadow.end();
    }
    template<class P>
    void trim_front_if(view& utf8, P pred)
    {
        auto head = utf8.begin();
        auto tail = utf8.end();
        while (head != tail)
        {
            auto c = *head;
            if (pred(c)) break;
            ++head;
        }
        utf8.remove_prefix(std::distance(utf8.begin(), head));
    }
    template<class P>
    void trim_back_if(view& utf8, P pred)
    {
        auto head = utf8.rbegin();
        auto tail = utf8.rend();
        while (head != tail)
        {
            auto c = *head;
            if (pred(c)) break;
            ++head;
        }
        utf8.remove_suffix(std::distance(utf8.rbegin(), head));
    }
    auto trim_front(view& utf8, view delims)
    {
        auto temp = utf8;
        trim_front_if(utf8, [&](char c){ return delims.find(c) == text::npos; });
        return temp.substr(0, temp.size() - utf8.size());
    }
    auto trim_front(view& utf8, char c = ' ')
    {
        auto head = utf8.begin();
        auto tail = utf8.end();
        while (head != tail && *head == c)
        {
            ++head;
        }
        utf8.remove_prefix(std::distance(utf8.begin(), head));
    }
    auto trim_back(view& utf8, view delims)
    {
        auto temp = utf8;
        trim_back_if(utf8, [&](char c){ return delims.find(c) == text::npos; });
        return temp.substr(utf8.size(), temp.size() - utf8.size());
    }
    auto trim(view utf8, char space = ' ')
    {
        while (!utf8.empty() && utf8.front() == space) utf8.remove_prefix(1);
        while (!utf8.empty() && utf8. back() == space) utf8.remove_suffix(1);
        return utf8;
    }
    auto trim(view utf8, view delims)
    {
        trim_front(utf8, delims);
        trim_back (utf8, delims);
        return utf8;
    }
    auto get_quote(view& utf8, view delims, view skip = {}) // Without quotes.
    {
        auto head = utf8.begin();
        auto tail = utf8.end();
        auto coor = find_char(head, tail, delims);
        if (std::distance(coor, tail) < 2)
        {
            utf8 = view{};
            return utf8;
        }
        ++coor;
        auto stop = find_char(coor, tail, delims);
        if (stop == tail)
        {
            utf8 = view{};
            return utf8;
        }
        //todo Clang 13.0.0 doesn't get it
        //auto crop = view{ coor, stop };
        auto crop = view{ &(*coor), (size_t)(stop - coor) };

        utf8.remove_prefix(crop.size() + 2);
        if (!skip.empty()) trim_front(utf8, skip);
        return crop;
    }
    auto get_quote(view& utf8) // With quotes.
    {
        if (utf8.size() < 2)
        {
            utf8 = view{};
            return utf8;
        }
        auto quot = utf8.front();
        auto head = utf8.begin();
        auto tail = utf8.end();
        auto stop = find_char(head + 1, tail, quot);
        if (stop == tail)
        {
            utf8 = view{};
            return utf8;
        }
        //todo Clang 13.0.0 doesn't get it
        //auto crop = view{ head, stop + 1 };
        auto crop = view{ &(*head), (size_t)(stop + 1 - head) };
        utf8.remove_prefix(crop.size());
        return crop;
    }
    template<bool Lazy = true>
    auto get_tail(view& utf8, view delims)
    {
        auto head = utf8.begin();
        auto tail = utf8.end();
        auto stop = find_char(head, tail, delims);
        if (stop == tail)
        {
            if constexpr (Lazy)
            {
                utf8 = view{};
                return qiew{ utf8 };
            }
            else
            {
                auto crop = qiew{ utf8 };
                utf8 = view{};
                return crop;
            }
        }
        //todo Clang 13.0.0 doesn't get it
        //auto str = view{ head, stop };
        auto str = qiew{ &(*head), (size_t)(stop - head) };
        //utf8.remove_prefix(std::distance(head, stop));
        utf8.remove_prefix(str.size());
        return str;
    }
    auto get_word(view& utf8, char delim = ' ')
    {
        return get_tail<faux>(utf8, view{ &delim, 1 });
    }
    auto get_token(view& utf8)
    {
        if (utf8.empty()) return utf8;
        auto c = utf8.front();
        auto item = (c == '\'' || c == '"') ? utf::get_quote(utf8)
                                            : utf::get_word(utf8);
        utf::trim_front(utf8);
        return item;
    }
    auto eat_tail(view& utf8, view delims)
    {
        auto head = utf8.begin();
        auto tail = utf8.end();
        auto stop = find_char(head, tail, delims);
        if (stop == tail) utf8 = view{};
        else              utf8.remove_prefix(std::distance(head, stop));
    }
    template<class View>
    auto pop_front(View&& line, auto size)
    {
        auto crop = line.substr(0, size);
        line.remove_prefix(size);
        return crop;
    }
    template<class View>
    auto pop_back(View&& line, auto size)
    {
        auto crop = line.substr(line.size() - size);
        line.remove_suffix(size);
        return crop;
    }
    template<class TextOrView>
    auto is_plain(TextOrView&& utf8)
    {
        auto test = utf8.find('\033');
        return test == text::npos;
    }
    auto to_low(unsigned char c)
    {
        return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
    }
    auto& to_low(text& utf8, size_t size = text::npos)
    {
        auto head = utf8.begin();
        auto tail = head + std::min(utf8.size(), size);
        std::transform(head, tail, head, [](auto c) { return to_low(c); });
        return utf8;
    }
    auto to_low(text&& utf8)
    {
        return to_low(utf8);
    }
    auto& to_up(text& utf8, size_t size = text::npos)
    {
        auto head = utf8.begin();
        auto tail = head + std::min(utf8.size(), size);
        std::transform(head, tail, head, [](unsigned char c) { return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c; });
        return utf8;
    }
    auto to_up(text&& utf8)
    {
        return to_up(utf8);
    }
    template<class W, class P>
    void for_each(text& utf8, W const& what, P proc)
    {
        auto frag = view{ what };
        auto spot = 0_sz;
        auto line_sz = utf8.length();
        auto what_sz = frag.length();

        if (!what_sz || line_sz < what_sz) return;

        while ((spot = utf8.find(frag, spot)) != text::npos)
        {
            auto fill = proc();
            utf8.replace(spot, what_sz, fill);
            spot += what_sz;
        }
    }
    template<class V>
    auto to_hex_0x(V const& n)
    {
        auto result = (flux{} << std::showbase << std::hex << n).str();
        return result;
    }
    auto to_hex_0x(wchr n)
    {
        return to_hex_0x((int)n);
    }
    auto trunc(view utf8, size_t maxy) // Returns a string trimmed at maxy lines.
    {
        auto crop = 0_sz;
        while (maxy--)
        {
            crop = utf8.find('\n', crop);
            if (crop != text::npos) crop++;
            else break;
        }
        return qiew{ utf8.substr(0, crop) };
    }
}

namespace netxs
{
    using qiew = utf::qiew;
}