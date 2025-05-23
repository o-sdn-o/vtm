// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

#include "canvas.hpp"

namespace netxs::xml
{
    template<class T>
    auto take(qiew utf8) -> std::optional<T>
    {
        utf::trim_front(utf8);
        if (utf8.starts_with("0x"))
        {
            utf8.remove_prefix(2);
            return utf::to_int<T, 16>(utf8);
        }
        else return utf8 ? utf::to_int<T, 10>(utf8)
                         : std::nullopt;
    }
    template<>
    auto take<fp32>(qiew utf8) -> std::optional<fp32>
    {
        utf::trim_front(utf8);
        return utf8 ? utf::to_int<fp32>(utf8)
                    : std::nullopt;
    }
    template<>
    auto take<text>(qiew utf8) -> std::optional<text>
    {
        return utf8.str();
    }
    template<>
    auto take<bool>(qiew utf8) -> std::optional<bool>
    {
        utf::trim_front(utf8);
        auto value = utf::to_lower(utf8.str());
        if (value.starts_with("undef")) return std::nullopt; // Use default.
        if (value.empty() || value == "1"
                          || value == "on"
                          || value == "yes"
                          || value == "true")
        {
            return true;
        }
        else if (value == "0"
              || value == "off"
              || value == "no"
              || value == "faux"
              || value == "false")
        {
            return faux;
        }
        else return std::nullopt;
    }
    template<>
    auto take<twod>(qiew utf8) -> std::optional<twod>
    {
        utf::trim_front(utf8, " ({[\"\'");
        if (utf8)
        if (auto x = utf::to_int(utf8))
        {
            utf::trim_front(utf8, " ,.x/:;");
            if (auto y = utf::to_int(utf8))
            {
                return twod{ x.value(), y.value() };
            }
        }
        return std::nullopt;
    }
    template<>
    auto take<dent>(qiew utf8) -> std::optional<dent>
    {
        utf::trim_front(utf8, " ({[\"\'");
        if (utf8)
        if (auto l = utf::to_int(utf8))
        {
            utf::trim_front(utf8, " ,.x/:;");
            if (auto r = utf::to_int(utf8))
            {
                utf::trim_front(utf8, " ,.x/:;");
                if (auto t = utf::to_int(utf8))
                {
                    utf::trim_front(utf8, " ,.x/:;");
                    if (auto b = utf::to_int(utf8))
                    {
                        return dent{ l.value(), r.value(), t.value(), b.value() };
                    }
                    else return dent{ l.value(), r.value(), t.value() };
                }
                else return dent{ l.value(), r.value() };
            }
            else return dent{ l.value() };
        }
        return std::nullopt;
    }
    template<>
    auto take<span>(qiew utf8) -> std::optional<span>
    {
        using namespace std::chrono;
        utf::trim_front(utf8, " ({[\"\'");
        if (utf8)
        if (auto x = utf::to_int(utf8))
        {
            auto v = x.value();
            auto p = span{};
                 if (utf8.empty()
                  || utf8.starts_with("ms" )) return span{ milliseconds{ v } };
            else if (utf8.starts_with("us" )) return span{ microseconds{ v } };
            else if (utf8.starts_with("ns" )) return span{  nanoseconds{ v } };
            else if (utf8.starts_with("s"  )) return span{      seconds{ v } };
            else if (utf8.starts_with("min")) return span{      minutes{ v } };
            else if (utf8.starts_with("h"  )) return span{        hours{ v } };
            else if (utf8.starts_with("d"  )) return span{         days{ v } };
            else if (utf8.starts_with("w"  )) return span{        weeks{ v } };
        }
        return std::nullopt;
    }
    template<>
    auto take<argb>(qiew utf8) -> std::optional<argb>
    {
        if (!utf8) return std::nullopt;
        auto tobyte = [](auto c)
        {
                 if (c >= '0' && c <= '9') return (byte)(c - '0');
            else if (c >= 'a' && c <= 'f') return (byte)(c - 'a' + 10);
            else                           return (byte)(0);
        };
        auto value = utf::to_lower(utf8.str());
        auto result = argb{};
        auto shadow = view{ value };
        utf::trim_front(shadow, " ({[\"\'");
        if (shadow.starts_with('#')) // hex: #rrggbbaa
        {
            shadow.remove_prefix(1);
            if (shadow.size() >= 8) // hex: #rrggbbaa
            {
                result.chan.r = (tobyte(shadow[0]) << 4) + tobyte(shadow[1]);
                result.chan.g = (tobyte(shadow[2]) << 4) + tobyte(shadow[3]);
                result.chan.b = (tobyte(shadow[4]) << 4) + tobyte(shadow[5]);
                result.chan.a = (tobyte(shadow[6]) << 4) + tobyte(shadow[7]);
                return result;
            }
            else if (shadow.size() >= 6) // hex: #rrggbb
            {
                result.chan.r = (tobyte(shadow[0]) << 4) + tobyte(shadow[1]);
                result.chan.g = (tobyte(shadow[2]) << 4) + tobyte(shadow[3]);
                result.chan.b = (tobyte(shadow[4]) << 4) + tobyte(shadow[5]);
                result.chan.a = 0xff;
                return result;
            }
            //log("%%Unknown hex color format: { %value% }, expected #rrggbbaa or #rrggbb color hex value", prompt::xml, value);
        }
        else if (shadow.starts_with("0x")) // hex: 0xaarrggbb
        {
            shadow.remove_prefix(2);
            if (shadow.size() >= 8) // hex: 0xaarrggbb
            {
                result.chan.a = (tobyte(shadow[0]) << 4) + tobyte(shadow[1]);
                result.chan.r = (tobyte(shadow[2]) << 4) + tobyte(shadow[3]);
                result.chan.g = (tobyte(shadow[4]) << 4) + tobyte(shadow[5]);
                result.chan.b = (tobyte(shadow[6]) << 4) + tobyte(shadow[7]);
                return result;
            }
            else if (shadow.size() >= 6) // hex: 0xrrggbb
            {
                result.chan.a = 0xff;
                result.chan.r = (tobyte(shadow[0]) << 4) + tobyte(shadow[1]);
                result.chan.g = (tobyte(shadow[2]) << 4) + tobyte(shadow[3]);
                result.chan.b = (tobyte(shadow[4]) << 4) + tobyte(shadow[5]);
                return result;
            }
            //log("%%Unknown hex color format: { %value% }, expected 0xaarrggbb or 0xrrggbb color hex value", prompt::xml, value);
        }
        else if (utf::check_any(shadow, ",;/")) // dec: 000,000,000,000
        {
            if (auto r = utf::to_int(shadow))
            {
                result.chan.r = (byte)r.value();
                utf::trim_front(shadow, ",./:;");
                if (auto g = utf::to_int(shadow))
                {
                    result.chan.g = (byte)g.value();
                    utf::trim_front(shadow, ",./:;");
                    if (auto b = utf::to_int(shadow))
                    {
                        result.chan.b = (byte)b.value();
                        utf::trim_front(shadow, ",./:;");
                        if (auto a = utf::to_int(shadow)) result.chan.a = (byte)a.value();
                        else                              result.chan.a = 0xff;
                        return result;
                    }
                }
            }
            //log("%%Unknown hex color format: { %value% }, expected 000,000,000,000 decimal (r,g,b,a) color value", prompt::xml, value);
        }
        else if (auto c = utf::to_int(shadow)) // Single ANSI color value
        {
            if (c.value() >=0 && c.value() <=255)
            {
                result = argb::vt256[c.value()];
                return result;
            }
            //log("%%Unknown ANSI 256-color value format: { %value% }, expected 0-255 decimal value", prompt::xml, value);
        }
        return std::nullopt;
    }
    template<class T>
    auto take_or(qiew utf8, T fallback)
    {
        if (auto v = take<T>(utf8))
        {
            return v.value();
        }
        else
        {
            return fallback;
        }
    }

    struct document
    {
        enum class type
        {
            na,            // start of file
            eof,           // end of file
            eol,           // end of line
            top_token,     // open tag name
            end_token,     // close tag name
            token,         // name
            raw_text,      //         ex: raw text
            quotes,        // '"'     ex: " or '
            quoted_text,   // '"'     ex: " text "
            begin_tag,     // '<'     ex: <name ...
            close_tag,     // '</'    ex: ... </name>
            comment_begin, // '<!--'  ex: ... <!-- ...
            comment_close, // '-->'   ex: ... -->
            close_inline,  // '>'     ex: ... >
            empty_tag,     // '/>'    ex: ... />
            equal,         // '='     ex: name=value
            defaults,      // '*'     ex: name*
            compact,       // '/[^>]' ex: compact syntax: <name/nested_block1/nested_block2=value param=value />
            include,       // ':'     ex: <name:...=value param=value />
            localpath,     //         ex: <name:/path/path=value param=value />
            filepath,      //         ex: <name:"/filepath/filepath"=value param=value />
            spaces,        // ' '     ex: \s\t\r\n...
            unknown,       //
            tag_value,     //
            error,         // Inline error message.
        };

        struct literal;
        using frag = netxs::sptr<literal>;

        struct literal
        {
            using wptr = netxs::wptr<literal>;

            wptr prev; // literal: Pointer to the prev.
            frag next; // literal: Pointer to the next.
            type kind; // literal: Content type.
            text utf8; // literal: Content data.

            template<class ...Args>
            literal(type kind, Args&&... args)
                : kind{ kind },
                  utf8{ std::forward<Args>(args)... }
            { }
        };

        struct suit
        {
            frag data; // suit: Linked list start.
            bool fail; // suit: Broken format.
            text file; // suit: Data source name.
            frag back; // suit: Linked list end.

            suit(suit&&) = default;
            suit(view file = {})
                : data{ ptr::shared<literal>(type::na) },
                  fail{ faux },
                  file{ file },
                  back{ data }
            { }

            void init(view filename = {})
            {
                data = ptr::shared<literal>(type::na);
                fail = faux;
                file = filename;
                back = data;
            }
            template<class ...Args>
            auto append(type kind, Args&&... args)
            {
                auto item = ptr::shared<literal>(kind, std::forward<Args>(args)...);
                item->prev = back;
                back->next = item;
                back = item;
                return back;
            }
            auto lines()
            {
                auto count = 0_sz;
                auto next = data;
                while (next)
                {
                    auto& utf8 = next->utf8;
                    count += std::count(utf8.begin(), utf8.end(), '\n');
                    next = next->next;
                }
                return std::max(1_sz, count);
            }
            auto utf8()
            {
                auto crop = text{};
                auto next = data;
                while (next)
                {
                    crop+= next->utf8;
                    next = next->next;
                }
                return crop;
            }
            auto show()
            {
                static constexpr auto top_token_fg = argb{ 0xFF'99'd7'ff };
                static constexpr auto end_token_fg = argb{ 0xFF'6a'96'b3 };
                static constexpr auto token_fg     = argb{ 0xFF'83'b8'da };
                static constexpr auto liter_fg     = argb{ 0xFF'80'80'80 };
                static constexpr auto comment_fg   = argb{ 0xFF'4e'4e'4e };
                static constexpr auto defaults_fg  = argb{ 0xFF'9e'9e'9e };
                static constexpr auto quotes_fg    = argb{ 0xFF'BB'BB'BB };
                static constexpr auto value_fg     = argb{ 0xFF'90'96'f0 };
                static constexpr auto value_bg     = argb{ 0xFF'20'20'20 };

                //test
                //auto tmp = page.data.front().upto;
                //auto clr = 0;

                auto yield = ansi::escx{};
                auto next = data;
                while (next)
                {
                    auto& item = *next;
                    auto& utf8 = item.utf8;
                    auto  kind = item.kind;
                    next = next->next;

                    //test
                    //if (item.upto == page.data.end() || tmp != item.upto)
                    //{
                    //    clr++;
                    //    tmp = item.upto;
                    //}

                    auto fgc = argb{};
                    auto bgc = argb{};
                    switch (kind)
                    {
                        case type::eof:           fgc = redlt;        break;
                        case type::top_token:     fgc = top_token_fg; break;
                        case type::end_token:     fgc = end_token_fg; break;
                        case type::compact:       fgc = end_token_fg; break;
                        case type::token:         fgc = token_fg;     break;
                        case type::raw_text:      fgc = yellowdk;     break;
                        case type::quoted_text:   fgc = yellowdk;     break;
                        case type::comment_begin: fgc = comment_fg;   break;
                        case type::comment_close: fgc = comment_fg;   break;
                        case type::begin_tag:     fgc = liter_fg;     break;
                        case type::close_tag:     fgc = liter_fg;     break;
                        case type::close_inline:  fgc = liter_fg;     break;
                        case type::empty_tag:     fgc = liter_fg;     break;
                        case type::equal:         fgc = liter_fg;     break;
                        case type::quotes:        fgc = quotes_fg;    break;
                        case type::defaults:      fgc = defaults_fg;  break;
                        case type::unknown:       fgc = redlt;        break;
                        case type::tag_value:     fgc = value_fg;
                                                  bgc = value_bg;     break;
                        case type::error:         fgc = whitelt;
                                                  bgc = reddk;
                                                  yield += ' ';       break;
                        default: break;
                    }
                    //test
                    //yield.bgc((tint)(clr % 8));

                    if (utf8.size())
                    {
                             if (bgc) yield.fgc(fgc).bgc(bgc).add(utf8).nil();
                        else if (fgc) yield.fgc(fgc)         .add(utf8).nil();
                        else          yield                  .add(utf8);
                    }
                }

                auto count = 1;
                auto width = 0_sz;
                auto total = lines();
                while (total)
                {
                    total /= 10;
                    width++;
                }
                auto numerate = [&]
                {
                    return ansi::pushsgr().fgc(liter_fg) + utf::adjust(std::to_string(count++), width, ' ', true) + ": " + ansi::popsgr();
                };
                yield = numerate() + yield;
                utf::for_each(yield, "\n", [&]{ return "\n" + numerate(); });
                yield.add('\n');
                return yield;
            }
        };

        struct elem;
        using sptr = netxs::sptr<elem>;
        using wptr = netxs::wptr<elem>;
        using heap = std::vector<frag>;
        using vect = std::vector<sptr>;
        using subs = utf::unordered_map<text, vect>;

        struct elem
        {
            enum form
            {
                node,
                attr,
                flat,
                pact, // Element has compact form (<element/elem2/elem3 ... />).
            };

            frag from; // elem: Pointer to the begging of the semantic block.
            frag name; // elem: Tag name.
            frag insA; // elem: Insertion point for inline subelements.
            frag insB; // elem: Insertion point for nested subelements.
            frag upto; // elem: Pointer to the end of the semantic block.
            heap body; // elem: Value fragments.
            subs hive; // elem: Subelements.
            wptr defs; // elem: Template.
            bool fake; // elem: Is it template.
            bool base; // elem: Merge overwrite priority (new list?).
            form mode; // elem: Element storage form.

            elem()
                : fake{ faux },
                  base{ faux },
                  mode{ node }
            { }
           ~elem()
            {
                hive.clear();
                if (auto prev = from->prev.lock())
                {
                    auto next = upto->next;
                              prev->next = next;
                    if (next) next->prev = prev;
                }
            }

            auto is_quoted()
            {
                if (body.size() == 1)
                {
                    auto& value_placeholder = body.front();
                    if (value_placeholder->kind == type::tag_value) // equal [spaces] quotes tag_value quotes
                    if (auto quote_placeholder = value_placeholder->prev.lock())
                    if (quote_placeholder->kind == type::quotes && quote_placeholder->utf8.size())
                    {
                        auto c = quote_placeholder->utf8.front();
                        return c == '\"' || c == '\'';
                    }
                }
                return faux;
            }
            template<bool WithTemplate = faux>
            auto list(qiew path_str)
            {
                path_str = utf::trim(path_str, '/');
                auto anchor = this;
                auto crop = vect{}; //auto& items = config.root->hive["menu"][0]->hive["item"]...;
                auto temp = text{};
                auto path = utf::split(path_str, '/');
                if (path.size())
                {
                    auto head = path.begin();
                    auto tail = path.end();
                    while (head != tail)
                    {
                        temp = *head++;
                        if (auto iter = anchor->hive.find(temp);
                                 iter!= anchor->hive.end())
                        {
                            auto& i = iter->second;
                            crop.reserve(i.size());
                            if (head == tail)
                            {
                                for (auto& item : i)
                                {
                                    if constexpr (WithTemplate) crop.push_back(item);
                                    else       if (!item->fake) crop.push_back(item);
                                }
                            }
                            else if (i.size() && i.front())
                            {
                                anchor = &(*(i.front()));
                            }
                            else break;
                        }
                        else break;
                    }
                }
                return crop;
            }
            auto take_value()
            {
                auto value = text{};
                for (auto& value_placeholder : body)
                {
                    value += value_placeholder->utf8;
                }
                utf::unescape(value);
                return value;
            }
            void init_value(qiew value, bool unescaped = true, std::optional<bool> quoted = {})
            {
                if (body.size())
                {
                    for (auto& value_placeholder : body) value_placeholder->utf8.clear();
                    body.resize(1);
                    auto value_placeholder = body.front();
                    if (value_placeholder->kind == type::tag_value) // equal [spaces] quotes tag_value quotes
                    if (auto quote_placeholder = value_placeholder->prev.lock())
                    if (quote_placeholder->kind == type::quotes)
                    if (auto equal_placeholder = quote_placeholder->prev.lock())
                    {
                        if (equal_placeholder->kind != type::equal) // Spaces after equal sign.
                        {
                            equal_placeholder = equal_placeholder->prev.lock();
                        }
                        if (equal_placeholder && equal_placeholder->kind == type::equal)
                        {
                            if ((value.size() && !quoted) || quoted.value())
                            {
                                equal_placeholder->utf8 = "="sv;
                                quote_placeholder->utf8 = "\""sv;
                                if (value_placeholder->next) value_placeholder->next->utf8 = "\""sv;
                            }
                            else
                            {
                                equal_placeholder->utf8 = value.size() ? "="sv : ""sv;
                                quote_placeholder->utf8 = ""sv;
                                if (value_placeholder->next) value_placeholder->next->utf8 = ""sv;
                            }
                        }
                        else log("%%Equal sign placeholder not found", prompt::xml);
                    }
                    if (unescaped) utf::escape(value, value_placeholder->utf8, '\"');
                    else           value_placeholder->utf8 = value;
                }
                else log("%%Unexpected assignment to '%%'", prompt::xml, name->utf8);
            }
            void sync_value(elem& node)
            {
                if (body.size())
                if (body.size() != node.body.size() || !std::equal(body.begin(), body.end(), node.body.begin(), [&](auto& s, auto& d){ return s->utf8 == d->utf8; }))
                {
                    auto value = text{};
                    for (auto& value_placeholder : node.body)
                    {
                        value += value_placeholder->utf8;
                    }
                    init_value(value, faux, node.is_quoted());
                }
            }
            template<class T>
            auto take(qiew attr, T fallback = {})
            {
                if (auto iter = hive.find(attr); iter != hive.end())
                {
                    auto& item_set = iter->second;
                    if (item_set.size()) // Take the first item only.
                    {
                        auto crop = item_set.front()->take_value();
                        return xml::take_or<T>(crop, fallback);
                    }
                }
                if (auto defs_ptr = defs.lock()) return defs_ptr->take(attr, fallback);
                else                             return fallback;
            }
            template<class T>
            auto take(qiew attr, T defval, utf::unordered_map<text, T> const& dict)
            {
                if (attr.empty()) return defval;
                auto crop = take(attr, ""s);
                auto iter = dict.find(crop);
                return iter == dict.end() ? defval
                                          : iter->second;
            }
            auto show(sz_t indent = 0) -> text
            {
                auto data = text{};
                data += text(indent, ' ') + '<' + name->utf8;
                if (fake) data += view_defaults;

                if (body.size())
                {
                    auto crop = take_value();
                    if (crop.size())
                    {
                        data.push_back('=');
                        utf::quote(crop, data, '\"');
                    }
                }

                if (hive.empty()) data += "/>\n";
                else
                {
                    data += ">\n";
                    for (auto& [sub_name, sub_list] : hive)
                    {
                        for (auto& item : sub_list)
                        {
                            data += item->show(indent + 4);
                        }
                    }
                    data += text(indent, ' ') + "</" + name->utf8 + ">\n";
                }

                return data;
            }
            auto snapshot()
            {
                auto crop = text{};
                auto head = from;
                while (head)
                {
                    crop += head->utf8;
                    if (head == upto) break;
                    head = head->next;
                }
                if (crop.starts_with('\n')
                 || crop.starts_with('\r'))
                {
                    auto temp = view{ crop };
                    auto dent = text{ utf::trim_front(temp, whitespaces) };
                    crop = temp;
                    utf::replace_all(crop, dent, "\n");
                }
                return crop;
            }
        };

        static constexpr auto find_start         = "<"sv;
        static constexpr auto rawtext_delims     = std::tuple{ " "sv, "/>"sv, ">"sv, "<"sv, "\n"sv, "\r"sv, "\t"sv };
        static constexpr auto token_delims       = " \t\n\r=*/><"sv;
        static constexpr auto view_comment_begin = "<!--"sv;
        static constexpr auto view_comment_close = "-->"sv;
        static constexpr auto view_close_tag     = "</"sv;
        static constexpr auto view_begin_tag     = "<"sv;
        static constexpr auto view_empty_tag     = "/>"sv;
        static constexpr auto view_slash         = "/"sv;
        static constexpr auto view_close_inline  = ">"sv;
        static constexpr auto view_quoted_text   = "\""sv;
        static constexpr auto view_equal         = "="sv;
        static constexpr auto view_defaults      = "*"sv;

        suit page;
        sptr root;

        document() = default;
        document(document&&) = default;
        document(view data, view file = {})
            : page{ file },
              root{ ptr::shared<elem>()}
        {
            read(data);
        }
        operator bool () const { return root ? !root->hive.empty() : faux; }

        void load(view data, view file = {})
        {
            page.init(file);
            root = ptr::shared<elem>();
            read(data);
        }
        template<bool WithTemplate = faux>
        auto take(view path)
        {
            if (!root) return vect{};
            else
            {
                path = utf::trim(path, '/');
                if (path.empty()) return vect{ root };
                else              return root->list<WithTemplate>(path);
            }
        }
        auto join(view path, vect const& list)
        {
            path = utf::trim(path, '/');
            auto slash_pos = path.rfind('/', path.size());
            auto parent_path = path.substr(0, slash_pos);
            auto branch_path = slash_pos != text::npos ? path.substr(slash_pos + sizeof('/')) : view{};
            auto dest_host = take(parent_path);
            if (dest_host.size())
            {
                auto parent = dest_host.front();
                if (parent->mode == elem::form::pact)
                {
                    log("%%Destination path is not suitable for merging '%parent_path%'", prompt::xml, parent_path);
                    return;
                }
                auto& hive = parent->hive;
                auto iter = hive.find(qiew{ branch_path });
                if (iter == hive.end())
                {
                    iter = hive.emplace(branch_path , vect{}).first;
                }
                auto& dest = iter->second;
                for (auto& item : list) if (item && item->name->utf8 == branch_path)
                {
                    //todo unify
                    if (item->base) dest.clear();
                    auto mode = item->mode;
                    auto from = item->from;
                    auto upto = item->upto;
                    auto next = upto->next;
                    if (auto gate = mode == elem::form::attr ? parent->insA : parent->insB)
                    if (auto prev = gate->prev.lock())
                    if (auto past = from->prev.lock())
                    {
                        from->prev = prev;
                        upto->next = gate;
                        gate->prev = upto;
                        prev->next = from;
                        past->next = next;  // Release an element from the previous list.
                        if (next) next->prev = past;
                        dest.push_back(item);
                        if (mode != elem::form::attr) // Prepend '\n    <' to item when inserting it to gate==insB.
                        {
                            if (from->utf8.empty()) // Checking indent. Take indent from parent + pads if it is absent.
                            {
                                from->utf8 = parent->from->utf8 + "    ";
                            }
                            if (from->next && from->next->kind == type::begin_tag) // Checking begin_tag.
                            {
                                auto shadow = view{ from->next->utf8 };
                                if (utf::trim_front(shadow, whitespaces).empty()) // Set it to '<' if it is absent.
                                {
                                    from->next->utf8 = "<";
                                }
                            }
                        }
                        continue;
                    }
                    log("%%Unexpected format for item '%parent_path%/%item->name->utf8%'", prompt::xml, parent_path, item->name->utf8);
                }
            }
            else log("%%Destination path not found '%parent_path%'", prompt::xml, parent_path);
        }
        // xml: Attach the node list to the specified path.
        void attach(view mount_point, vect const& sub_list)
        {
            auto dest_list = take(mount_point);
            if (dest_list.size())
            {
                auto& parent = dest_list.front();
                if (parent->mode == elem::form::pact)
                {
                    log("%%Destination path is not suitable for merging '%parent_path%'", prompt::xml, mount_point);
                    return;
                }
                auto& parent_hive = parent->hive;
                auto connect = [&](auto& subnode_name)
                {
                    auto iter = parent_hive.find(subnode_name);
                    if (iter == parent_hive.end()) iter = parent_hive.emplace(subnode_name, vect{}).first;
                    return iter;
                };
                auto iter = connect(sub_list.front()->name->utf8);
                for (auto& item : sub_list)
                {
                    auto& current_node_name = iter->first;
                    auto& subnode_name = sub_list.front()->name->utf8;
                    if (current_node_name != subnode_name) // The case when the list is heterogeneous.
                    {
                        iter = connect(subnode_name);
                    }
                    //todo unify
                    auto& dest = iter->second;
                    if (item->base) dest.clear();
                    auto mode = item->mode;
                    auto from = item->from;
                    auto upto = item->upto;
                    auto next = upto->next;
                    if (auto gate = mode == elem::form::attr ? parent->insA : parent->insB)
                    if (auto prev = gate->prev.lock())
                    if (auto past = from->prev.lock())
                    {
                        from->prev = prev;
                        upto->next = gate;
                        gate->prev = upto;
                        prev->next = from;
                        past->next = next;  // Release an element from the previous list.
                        if (next) next->prev = past;
                        dest.push_back(item);
                        continue;
                    }
                    log("%%Unexpected format for item '%mount_point%%node%'", prompt::xml, mount_point, item->name->utf8);
                }
            }
            else log("%%Destination path not found '%mount_point%'", prompt::xml, mount_point);
        }
        void overlay(sptr node_ptr, text path)
        {
            auto& node = *node_ptr;
            auto& name = node.name->utf8;
            path += "/" + name;
            auto dest_list = take<true>(path);
            auto is_dest_list = (dest_list.size() && dest_list.front()->fake) || dest_list.size() > 1;
            if (is_dest_list || dest_list.empty())
            {
                join(path, { node_ptr });
            }
            else
            {
                auto& dest = dest_list.front();
                dest->sync_value(node);
                for (auto& [sub_name, sub_list] : node.hive) // Proceed subelements.
                {
                    auto count = sub_list.size();
                    if (count == 1 && sub_list.front()->fake == faux)
                    {
                        overlay(sub_list.front(), path);
                    }
                    else if (count) // It is a list.
                    {
                        join(path + "/" + sub_name, sub_list);
                    }
                    else log("%%Unexpected tag without data: %tag%", prompt::xml, sub_name);
                }
            }
        }

    private:
        vect compacted;
        auto fail(text msg)
        {
            page.fail = true;
            page.append(type::error, msg);
            log("%%%msg% at %page.file%:%lines%", prompt::xml, msg, page.file, page.lines());
        }
        auto fail(type last, type what)
        {
            auto str = [&](type what)
            {
                switch (what)
                {
                    case type::na:            return view{ "{START}" }   ;
                    case type::eof:           return view{ "{EOF}" }     ;
                    case type::eol:           return view{ "{EOL}" }     ;
                    case type::token:         return view{ "{token}" }   ;
                    case type::raw_text:      return view{ "{raw text}" };
                    case type::compact:       return view{ "{compact}" } ;
                    case type::quoted_text:   return view_quoted_text    ;
                    case type::begin_tag:     return view_begin_tag      ;
                    case type::close_tag:     return view_close_tag      ;
                    case type::comment_begin: return view_comment_begin  ;
                    case type::comment_close: return view_comment_close  ;
                    case type::close_inline:  return view_close_inline   ;
                    case type::empty_tag:     return view_empty_tag      ;
                    case type::equal:         return view_equal          ;
                    case type::defaults:      return view_defaults       ;
                    default:                  return view{ "{unknown}" } ;
                };
            };
            fail(ansi::add("Unexpected '", str(what), "' after '", str(last), "'"));
        }
        auto peek(view& data, type& what, type& last)
        {
            last = what;
            if (data.empty()) what = type::eof;
            else if (data.starts_with(view_comment_begin)) what = type::comment_begin;
            else if (last == type::na)
            {
                if (!data.starts_with(view_close_tag    )
                 &&  data.starts_with(view_begin_tag    )) what = type::begin_tag;
                else return;
            }
            else if (data.starts_with(view_close_tag    )) what = type::close_tag;
            else if (data.starts_with(view_begin_tag    )) what = type::begin_tag;
            else if (data.starts_with(view_empty_tag    )) what = type::empty_tag;
            else if (data.starts_with(view_close_inline )) what = type::close_inline;
            else if (data.starts_with(view_slash        ))
            {
                if (last == type::token) what = type::compact;
                else                     what = type::raw_text;
            }
            else if (data.starts_with(view_quoted_text  )) what = type::quoted_text;
            else if (data.starts_with(view_equal        )) what = type::equal;
            else if (data.starts_with(view_defaults     )
                  && last == type::token)                  what = type::defaults;
            else if (whitespaces.find(data.front()) != view::npos) what = type::spaces;
            else if (last == type::close_tag
                  || last == type::begin_tag
                  || last == type::token
                  || last == type::defaults
                  || last == type::raw_text
                  || last == type::compact
                  || last == type::quoted_text) what = type::token;
            else                                what = type::raw_text;
        }
        auto name(view& data)
        {
            return utf::take_front(data, token_delims).str();
        }
        auto body(view& data, type kind = type::tag_value) -> frag
        {
            auto item_ptr = frag{};
            if (data.size())
            {
                auto delim = data.front();
                if (delim != '\'' && delim != '\"')
                {
                    auto crop = utf::take_front(data, rawtext_delims);
                               page.append(type::quotes);
                    item_ptr = page.append(kind, crop);
                               page.append(type::quotes);
                }
                else
                {
                    auto crop = utf::take_quote(data, delim);
                    auto delim_view = view(&delim, 1);
                               page.append(type::quotes, delim_view);
                    item_ptr = page.append(kind, crop);
                               page.append(type::quotes, delim_view);
                }
            }
            else item_ptr = page.append(kind);
            return item_ptr;
        }
        auto skip(view& data, type kind)
        {
            auto temp = data;
            switch (kind)
            {
                case type::comment_begin: data.remove_prefix(view_comment_begin.size()); break;
                case type::comment_close: data.remove_prefix(view_comment_close.size()); break;
                case type::close_tag:     data.remove_prefix(view_close_tag    .size()); break;
                case type::begin_tag:     data.remove_prefix(view_begin_tag    .size()); break;
                case type::empty_tag:     data.remove_prefix(view_empty_tag    .size()); break;
                case type::close_inline:  data.remove_prefix(view_close_inline .size()); break;
                case type::quoted_text:   data.remove_prefix(view_quoted_text  .size()); break;
                case type::equal:         data.remove_prefix(view_equal        .size()); break;
                case type::defaults:      data.remove_prefix(view_defaults     .size()); break;
                case type::token:
                case type::top_token:
                case type::end_token:     utf::eat_tail(data, token_delims); break;
                case type::raw_text:
                case type::quotes:
                case type::tag_value:     body(data, type::raw_text);         break;
                case type::spaces:        utf::trim_front(data, whitespaces); break;
                case type::na:            utf::take_front(data, find_start);  break;
                case type::compact:
                case type::unknown:       if (data.size()) data.remove_prefix(1); break;
                default: break;
            }
            return temp.substr(0, temp.size() - data.size());
        }
        auto trim(view& data)
        {
            auto temp = utf::trim_front(data, whitespaces);
            auto crop = !temp.empty();
            if (crop) page.append(type::spaces, std::move(temp));
            return crop;
        }
        auto diff(view& data, view& temp, type kind = type::spaces)
        {
                 if (temp.size() > data.size()) page.append(kind, temp.substr(0, temp.size() - data.size()));
            else if (temp.size() < data.size()) fail("Unexpected data");
        }
        auto pair(sptr& item, view& data, type& what, type& last, type kind)
        {
            //todo
            //include external blocks if name contains ':'s.
            item->name = page.append(kind, name(data));
            auto temp = data;
            utf::trim_front(temp, whitespaces);
            peek(temp, what, last);
            if (what == type::defaults)
            {
                diff(data, temp, type::defaults);
                data = temp;
                item->fake = true;
                auto& last_type = page.back->kind;
                if (last_type == type::top_token || last_type == type::token)
                {
                    last_type = type::defaults;
                }
                page.append(type::defaults, skip(data, what));
                temp = data;
                utf::trim_front(temp, whitespaces);
                peek(temp, what, last);
                item->base = what == type::empty_tag;
            }
            if (what == type::equal)
            {
                diff(temp, data, type::spaces);
                data = temp;
                page.append(type::equal, skip(data, what));
                trim(data);
                peek(data, what, last);
                if (what == type::quoted_text || what == type::raw_text)
                {
                    item->body.push_back(body(data));
                }
                else fail(last, what);
            }
            else if (what != type::compact) // Add placeholder for absent value.
            {
                                     page.append(type::equal);
                                     page.append(type::quotes);
                item->body.push_back(page.append(type::tag_value));
                                     page.append(type::quotes);
            }
            return temp;
        }
        auto open(sptr& item)
        {
            if (!page.data || page.back->kind != type::spaces)
            {
                page.append(type::spaces);
            }
            item->from = page.back;
        }
        auto seal(sptr& item)
        {
            item->upto = page.back;
        }
        auto note(view& data, type& what, type& last)
        {
            auto size = data.find(view_comment_close);
            if (size == view::npos)
            {
                page.append(type::unknown, data);
                data = {};
                last = what;
                what = type::eof;
                return faux;
            }
            size += view_comment_close.size();
            page.append(type::comment_begin, data.substr(0, size));
            data.remove_prefix(size);
            return true;
        }
        void push(sptr& item, sptr& next, auto& defs)
        {
            auto& sub_name = next->name->utf8;
            if (next->fake) defs[sub_name] = next;
            else
            {
                auto iter = defs.find(sub_name);
                if (iter != defs.end())
                {
                    next->defs = iter->second;
                }
            }
            item->hive[sub_name].push_back(next);
        }
        void read_subsections(sptr& item, view& data, type& what, type& last, si32& deep, auto& defs)
        {
            do
            {
                auto temp = data;
                utf::trim_front(temp, whitespaces);
                //auto p = std::vector{ std::tuple{ 0, what, last, temp }};
                peek(temp, what, last);
                do
                {
                    //p.push_back(std::tuple{ 1, what, last, temp });
                    if (what == type::quoted_text)
                    {
                        diff(temp, data, type::quoted_text);
                        data = temp;
                        item->body.push_back(body(data));
                        trim(data);
                        temp = data;
                    }
                    else if (what == type::raw_text)
                    {
                        auto size = data.find('<');
                        if (size == view::npos)
                        {
                            item->body.push_back(page.append(type::unknown, data));
                            data = {};
                            last = what;
                            what = type::eof;
                            break;
                        }
                        item->body.push_back(page.append(type::raw_text, data.substr(0, size)));
                        data.remove_prefix(size);
                        temp = data;
                    }
                    else if (what == type::begin_tag && deep < 30)
                    {
                        trim(data);
                        data = temp;
                        auto next = ptr::shared<elem>();
                        what = read_node(next, data, deep + 1);
                        push(item, next, defs);
                        temp = data;
                        utf::trim_front(temp, whitespaces);
                    }
                    else if (what == type::comment_begin) // Proceed '<!--'.
                    {
                        auto size = data.find(view_comment_close);
                        if (size == view::npos)
                        {
                            page.append(type::unknown, data);
                            data = {};
                            last = what;
                            what = type::eof;
                            break;
                        }
                        size += view_comment_close.size();
                        page.append(type::comment_begin, data.substr(0, size));
                        data.remove_prefix(size);
                        temp = data;
                        utf::trim_front(temp, whitespaces);
                    }
                    else if (what != type::close_tag && what != type::eof)
                    {
                        fail(last, what);
                        skip(temp, what);
                        diff(temp, data, type::unknown);
                        data = temp;
                    }
                    //p.push_back(std::tuple{ 2, what, last, temp });
                    peek(temp, what, last);
                }
                while (what != type::close_tag && what != type::eof);
                if (what == type::close_tag) // Proceed '</token>'.
                {
                    auto skip_frag = skip(temp, what);
                    auto trim_frag = utf::trim_front(temp, whitespaces);
                    peek(temp, what, last);
                    if (what == type::token)
                    {
                        auto object = name(temp);
                        auto spaced = trim(data);
                        if (object == item->name->utf8)
                        {
                            item->insB = spaced ? page.back
                                                : page.append(type::spaces);
                            page.append(                      type::close_tag, skip_frag);
                            if (trim_frag.size()) page.append(type::spaces,    trim_frag);
                            page.append(                      type::end_token, item->name->utf8);
                            data = temp;
                            auto tail = data.find('>');
                            if (tail != view::npos) data.remove_prefix(tail + 1);
                            else                    data = {};
                            diff(data, temp, type::close_tag);
                            break;
                        }
                        else
                        {
                            what = type::unknown;
                            page.append(                      what, skip_frag);
                            if (trim_frag.size()) page.append(what, trim_frag);
                            page.append(                      what, object);
                            data = temp;
                            auto tail = data.find('>');
                            if (tail != view::npos) data.remove_prefix(tail + 1);
                            else                    data = {};
                            diff(data, temp, what);
                            fail(ansi::add("Unexpected closing tag name '", object, "', expected: '", item->name->utf8, "'"));
                            continue; // Repeat until eof or success.
                        }
                    }
                    else
                    {
                        diff(temp, data, type::unknown);
                        data = temp;
                        fail(last, what);
                        continue; // Repeat until eof or success.
                    }
                }
                else if (what == type::eof)
                {
                    trim(data);
                    if (page.back->kind == type::eof) fail("Unexpected {EOF}");
                }
            }
            while (data.size());
        }
        auto read_node(sptr& item, view& data, si32 deep = {}) -> document::type
        {
            auto defs = utf::unordered_map<text, wptr>{};
            auto what = type::na;
            auto last = type::na;
            auto fire = faux;
            trim(data);
            open(item);
            peek(data, what, last);
            if (what == type::begin_tag)
            {
                page.append(type::begin_tag, skip(data, what));
                trim(data);
                peek(data, what, last);
                if (what == type::token)
                {
                    auto temp = pair(item, data, what, last, type::top_token);
                    while (what == type::compact)
                    {
                        data = temp;
                        page.append(what, skip(data, what));
                        item->mode = elem::form::pact;
                        auto next = ptr::shared<elem>();
                        open(next);
                        page.append(type::begin_tag); // Add begin_tag placeholder.
                        peek(data, what, last);
                        temp = pair(next, data, what, last, type::top_token);
                        auto& sub_name = next->name->utf8;
                        item->hive[sub_name].push_back(next);
                        compacted.push_back(item);
                        item = next;
                    }
                    trim(data);
                    peek(data, what, last);
                    if (what == type::token)
                    {
                        do // Proceed inlined subs.
                        {
                            auto next = ptr::shared<elem>();
                            next->mode = elem::form::attr;
                            open(next);
                            pair(next, data, what, last, type::token);
                            if (last == type::defaults) next->base = true; // Inlined list resetter.
                            seal(next);
                            push(item, next, defs);
                            trim(data);
                            peek(data, what, last);
                        }
                        while (what == type::token);
                    }
                    if (what == type::empty_tag) // Proceed '/>'.
                    {
                        item->mode = elem::form::flat;
                        item->insA = last == type::spaces ? page.back
                                                          : page.append(type::spaces);
                        last = type::spaces;
                        page.append(type::empty_tag, skip(data, what));
                        while (true) // Pull inline comments: .../>  <!-- comments --> ... <!-- comments -->
                        {
                            temp = data;
                            auto idle = utf::trim_front(temp, whitespaces);
                            auto w = what;
                            auto l = last;
                            peek(temp, w, l);
                            if (idle.find('\n') == text::npos && w == type::comment_begin)
                            {
                                data = temp;
                                what = w;
                                last = l;
                                page.append(type::spaces, idle);
                                if (!note(data, what, last)) break;
                            }
                            else break;
                        }
                    }
                    else if (compacted.empty() && what == type::close_inline) // Proceed '>' nested subs.
                    {
                        item->insA = last == type::spaces ? page.back
                                                          : page.append(type::spaces);
                        page.append(type::close_inline, skip(data, what));
                        read_subsections(item, data, what, last, deep, defs);
                    }
                    else fire = true;
                }
                else fire = true;
            }
            else fire = true;
            if (!item->name)
            {
                auto head = page.back;
                while (true) // Reverse find a broken open tag and mark all after it as an unknown data.
                {
                    auto kind = head->kind;
                    head->kind = type::unknown;
                    if (head == page.data || kind == type::begin_tag) break;
                    head = head->prev.lock();
                }
                item->name = page.append(type::tag_value);
                fail("Empty tag name");
            }
            if (fire) fail(last, what);
            if (what == type::eof) page.append(what);
            seal(item);
            while (!compacted.empty()) // Close compact nodes.
            {
                item = compacted.back();
                seal(item);
                compacted.pop_back();
            }
            return what;
        }
        void read(view& data)
        {
            auto defs = utf::unordered_map<text, wptr>{};
            auto what = type::na;
            auto last = type::na;
            auto deep = 0;
            open(root);
            root->mode = elem::form::node;
            root->name = page.append(type::na);
            root->insB = page.append(type::spaces);
            read_subsections(root, data, what, last, deep, defs);
            seal(root);
            if (page.fail) log("%%Inconsistent xml data from %file%:\n%config%\n", prompt::xml, page.file.empty() ? "memory"sv : page.file, page.show());
        }
    };

    struct settings
    {
        using vect = xml::document::vect;
        using sptr = netxs::sptr<xml::document>;
        using hist = std::list<std::pair<text, text>>;

        sptr document; // settings: XML document.
        vect tempbuff; // settings: Temp buffer.
        vect homelist; // settings: Current directory item list.
        text homepath; // settings: Current working directory.
        text backpath; // settings: Fallback path.
        hist cwdstack; // settings: Stack for saving current cwd.

        settings() = default;
        settings(settings const&) = default;
        settings(view utf8_xml)
            : document{ ptr::shared<xml::document>(utf8_xml, "") }
        {
            homepath = "/";
            homelist = document->take(homepath);
        }
        settings(xml::document& d)
            : document{ ptr::shared<xml::document>(std::move(d)) }
        {
            homepath = "/";
            homelist = document->take(homepath);
        }

        auto cd(view gotopath, view fallback = {})
        {
            backpath = utf::trim(fallback, '/');
            if (gotopath.empty()) return faux;
            if (gotopath.front() == '/')
            {
                homepath = "/";
                homepath += utf::trim(gotopath, '/');
                homelist = document->take(homepath);
            }
            else
            {
                auto relative = utf::trim(gotopath, '/');
                if (homelist.size())
                {
                    homelist = homelist.front()->list(relative);
                }
                homepath += "/";
                homepath += relative;
            }
            auto test = !!homelist.size();
            if constexpr (debugmode)
            if (!test)
            {
                log("%%%err%xml path not found: %path%%nil%", prompt::xml, ansi::err(), homepath, ansi::nil());
            }
            return test;
        }
        void popd()
        {
            if (cwdstack.empty())
            {
                log("%%CWD stack is empty", prompt::xml);
            }
            else
            {
                auto& [gotopath, fallback] = cwdstack.back();
                cd(gotopath, fallback);
                cwdstack.pop_back();
            }
        }
        void pushd(view gotopath, view fallback = {})
        {
            cwdstack.push_back({ homepath, backpath });
            cd(gotopath, fallback);
        }
        template<bool Quiet = faux, class T = si32>
        auto take(text frompath, T defval = {}, si32 primary_value = 3) // Three levels of references (to avoid circular references).
        {
            if (frompath.empty()) return defval;
            auto crop = text{};
            if (frompath.front() == '/')
            {
                frompath = utf::trim(frompath, '/');
                tempbuff = document->take(frompath);
            }
            else
            {
                frompath = utf::trim(frompath, '/');
                if (homelist.size()) tempbuff = homelist.front()->list(frompath);
                if (tempbuff.empty() && backpath.size())
                {
                    frompath = backpath + "/" + frompath;
                    tempbuff = document->take(frompath);
                }
                else frompath = homepath + "/" + frompath;
            }
            if (tempbuff.size()) crop = tempbuff.back()->take_value();
            else
            {
                if constexpr (!Quiet) log("%%%red% xml path not found: %nil%%path%", prompt::xml, ansi::fgc(redlt), ansi::nil(), frompath);
                return defval;
            }
            auto is_quoted = tempbuff.back()->is_quoted();
            tempbuff.clear();
            auto is_like_variable = [&]{ return primary_value && !is_quoted && crop.size() && (crop.front() == '/' || crop.size() < 128); };
            if constexpr (std::is_same_v<std::decay_t<T>, text>)
            {
                if (is_like_variable()) // Try to find variable if it is not quoted and its len < 128.
                {
                    return take<Quiet>(crop.front() == '/' ? crop : "/config/variables/" + crop, crop, primary_value - 1);
                }
            }
            if (auto result = xml::take<T>(crop)) return result.value();
            if (is_like_variable())               return take<Quiet>(crop.front() == '/' ? crop : "/config/variables/" + crop, defval, primary_value - 1);
            else                                  return defval;
        }
        auto expand(document::sptr item_ptr, si32 primary_value = 3)
        {
            auto crop = item_ptr->take_value();
            auto is_quoted = item_ptr->is_quoted();
            auto is_like_variable = !is_quoted && primary_value && crop.size() && (crop.front() == '/' || crop.size() < 128);
            if (is_like_variable) // Try to find variable if it is not quoted and its len < 128.
            {
                return take<true>(crop.front() == '/' ? crop : "/config/variables/" + crop, crop, primary_value - 1);
            }
            return crop;
        }
        auto expand_list(document::sptr subsection_ptr, view attribute)
        {
            auto strings = txts{};
            auto attr_list = subsection_ptr->list(attribute);
            strings.reserve(attr_list.size());
            for (auto attr_ptr : attr_list)
            {
                strings.emplace_back(expand(attr_ptr));
            }
            return strings;
        }
        template<class T>
        auto take(text frompath, T defval, utf::unordered_map<text, T> const& dict)
        {
            if (frompath.empty()) return defval;
            auto crop = take<true>(frompath, ""s);
            if (crop.empty())
            {
                log("%%%red% xml path not found: %nil%%path%", prompt::xml, ansi::fgc(redlt), ansi::nil(), frompath);
                return defval;
            }
            auto iter = dict.find(crop);
            return iter == dict.end() ? defval : iter->second;
        }
        auto take(text frompath, cell defval)
        {
            if (frompath.empty()) return defval;
            auto fgc_path = frompath + '/' + "fgc";
            auto bgc_path = frompath + '/' + "bgc";
            auto itc_path = frompath + '/' + "itc";
            auto bld_path = frompath + '/' + "bld";
            auto und_path = frompath + '/' + "und";
            auto inv_path = frompath + '/' + "inv";
            auto ovr_path = frompath + '/' + "ovr";
            auto blk_path = frompath + '/' + "blk";
            auto txt_path = frompath + '/' + "txt";
            auto fba_path = frompath + '/' + "alpha";
            auto crop = cell{ defval.txt() };
            crop.fgc(take<true>(fgc_path, defval.fgc()));
            crop.bgc(take<true>(bgc_path, defval.bgc()));
            crop.itc(take<true>(itc_path, defval.itc()));
            crop.bld(take<true>(bld_path, defval.bld()));
            crop.und(take<true>(und_path, defval.und()));
            crop.inv(take<true>(inv_path, defval.inv()));
            crop.ovr(take<true>(ovr_path, defval.ovr()));
            crop.blk(take<true>(blk_path, defval.blk()));
            auto t = take<true>(txt_path, ""s);
            auto a = take<true>(fba_path, -1);
            if (t.size()) crop.txt(t);
            if (a != -1)  crop.alpha((byte)std::clamp(a, 0, 255));
            return crop;
        }
        template<bool WithTemplate = faux>
        auto list(view frompath)
        {
            if (frompath.empty())        return homelist;
            if (frompath.front() == '/') return document->take<WithTemplate>(frompath);
            if (homelist.size())         return homelist.front()->list<WithTemplate>(frompath);
            else                         return vect{};
        }
        template<class T>
        void set(view frompath, T&& value)
        {
            auto items = list(frompath);
            if (items.empty())
            {
                //todo add new
            }
            else
            {
                auto line = utf::concat(value);
                utf::unescape(line);
                items.front()->init_value(line);
            }
        }
        auto utf8()
        {
            return document->page.utf8();
        }
        template<bool Print = faux>
        auto fuse(view utf8_xml, view filepath = {})
        {
            if (utf8_xml.empty()) return;
            if (filepath.size()) document->page.file = filepath;
            homepath.clear();
            homelist.clear();
            auto tmp_config = xml::document{ utf8_xml, filepath };
            if constexpr (Print)
            {
                log("%%Settings from %file%:\n%config%", prompt::xml, filepath.empty() ? "memory"sv : filepath, tmp_config.page.show());
            }
            auto path = text{};
            document->overlay(tmp_config.root, path);
            homepath = "/";
            homelist = document->take(homepath);
        }
        friend auto& operator << (std::ostream& s, settings const& p)
        {
            return s << p.document->page.show();
        }
    };
    namespace options
    {
        static auto format = utf::unordered_map<text, si32>
           {{ "none",      mime::disabled },
            { "text",      mime::textonly },
            { "ansi",      mime::ansitext },
            { "rich",      mime::richtext },
            { "html",      mime::htmltext },
            { "protected", mime::safetext }};

        static auto cursor = utf::unordered_map<text, si32>
           {{ "underline",  text_cursor::underline },
            { "block",      text_cursor::block     },
            { "bar",        text_cursor::I_bar     },
            { "I_bar",      text_cursor::I_bar     }};

        static auto align = utf::unordered_map<text, bias>
           {{ "left",   bias::left   },
            { "right",  bias::right  },
            { "center", bias::center }};
    }
}
namespace netxs
{
    using xmls = xml::settings;
}