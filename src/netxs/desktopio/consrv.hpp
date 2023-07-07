// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

struct consrv
{
    //static constexpr auto isreal = requires(Term terminal) { terminal.decstr(); }; // MSVC bug: It doesn't see constexpr value everywehere, even constexpr functions inside lambdas.
    static constexpr auto isreal()
    {
        return requires(Term terminal) { terminal.decstr(); };
    }

    struct cook
    {
        wide wstr{};
        text ustr{};
        view rest{};
        ui32 ctrl{};

        auto save(bool isutf16)
        {
            wstr.clear();
            utf::to_utf(ustr, wstr);
            if (isutf16) rest = { reinterpret_cast<char*>(wstr.data()), wstr.size() * 2 };
            else         rest = ustr;
        }
        auto drop()
        {
            ustr.clear();
            wstr.clear();
            rest = ustr;
        }
    };

    struct memo
    {
        size_t                           seek{};
        std::list<std::pair<si32, rich>> undo{};
        std::list<std::pair<si32, rich>> redo{};
        std::vector<rich>                data{};
        rich                             idle{};

        auto save(para& line)
        {
            auto& data = line.content();
            if (undo.empty() || !data.same(undo.back().second))
            {
                undo.emplace_back(line.caret, data);
                redo.clear();
            }
        }
        auto swap(para& line, bool back)
        {
            if (back) std::swap(undo, redo);
            if (undo.size())
            {
                auto& data = line.content();
                while (undo.size() && data.same(undo.back().second)) undo.pop_back();
                if (undo.size())
                {
                    redo.emplace_back(line.caret, data);
                    line.content(undo.back().second);
                    line.caret = undo.back().first;
                    undo.pop_back();
                }
            }
            if (back) std::swap(undo, redo);
        }
        auto done(para& line)
        {
            auto& new_data = line.content();
            auto trimmed = utf::trim(new_data.utf8(), whitespaces);

            if (trimmed.size() && (data.empty() || data.back() != new_data))
            {
                data.push_back(new_data);
                seek = data.size() - 1;
            }
            undo.clear();
            redo.clear();
        }
        auto& fallback() const
        {
            if (data.size()) return data[seek];
            else             return idle;
        }
        auto roll()
        {
            if (data.size())
            {
                if (seek == 0) seek = data.size() - 1;
                else           seek--;
            }
        }
        auto find(para& line)
        {
            if (data.empty()) return faux;
            auto stop = seek;
            auto size = line.caret;
            while (!line.substr(0, size).same(data[seek].substr(0, size))
                 || line.content()      .same(data[seek]))
            {
                roll();
                if (seek == stop) return faux;
            }
            save(line);
            line.content() = data[seek];
            line.caret = size;
            line.caret_check();
            return true;
        }
        auto deal(para& line, size_t i)
        {
            if (i < data.size())
            {
                save(line);
                line.content(data[seek = i]);
            }
        }
        auto prev(para& line) { if (seek > 0 && line.content().same(data[seek])) roll();
                                deal(line, seek           ); }
        auto pgup(para& line) { deal(line, 0              ); }
        auto next(para& line) { deal(line, seek + 1       ); }
        auto pgdn(para& line) { deal(line, data.size() - 1); }
    };

    struct clnt
    {
        struct hndl
        {
            enum type
            {
                unused,
                events,
                scroll,
                altbuf,
            };

            clnt& boss;
            ui32& mode;
            type  kind;
            void* link;
            text  toUTF8;
            text  toANSI;
            wide  toWIDE;

            hndl(clnt& boss, ui32& mode, type kind, void* link)
                : boss{ boss },
                  mode{ mode },
                  kind{ kind },
                  link{ link }
            { }
            friend auto& operator << (std::ostream& s, hndl const& h)
            {
                if (h.kind == hndl::type::events)
                {
                    s << "events(" << h.mode << ") 0";
                    if (h.mode & nt::console::inmode::echo      ) s << " | ECHO";
                    if (h.mode & nt::console::inmode::insert    ) s << " | INSERT";
                    if (h.mode & nt::console::inmode::cooked    ) s << " | COOKED_READ";
                    if (h.mode & nt::console::inmode::mouse     ) s << " | MOUSE_INPUT";
                    if (h.mode & nt::console::inmode::preprocess) s << " | PROCESSED_INPUT";
                    if (h.mode & nt::console::inmode::quickedit ) s << " | QUICK_EDIT";
                    if (h.mode & nt::console::inmode::winsize   ) s << " | WINSIZE";
                    if (h.mode & nt::console::inmode::vt        ) s << " | VIRTUAL_TERMINAL_INPUT";
                }
                else
                {
                    s << "scroll(" << h.mode << ") 0";
                    if (h.mode & nt::console::outmode::preprocess ) s << " | PROCESSED_OUTPUT";
                    if (h.mode & nt::console::outmode::wrap_at_eol) s << " | WRAP_AT_EOL";
                    if (h.mode & nt::console::outmode::vt         ) s << " | VIRTUAL_TERMINAL_PROCESSING";
                    if (h.mode & nt::console::outmode::no_auto_cr ) s << " | NO_AUTO_CR";
                    if (h.mode & nt::console::outmode::lvb_grid   ) s << " | LVB_GRID_WORLDWIDE";
                }
                return s;
            }
        };

        struct info
        {
            ui32 iconid{};
            ui32 hotkey{};
            ui32 config{};
            ui16 colors{};
            ui16 format{};
            twod scroll{};
            rect window{};
            bool cliapp{};
            bool expose{};
            text header{};
            text curexe{};
            text curdir{};
        };

        using list = std::list<hndl>;
        using bufs = std::list<decltype(Term::altbuf)>;

        list tokens; // clnt: Taked handles.
        ui32 procid; // clnt: Process id.
        ui32 thread; // clnt: Process thread id.
        ui32 pgroup; // clnt: Process group id.
        info detail; // clnt: Process details.
        //todo store time stamps for the history items
        memo inputs; // clnt: Input history.
        cell backup; // clnt: Text attributes to restore on detach.
        bufs alters; // clnt: Additional scrollback buffers.
    };

    using hndl = clnt::hndl;

    struct base
    {
        ui64  taskid;
        clnt* client;
        hndl* target;
        ui32  fxtype;
        ui32  packsz;
        ui32  echosz;
        ui32  _pad_1;
    };

    struct task : base
    {
        ui32 callfx;
        ui32 arglen;
        byte argbuf[96];
    };

    template<class Payload>
    struct wrap
    {
        template<class T>
        static auto& cast(T& buffer)
        {
            static_assert(sizeof(T) >= sizeof(Payload));
            return *reinterpret_cast<Payload*>(&buffer);
        }
        template<class T>
        static auto& cast(T* buffer)
        {
            static_assert(sizeof(T) >= sizeof(Payload));
            return *reinterpret_cast<Payload*>(buffer);
        }
        static auto& cast(text& buffer)
        {
            buffer.resize(sizeof(Payload));
            return *reinterpret_cast<Payload*>(buffer.data());
        }
        static auto cast(text& buffer, size_t count)
        {
            buffer.resize(sizeof(Payload) * count);
            return std::span{ reinterpret_cast<Payload*>(buffer.data()), count };
        }
    };

    template<class Payload>
    struct drvpacket : base, wrap<Payload>
    {
        ui32 callfx;
        ui32 arglen;
    };

    struct cdrw
    {
        using cptr = void const*;
        using stat = NTSTATUS;

        struct order
        {
            ui64 taskid;
            cptr buffer;
            ui32 length;
            ui32 offset;
        };

        ui64 taskid;
        stat status;
        ui32 _pad_1;
        ui64 report;
        cptr buffer;
        ui32 length;
        ui32 offset;

        //auto readoffset() const { return static_cast<ui32>(length ? length + sizeof(ui32) * 2 /*sizeof(drvpacket payload)*/ : 0); }
        //auto sendoffset() const { return length; }
        auto readoffset() const { return _pad_1; }
        auto sendoffset() const { return length; }
        template<class T>
        auto recv_data(fd_t condrv, T&& buffer)
        {
            auto request = order
            {
                .taskid = taskid,
                .buffer = buffer.data(),
                .length = static_cast<decltype(order::length)>(buffer.size() * sizeof(buffer.front())),
                .offset = readoffset(),
            };
            auto rc = nt::ioctl(nt::console::op::read_input, condrv, request);
            if (rc != ERROR_SUCCESS)
            {
                if constexpr (debugmode) log("\tAbort reading input (condrv, request) rc=", rc);
                status = nt::status::unsuccessful;
                return faux;
            }
            return true;
        }
        template<bool Complete = faux, class T>
        auto send_data(fd_t condrv, T&& buffer, bool inc_nul_terminator = faux)
        {
            auto result = order
            {
                .taskid = taskid,
                .buffer = buffer.data(),
                .length = static_cast<decltype(order::length)>((buffer.size() + inc_nul_terminator) * sizeof(buffer.front())),
                .offset = sendoffset(),
            };
            auto rc = nt::ioctl(nt::console::op::write_output, condrv, result);
            if (rc != ERROR_SUCCESS)
            {
                if constexpr (debugmode) log("\tnt::console::op::write_output returns unexpected result ", utf::to_hex(rc));
                status = nt::status::unsuccessful;
                result.length = 0;
            }
            report = result.length;
            if constexpr (Complete) //Note: Be sure that packet.reply.bytes or count is set.
            {
                auto rc = nt::ioctl(nt::console::op::complete_io, condrv, *this);
            }
        }
        auto interrupt(fd_t condrv)
        {
            status = nt::status::invalid_handle;
            auto rc = nt::ioctl(nt::console::op::complete_io, condrv, *this);
            if constexpr (debugmode) log("\tPending operation aborted");
        }
    };

    struct evnt
    {
        using jobs = generics::jobs<std::tuple<cdrw, decltype(base{}.target), bool>>;
        using fire = netxs::os::io::fire;
        using lock = std::recursive_mutex;
        using sync = std::condition_variable_any;
        using vect = std::vector<INPUT_RECORD>;
        using irec = INPUT_RECORD;

        static constexpr auto shift_pressed = ui32{ SHIFT_PRESSED                          };
        static constexpr auto alt___pressed = ui32{ LEFT_ALT_PRESSED  | RIGHT_ALT_PRESSED  };
        static constexpr auto ctrl__pressed = ui32{ LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED };
        static constexpr auto altgr_pressed = ui32{ alt___pressed | ctrl__pressed          };

        struct nttask
        {
            ui32 procid;
            ui32 _pad_1;
            ui64 _pad_2;
            si32 action;
            ui32 _pad_3;
        };

        consrv& server; // events_t: Console server reference.
        vect    buffer; // events_t: Input event list.
        vect    recbuf; // events_t: Temporary buffer for copying event records.
        sync    signal; // events_t: Input event append signal.
        lock    locker; // events_t: Input event buffer mutex.
        cook    cooked; // events_t: Cooked input string.
        jobs    worker; // events_t: Background task executer.
        bool    closed; // events_t: Console server was shutdown.
        fire    ondata; // events_t: Signal on input buffer data.
        wide    wcpair; // events_t: Surrogate pair buffer.
        wide    toWIDE; // events_t: UTF-16 decoder buffer.
        text    toUTF8; // events_t: UTF-8  decoder buffer.
        text    toANSI; // events_t: ANSI   decoder buffer.
        irec    leader; // events_t: Hanging key event record (lead byte).

        evnt(consrv& serv)
            :  server{ serv },
               closed{ faux },
               ondata{ true },
               leader{      }
        { }

        void reset()
        {
            auto lock = std::lock_guard{ locker };
            closed = faux;
            buffer.clear();
            ondata.flush();
        }
        auto count()
        {
            auto lock = std::lock_guard{ locker };
            return static_cast<ui32>(buffer.size());
        }
        void abort(hndl* handle_ptr)
        {
            auto lock = std::lock_guard{ locker };
            worker.cancel([&](auto& token)
            {
                auto& answer = std::get<0>(token);
                auto& target = std::get<1>(token);
                auto& cancel = std::get<2>(token);
                if (target == handle_ptr)
                {
                    cancel = true;
                    answer.interrupt(server.condrv);
                }
            });
            signal.notify_all();
        }
        void alert(si32 what, ui32 pgroup = 0)
        {
            if (server.io_log)
            {
                log(server.prompt, what == CTRL_C_EVENT     ? "Ctrl+C"
                                 : what == CTRL_BREAK_EVENT ? "Ctrl+Break"
                                 : what == CTRL_CLOSE_EVENT ? "CtrlClose"
                                                            : "Unknown", " event");
            }
            for (auto& client : server.joined)
            {
                if (!pgroup || pgroup == client.pgroup)
                {
                    auto task = nttask{ .procid = client.procid, .action = what };
                    auto stat = os::nt::UserConsoleControl((ui32)sizeof("Ending"), &task, (ui32)sizeof(task));
                    if (server.io_log)
                    {
                        log("\tclient proc id ", client.procid,  ", control status ", utf::to_hex_0x(stat));
                    }
                }
            }
        }
        void stop()
        {
            auto lock = std::lock_guard{ locker };
            alert(CTRL_CLOSE_EVENT);
            closed = true;
            signal.notify_all();
        }
        void clear()
        {
            auto lock = std::lock_guard{ locker };
            ondata.flush();
            buffer.clear();
            recbuf.clear();
            cooked.drop();
        }
        auto generate(wchr ch, ui32 st = 0, ui16 vc = 0, si32 dn = 1, ui16 sc = 0)
        {
            buffer.emplace_back(INPUT_RECORD
            {
                .EventType = KEY_EVENT,
                .Event =
                {
                    .KeyEvent =
                    {
                        .bKeyDown               = dn,
                        .wRepeatCount           = 1,
                        .wVirtualKeyCode        = vc,
                        .wVirtualScanCode       = sc,
                        .uChar = { .UnicodeChar = ch },
                        .dwControlKeyState      = st,
                    }
                }
            });
            return true;
        }
        auto generate(wchr c1, wchr c2)
        {
            buffer.reserve(buffer.size() + 2);
            generate(c1);
            generate(c2);
            return true;
        }
        auto generate(wiew wstr, ui32 s = 0)
        {
            buffer.reserve(wstr.size());
            auto head = wstr.begin();
            auto tail = wstr.end();
            //auto noni = server.inpmod & nt::console::inmode::preprocess; // `-NonInteractive` powershell mode.
            while (head != tail)
            {
                auto c = *head++;
                if (c == '\n' || c == '\r')
                {
                    if (head != tail && *head == (c == '\n' ? '\r' : '\n')) head++; // Eat CR+LF/LF+CR.
                    generate('\r', s, key::vk::Enter, 1, 0x1c /*takevkey<key::vk::Enter>().key*/); // Emulate hitting Enter.
                    // Far Manager treats Shift+Enter as its own macro not a soft break.
                    //if (noni) generate('\n', s);
                    //else      generate('\r', s | SHIFT_PRESSED, key::vk::Enter, 1, 0x1c /*takevkey<key::vk::Enter>().key*/); // Emulate hitting Enter. Pressed Shift to soft line break when pasting from clipboard.
                }
                else
                {
                    generate(c, s);
                }
            }
            return true;
        }
        auto generate(view ustr)
        {
            toWIDE.clear();
            utf::to_utf(ustr, toWIDE);
            return generate(toWIDE);
        }
        auto write(view ustr)
        {
            auto lock = std::lock_guard{ locker };
            generate(ustr);
            if (!buffer.empty())
            {
                ondata.reset();
                signal.notify_one();
            }
        }
        void focus(bool state)
        {
            auto lock = std::lock_guard{ locker };
            buffer.emplace_back(INPUT_RECORD
            {
                .EventType = FOCUS_EVENT,
                .Event =
                {
                    .FocusEvent =
                    {
                        .bSetFocus = state,
                    }
                }
            });
            ondata.reset();
            signal.notify_one();
        }
        void mouse(input::hids& gear, bool moved, twod const& coord)
        {
            auto state = os::nt::ms_kbstate(gear.ctlstate);
            auto bttns = gear.m.buttons & 0b00011111;
            auto flags = ui32{};
            if (moved         ) flags |= MOUSE_MOVED;
            if (gear.m.doubled) flags |= DOUBLE_CLICK;
            if (gear.m.wheeldt)
            {
                     if (gear.m.wheeled) flags |= MOUSE_WHEELED;
                else if (gear.m.hzwheel) flags |= MOUSE_HWHEELED;
                bttns |= gear.m.wheeldt << 16;
            }
            auto lock = std::lock_guard{ locker };
            buffer.emplace_back(INPUT_RECORD
            {
                .EventType = MOUSE_EVENT,
                .Event =
                {
                    .MouseEvent =
                    {
                        .dwMousePosition =
                        {
                            .X = (si16)std::clamp<si32>(coord.x, 0, std::numeric_limits<si16>::max()),
                            .Y = (si16)std::clamp<si32>(coord.y, 0, std::numeric_limits<si16>::max()),
                        },
                        .dwButtonState     = bttns,
                        .dwControlKeyState = state,
                        .dwEventFlags      = flags,
                    }
                }
            });
            ondata.reset();
            signal.notify_one();
        }
        void winsz(twod const& winsize)
        {
            auto lock = std::lock_guard{ locker };
            buffer.emplace_back(INPUT_RECORD // Ignore ENABLE_WINDOW_INPUT - we only signal a viewport change.
            {
                .EventType = WINDOW_BUFFER_SIZE_EVENT,
                .Event =
                {
                    .WindowBufferSizeEvent =
                    {
                        .dwSize =
                        {
                            .X = (si16)std::min<si32>(winsize.x, std::numeric_limits<si16>::max()),
                            .Y = (si16)std::min<si32>(winsize.y, std::numeric_limits<si16>::max()),
                        }
                    }
                }
            });
            ondata.reset();
            signal.notify_one();
        }
        template<char C>
        static auto takevkey()
        {
            struct vkey { si16 key, vkey; ui32 base; };
            static auto x = ::VkKeyScanW(C);
            static auto k = vkey{ x, x & 0xff, x & 0xff |((x & 0x0100 ? shift_pressed : 0)
                                                        | (x & 0x0200 ? ctrl__pressed : 0)
                                                        | (x & 0x0400 ? alt___pressed : 0)) << 8 };
            return k;
        }
        template<char C>
        static auto truechar(ui16 v, ui32 s)
        {
            static auto x = takevkey<C>();
            static auto need_shift = !!(x.key & 0x100);
            static auto need__ctrl = !!(x.key & 0x200);
            static auto need___alt = !!(x.key & 0x400);
            return v == x.vkey && need_shift == !!(s & shift_pressed)
                               && need__ctrl == !!(s & ctrl__pressed)
                               && need___alt == !!(s & alt___pressed);
        }
        static auto mapvkey(wchr& c, ui16 v)
        {
            return v != VK_CONTROL && v != VK_SHIFT && (c = ::MapVirtualKeyW(v, MAPVK_VK_TO_CHAR) & 0xffff);
        }
        auto vtencode(input::hids& gear, si32 ctrls, bool decckm, wchr c)
        {
            static auto truenull = takevkey<'\0'>().vkey;
            static auto alonekey = std::unordered_map<ui16, wide>
            {
                { VK_BACK,   L"\x7f"     },
                { VK_TAB,    L"\x09"     },
                { VK_PAUSE,  L"\x1a"     },
                { VK_ESCAPE, L"\033"     },
                { VK_PRIOR,  L"\033[5~"  },
                { VK_NEXT,   L"\033[6~"  },
                { VK_END,    L"\033[F"   },
                { VK_HOME,   L"\033[H"   },
                { VK_LEFT,   L"\033[D"   },
                { VK_UP,     L"\033[A"   },
                { VK_RIGHT,  L"\033[C"   },
                { VK_DOWN,   L"\033[B"   },
                { VK_INSERT, L"\033[2~"  },
                { VK_DELETE, L"\033[3~"  },
                { VK_F1,     L"\033OP"   },
                { VK_F2,     L"\033OQ"   },
                { VK_F3,     L"\033OR"   },
                { VK_F4,     L"\033OS"   },
                { VK_F5,     L"\033[15~" },
                { VK_F6,     L"\033[17~" },
                { VK_F7,     L"\033[18~" },
                { VK_F8,     L"\033[19~" },
                { VK_F9,     L"\033[20~" },
                { VK_F10,    L"\033[21~" },
                { VK_F11,    L"\033[23~" },
                { VK_F12,    L"\033[24~" },
            };
            static auto shiftkey = std::unordered_map<ui16, wide>
            {
                { VK_PRIOR,  L"\033[5; ~"  },
                { VK_NEXT,   L"\033[6; ~"  },
                { VK_END,    L"\033[1; F"  },
                { VK_HOME,   L"\033[1; H"  },
                { VK_LEFT,   L"\033[1; D"  },
                { VK_UP,     L"\033[1; A"  },
                { VK_RIGHT,  L"\033[1; C"  },
                { VK_DOWN,   L"\033[1; B"  },
                { VK_INSERT, L"\033[2; ~"  },
                { VK_DELETE, L"\033[3; ~"  },
                { VK_F1,     L"\033[1; P"  },
                { VK_F2,     L"\033[1; Q"  },
                { VK_F3,     L"\033[1; R"  },
                { VK_F4,     L"\033[1; S"  },
                { VK_F5,     L"\033[15; ~" },
                { VK_F6,     L"\033[17; ~" },
                { VK_F7,     L"\033[18; ~" },
                { VK_F8,     L"\033[19; ~" },
                { VK_F9,     L"\033[20; ~" },
                { VK_F10,    L"\033[21; ~" },
                { VK_F11,    L"\033[23; ~" },
                { VK_F12,    L"\033[24; ~" },
            };
            static auto specials = std::unordered_map<ui32, wide>
            {
                { VK_BACK              | ctrl__pressed << 8, { L"\x08"      }},
                { VK_BACK              | alt___pressed << 8, { L"\033\x7f"  }},
                { VK_BACK              | altgr_pressed << 8, { L"\033\x08"  }},
                { VK_TAB               | ctrl__pressed << 8, { L"\t"        }},
                { VK_TAB               | shift_pressed << 8, { L"\033[Z"    }},
                { VK_TAB               | alt___pressed << 8, { L"\033[1;3I" }},
                { VK_ESCAPE            | alt___pressed << 8, { L"\033\033"  }},
                { '1'                  | ctrl__pressed << 8, { L"1"         }},
                { '3'                  | ctrl__pressed << 8, { L"\x1b"      }},
                { '4'                  | ctrl__pressed << 8, { L"\x1c"      }},
                { '5'                  | ctrl__pressed << 8, { L"\x1d"      }},
                { '6'                  | ctrl__pressed << 8, { L"\x1e"      }},
                { '7'                  | ctrl__pressed << 8, { L"\x1f"      }},
                { '8'                  | ctrl__pressed << 8, { L"\x7f"      }},
                { '9'                  | ctrl__pressed << 8, { L"9"         }},
                { VK_DIVIDE            | ctrl__pressed << 8, { L"\x1f"      }},
                { takevkey<'?'>().base | altgr_pressed << 8, { L"\033\x7f"  }},
                { takevkey<'?'>().base | ctrl__pressed << 8, { L"\x7f"      }},
                { takevkey<'/'>().base | altgr_pressed << 8, { L"\033\x1f"  }},
                { takevkey<'/'>().base | ctrl__pressed << 8, { L"\x1f"      }},
            };

            if (server.inpmod & nt::console::inmode::vt && gear.pressed)
            {
                auto& s = ctrls;
                auto& v = gear.virtcod;

                if (s & LEFT_CTRL_PRESSED && s & RIGHT_ALT_PRESSED) // This combination is already translated.
                {
                    s &= ~(LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
                }

                auto shift = s & shift_pressed ? shift_pressed : 0;
                auto alt   = s & alt___pressed ? alt___pressed : 0;
                auto ctrl  = s & ctrl__pressed ? ctrl__pressed : 0;
                if (shift || alt || ctrl)
                {
                    if (ctrl && alt) // c == 0 for ctrl+alt+key combinationsons on windows.
                    {
                        auto a = c ? c : v; // Chars and vkeys for ' '(0x20),'A'-'Z'(0x41-5a) are the same on windows.
                             if (a == 0x20 || (a >= 0x41 && a <= 0x5a)) return generate('\033', (wchr)( a  & 0b00011111)); // Alt causes to prepend '\033'. Ctrl trims by 0b00011111.
                        else if (c == 0x00 && v == truenull           ) return generate('\033', (wchr)('@' & 0b00011111)); // Map ctrl+alt+@ to ^[^@;
                    }

                    if (auto iter = shiftkey.find(v); iter != shiftkey.end())
                    {
                        auto& mods = *++(iter->second.rbegin());
                        mods = '1';
                        if (shift) mods += 1;
                        if (alt  ) mods += 2;
                        if (ctrl ) mods += 4;
                        return generate(iter->second);
                    }
                    else if (auto iter = specials.find(v | (shift | alt | ctrl) << 8); iter != specials.end())
                    {
                        return generate(iter->second);
                    }
                    else if (!ctrl &&  alt && c) return generate('\033', c);
                    else if ( ctrl && !alt)
                    {
                             if (c == 0x20 || (c == 0x00 && v == truenull)) return generate('@' & 0b00011111, s, truenull); // Detect ctrl+@ and ctrl+space.
                        else if (c == 0x00 && mapvkey(c, v))                return generate( c  & 0b00011111); // Emulate ctrl+key mapping to C0 if current kb layout does not contain it.
                    }
                }

                if (auto iter = alonekey.find(v); iter != alonekey.end())
                {
                    if (v >= VK_END && v <= VK_DOWN) iter->second[1] = decckm ? 'O' : '[';
                    return generate(iter->second);
                }
                else if (c) return generate(c); //todo check surrogate pairs
            }
            return faux;
        }
        void keybd(input::hids& gear, bool decckm)
        {
            auto lock = std::lock_guard{ locker };
            toWIDE.clear();
            utf::to_utf(gear.cluster, toWIDE);
            if (toWIDE.empty()) toWIDE.push_back(0);
            auto c = toWIDE.front();

            auto ctrls = os::nt::ms_kbstate(gear.ctlstate) | (gear.extflag ? ENHANCED_KEY : 0);
            if (toWIDE.size() > 1) // Surrogate pair special case (not a clipboard paste, see generate(wiew wstr, ui32 s = 0)).
            {
                if (gear.pressed) // Proceed push events only.
                {
                    for (auto c : toWIDE)
                    {
                        generate(c, ctrls, gear.virtcod, 1, gear.scancod);
                        generate(c, ctrls, gear.virtcod, 0, gear.scancod);
                    }
                }
            }
            else if (!vtencode(gear, ctrls, decckm, c))
            {
                generate(c, ctrls, gear.virtcod, gear.pressed, gear.scancod);
            }

            if (c == ansi::c0_etx)
            {
                if (gear.keybd::scancod == ansi::ctrl_break)
                {
                    alert(CTRL_BREAK_EVENT);
                    buffer.pop_back();
                }
                else
                {
                    if (server.inpmod & nt::console::inmode::preprocess)
                    {
                        if (gear.pressed) alert(CTRL_C_EVENT);
                        //todo revise
                        //buffer.pop_back();
                    }
                }
            }

            if (!buffer.empty())
            {
                ondata.reset();
                signal.notify_one();
            }
        }
        void undo(bool undoredo)
        {
            auto lock = std::lock_guard{ locker };
            generate({}, {}, undoredo ? key::vk::Undo : key::vk::Redo);
            ondata.reset();
            signal.notify_one();
        }
        template<class L>
        auto readline(L& lock, bool& cancel, bool utf16, bool EOFon, ui32 stops, memo& hist)
        {
            //todo bracketed paste support
            // save server.uiterm.bpmode
            // server.uiterm.bpmode = true;
            // restore at exit
            auto mode = testy<bool>{ !!(server.inpmod & nt::console::inmode::insert) };
            auto buff = text{};
            auto nums = utfx{};
            auto line = para{ cooked.ustr };
            auto done = faux;
            auto crlf = 0;
            auto burn = [&]
            {
                if (buff.size())
                {
                    hist.save(line);
                    line.insert(buff, mode);
                    buff.clear();
                }
            };

            do
            {
                auto coor = line.caret;
                auto last = line.length();
                auto pops = 0_sz;
                for (auto& rec : buffer)
                {
                    if (server.io_log)
                    {
                        if (rec.EventType == KEY_EVENT)
                        log(prompt::cin, ansi::hi(utf::debase<faux, faux>(utf::to_utf(rec.Event.KeyEvent.uChar.UnicodeChar))),
                            " ", rec.Event.KeyEvent.bKeyDown ? "Pressed " : "Released",
                            " ctrl: ", utf::to_hex_0x(rec.Event.KeyEvent.dwControlKeyState),
                            " char: ", utf::to_hex_0x(rec.Event.KeyEvent.uChar.UnicodeChar),
                            " virt: ", utf::to_hex_0x(rec.Event.KeyEvent.wVirtualKeyCode),
                            " scan: ", utf::to_hex_0x(rec.Event.KeyEvent.wVirtualScanCode),
                            " rept: ",                rec.Event.KeyEvent.wRepeatCount);
                    }

                    auto& v = rec.Event.KeyEvent.wVirtualKeyCode;
                    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
                    {
                        auto& c = rec.Event.KeyEvent.uChar.UnicodeChar;
                        auto& n = rec.Event.KeyEvent.wRepeatCount;
                        cooked.ctrl = rec.Event.KeyEvent.dwControlKeyState;
                        auto contrl = cooked.ctrl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED);
                        if (c == 0x7f && v == 0) v = VK_BACK;
                        switch (v)
                        {
                            case VK_CONTROL:
                            case VK_SHIFT:
                            case VK_MENU:/*Alt*/
                            case VK_PAUSE:
                            case VK_LWIN:
                            case VK_RWIN:
                            case VK_APPS:
                            case VK_NUMLOCK:
                            case VK_CAPITAL:
                            case VK_SCROLL:
                            case VK_CLEAR:/*NUMPAD 5*/
                            case VK_F2:  //todo menu
                            case VK_F4:  //todo menu
                            case VK_F7:  //todo menu
                            case VK_F9:  //todo menu
                            case VK_F10: //todo clear exes aliases
                            case VK_F11:
                            case VK_F12:
                                break;
                            case VK_INSERT: burn(); mode(!mode);                                                                   break;
                            case VK_F6:     burn(); hist.save(line);               line.insert(cell{}.c0_to_txt('Z' - '@'), mode); break;
                            case VK_ESCAPE: burn(); hist.save(line);               line.wipe();                                    break;
                            case VK_HOME:   burn(); hist.save(line);               line.move_to_home(contrl);                      break;
                            case VK_END:    burn(); hist.save(line);               line.move_to_end (contrl);                      break;
                            case VK_BACK:   burn(); hist.save(line); while (n-- && line.wipe_rev(contrl)) { }                      break;
                            case VK_DELETE: burn(); hist.save(line); while (n-- && line.wipe_fwd(contrl)) { }                      break;
                            case VK_LEFT:   burn();                  while (n-- && line.step_rev(contrl)) { }                      break;
                            case VK_F1:     contrl = faux;
                            case key::vk::RightArrow:  burn(); hist.save(line); while (n-- && line.step_fwd(contrl, hist.fallback())) { }     break;
                            case key::vk::F3:     burn(); hist.save(line); while (       line.step_fwd(faux,   hist.fallback())) { }     break;
                            case key::vk::F8:     burn();                  while (n-- && hist.find(line)) { };                           break;
                            case key::vk::Undo:  while (n--) hist.swap(line, faux); break;
                            case key::vk::Redo:  while (n--) hist.swap(line, true); break;
                            case VK_PRIOR:  burn(); hist.pgup(line);                                                               break;
                            case VK_NEXT:   burn(); hist.pgdn(line);                                                               break;
                            case VK_F5:
                            case VK_UP:     burn(); hist.prev(line);                                                               break;
                            case VK_DOWN:   burn(); hist.next(line);                                                               break;
                            case VK_NUMPAD0:
                            case VK_NUMPAD1:
                            case VK_NUMPAD2:
                            case VK_NUMPAD3:
                            case VK_NUMPAD4:
                            case VK_NUMPAD5:
                            case VK_NUMPAD6:
                            case VK_NUMPAD7:
                            case VK_NUMPAD8:
                            case VK_NUMPAD9:
                            if (cooked.ctrl & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) // Process Alt+Numpad input.
                            {
                                while (n--) nums = nums * 10 + v - VK_NUMPAD0;
                                break;
                            }
                            default:
                            {
                                n--;
                                if (c < ' ')
                                {
                                    auto cook = [&](auto c, auto crlf_value)
                                    {
                                        done = true;
                                        crlf = crlf_value;
                                        burn();
                                        cooked.ustr.clear();
                                        if (crlf_value)
                                        {
                                            line.lyric->utf8(cooked.ustr);
                                            cooked.ustr.push_back('\r');
                                            if (server.inpmod & nt::console::inmode::preprocess)
                                            {
                                                cooked.ustr.push_back('\n');
                                            }
                                        }
                                        else
                                        {
                                            hist.save(line);
                                            line.move_to_end(true);
                                            line.lyric->utf8(cooked.ustr);
                                            cooked.ustr.push_back((char)c);
                                        }
                                        if (n == 0) pops++;
                                    };
                                         if (stops & 1 << c && (c != '\t' || c == '\t' && v == VK_TAB)) { cook(c, 0); hist.save(line);                            }
                                    else if (c == '\r' || c == '\n')                                    { cook(c, 1); hist.done(line);                            }
                                    else if (c == '\t')                                                 { burn();     hist.save(line); line.insert("    ", mode); }
                                    else if (c == 'C' - '@')
                                    {
                                        hist.save(line);
                                        cooked.ustr = "\n";
                                        done = true;
                                        crlf = 2;
                                        line.insert(cell{}.c0_to_txt(c), mode);
                                        if (n == 0) pops++;
                                    }
                                    else
                                    {
                                        burn();
                                        hist.save(line);
                                        line.insert(cell{}.c0_to_txt(c), mode);
                                    }
                                }
                                else
                                {
                                    auto grow = utf::to_utf(c, wcpair, buff);
                                    if (grow && n)
                                    {
                                        auto temp = view{ buff.data() + grow, buff.size() - grow };
                                        while (n--)
                                        {
                                            buff += temp;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (nums && v == VK_MENU) // Alt is released after num digits input.
                    {
                        server.inpenc->decode(nums, buff);
                        nums = 0;
                    }
                    if (done) break;
                    else      pops++;
                }

                if (pops)
                {
                    auto head = buffer.begin();
                    auto tail = head + pops;
                    buffer.erase(head, tail);
                    pops = {};
                }

                burn();

                if (server.inpmod & nt::console::inmode::echo)
                {
                    lock.unlock();
                    if constexpr (isreal())
                    {
                        server.uiterm.update([&]
                        {
                            static auto empty = cell{ emptyspace }.wdt(1);
                            static auto erase = cell{ whitespace }.wdt(1);
                            auto& term = *server.uiterm.target;
                            auto& data = line.content();
                            auto  size = line.length();
                                 if (size < last)        data.crop(last + 0, erase); // Erase trailing cells when shrinking.
                            else if (size == line.caret) data.crop(size + 1, empty); // Avoid pending cursor.
                            term.move(-coor);
                            term.data(data);

                            if (done && crlf && server.inpmod & nt::console::inmode::preprocess) // On PROCESSED_INPUT + ECHO_INPUT only.
                            {
                                term.cr();
                                term.lf(crlf);
                            }
                            else term.move(line.caret - line.length());

                            if (mode.reset())
                            {
                                if constexpr (isreal())
                                {
                                    server.uiterm.cursor.toggle();
                                }
                            }
                            data.crop(size);
                        });
                    }
                    else
                    {
                        auto& data = line.content();
                        server.uiterm.data(data);
                    }
                    lock.lock();
                }
            }
            while (!done && ((void)signal.wait(lock, [&]{ return buffer.size() || closed || cancel; }), !closed && !cancel));

            if (EOFon)
            {
                static constexpr auto EOFkey = 'Z' - '@';
                auto EOFpos = cooked.ustr.find(EOFkey);
                if (EOFpos != text::npos) cooked.ustr.resize(EOFpos);
            }

            cooked.save(utf16);
            if (buffer.empty()) ondata.flush();

            server.inpmod = (server.inpmod & ~nt::console::inmode::insert) | (mode ? nt::console::inmode::insert : 0);
        }
        template<class L>
        auto readchar(L& lock, bool& cancel, bool utf16)
        {
            do
            {
                for (auto& rec : buffer) if (rec.EventType == KEY_EVENT)
                {
                    auto& s = rec.Event.KeyEvent.dwControlKeyState;
                    auto& d = rec.Event.KeyEvent.bKeyDown;
                    auto& n = rec.Event.KeyEvent.wRepeatCount;
                    auto& v = rec.Event.KeyEvent.wVirtualKeyCode;
                    auto& c = rec.Event.KeyEvent.uChar.UnicodeChar;
                    cooked.ctrl = s;
                    if (n-- && (d && (c || truechar<'\0'>(v, s))
                            || !d &&  c && v == VK_MENU))
                    {
                        auto grow = utf::to_utf(c, wcpair, cooked.ustr);
                        if (grow && n)
                        {
                            auto temp = view{ cooked.ustr.data() + cooked.ustr.size() - grow, grow };
                            while (n--) cooked.ustr += temp;
                        }
                    }
                }
                buffer.clear(); // Don't try to catch the next events (we are too fast for IME input; ~1ms between events from IME). 
            }
            while (cooked.ustr.empty() && ((void)signal.wait(lock, [&]{ return buffer.size() || closed || cancel; }), !closed && !cancel));

            cooked.save(utf16);
            ondata.flush();
        }
        template<bool Complete = faux, class Payload>
        auto reply(Payload& packet, cdrw& answer, ui32 readstep)
        {
            auto& inpenc = *server.inpenc;
            if (server.io_log) log("\thandle ", utf::to_hex_0x(packet.target), ":",
                                 "\n\tbuffered ", Complete ? "read: " : "rest: ", ansi::hi(utf::debase<faux, faux>(cooked.ustr)),
                                 "\n\treply ", server.show_page(packet.input.utf16, inpenc.codepage), ":");
            if (packet.input.utf16 || inpenc.codepage == CP_UTF8)
            {
                auto size = std::min((ui32)cooked.rest.size(), readstep);
                auto data = view{ cooked.rest.data(), size };
                if (server.io_log) log("\t", ansi::hi(utf::debase<faux, faux>(packet.input.utf16 ? utf::to_utf((wchr*)data.data(), data.size() / sizeof(wchr))
                                                                                                 : data)));
                cooked.rest.remove_prefix(size);
                packet.reply.ctrls = cooked.ctrl;
                packet.reply.bytes = size;
                answer.send_data<Complete>(server.condrv, data);
            }
            else
            {
                auto data = inpenc.encode(cooked.rest, readstep);
                auto size = (ui32)data.size();
                if (server.io_log) log("\t", ansi::hi(utf::debase<faux, faux>(inpenc.decode_log(data))));
                packet.reply.ctrls = cooked.ctrl;
                packet.reply.bytes = size;
                answer.send_data<Complete>(server.condrv, data);
            }
        }
        template<class Payload>
        auto placeorder(Payload& packet, wiew nameview, view initdata, ui32 readstep)
        {
            auto lock = std::lock_guard{ locker };
            if (cooked.rest.empty())
            {
                // Notes:
                //  - input buffer is shared across the process tree
                //  - input history menu takes keypresses from the shared input buffer
                //  - '\n' is added to the '\r'

                auto token = jobs::token(server.answer, packet.target, faux);
                worker.add(token, [&, readstep, packet, initdata = text{ initdata }, nameview = utf::to_utf(nameview)](auto& token) mutable
                {
                    auto lock = std::unique_lock{ locker };
                    auto& answer = std::get<0>(token);
                    auto& cancel = std::get<2>(token);
                    auto& client = *packet.client;
                    answer.buffer = &packet.input; // Restore after copy. Payload start address.

                    if (closed || cancel) return;

                    cooked.ustr.clear();
                    if (server.inpmod & nt::console::inmode::cooked)
                    {
                        if (packet.input.utf16) utf::to_utf((wchr*)initdata.data(), initdata.size() / 2, cooked.ustr);
                        else                    cooked.ustr = initdata;
                        readline(lock, cancel, packet.input.utf16, packet.input.EOFon, packet.input.stops, client.inputs);
                    }
                    else
                    {
                        readchar(lock, cancel, packet.input.utf16);
                    }
                    if (closed || cancel)
                    {
                        if (server.io_log) log("\thandle ", utf::to_hex_0x(packet.target), ": task canceled");
                        cooked.drop();
                        return;
                    }
                    reply<true>(packet, answer, readstep);
                });
                server.answer = {};
            }
            else reply(packet, server.answer, readstep);
        }
        template<class T>
        auto sendevents(T&& recs, bool utf16)
        {
            auto lock = std::lock_guard{ locker };
            auto& inpenc = *server.inpenc;
            if (utf16 || inpenc.codepage == CP_UTF8) // Store UTF-8 as is (I see no reason to decode).
            {
                buffer.insert(buffer.end(), recs.begin(), recs.end());
            }
            else
            {
                auto head = recs.begin();
                auto tail = recs.end();
                while (head != tail)
                {
                    auto& coming = *head++;
                    if (coming.EventType == KEY_EVENT)
                    {
                        auto lead = (byte)leader.Event.KeyEvent.uChar.AsciiChar;
                        auto next = (byte)coming.Event.KeyEvent.uChar.AsciiChar;
                        if (lead)
                        {
                            if (leader.Event.KeyEvent.wVirtualKeyCode == coming.Event.KeyEvent.wVirtualKeyCode)
                            {
                                coming.Event.KeyEvent.uChar.UnicodeChar = inpenc.decode(lead, next);
                            }
                            leader = {}; // Drop hanging lead byte.
                        }
                        else
                        {
                            if (inpenc.test(next))
                            {
                                leader = coming;
                                continue;
                            }
                            else coming.Event.KeyEvent.uChar.UnicodeChar = inpenc.decode(next);
                        }
                    }
                    buffer.insert(buffer.end(), coming);
                }
            }
            if (!buffer.empty())
            {
                ondata.reset();
                signal.notify_one();
            }
        }
        template<class T>
        void logbuf(T&& recs)
        {
            if (recs.empty()) return;
            auto crop = ansi::add("\treply.count: ", recs.size(), '\n');
            for (auto& r : recs)
            {
                switch (r.EventType)
                {
                    case KEY_EVENT:
                        crop.add("\ttype: key",
                                " ctrl:", utf::to_hex_0x(r.Event.KeyEvent.dwControlKeyState),
                                " vcod:", utf::to_hex_0x(r.Event.KeyEvent.wVirtualKeyCode),
                                " scod:", utf::to_hex_0x(r.Event.KeyEvent.wVirtualScanCode),
                                " wchr:", utf::to_hex_0x(r.Event.KeyEvent.uChar.UnicodeChar),
                                " down:",                r.Event.KeyEvent.bKeyDown ? '1':'0',
                                " count:",               r.Event.KeyEvent.wRepeatCount, '\n');
                        break;
                    case MOUSE_EVENT:
                        crop.add("\ttype: mouse",
                                " ctrl:", utf::to_hex_0x(r.Event.MouseEvent.dwControlKeyState),
                                " coor:",          twod{ r.Event.MouseEvent.dwMousePosition.X, r.Event.MouseEvent.dwMousePosition.Y },
                                " bttn:", utf::to_hex_0x(r.Event.MouseEvent.dwButtonState),
                                " flag:", utf::to_hex_0x(r.Event.MouseEvent.dwEventFlags), '\n');
                        break;
                    case WINDOW_BUFFER_SIZE_EVENT:
                        crop.add("\ttype: winsize ", twod{ r.Event.WindowBufferSizeEvent.dwSize.X, r.Event.WindowBufferSizeEvent.dwSize.Y }, '\n');
                        break;
                    case MENU_EVENT:
                        crop.add("\ttype: menu command:", r.Event.MenuEvent.dwCommandId, '\n');
                        break;
                    case FOCUS_EVENT:
                        crop.add("\ttype: focus ", r.Event.FocusEvent.bSetFocus ? "on" : "off", '\n');
                        break;
                }
            }
            log(crop);
        }
        template<bool Complete = faux, class Payload>
        auto readevents(Payload& packet, cdrw& answer)
        {
            if (!server.size_check(packet.echosz, answer.sendoffset())) return;
            auto avail = packet.echosz - answer.sendoffset();
            auto limit = avail / (ui32)sizeof(recbuf.front());
            if (server.io_log) log("\tuser limit: ", limit);
            auto head = buffer.begin();
            if (packet.input.utf16)
            {
                recbuf.clear();
                recbuf.reserve(count());
                auto tail = head + std::min(limit, count());
                while (head != tail)
                {
                    recbuf.emplace_back(*head++);
                }
            }
            else
            {
                auto splitter = [&](auto codec_proc)
                {
                    recbuf.reserve(recbuf.size() + count());
                    auto tail = buffer.end();
                    auto code = utfx{};
                    auto left = INPUT_RECORD{};
                    if (recbuf.size() && recbuf.size() <= limit)
                    {
                        ++head; // Drop the deferred record.
                    }
                    while (head != tail && recbuf.size() < limit)
                    {
                        auto& src = *head++;
                        if (src.EventType != KEY_EVENT) // Just copy non keybd events.
                        {
                            recbuf.emplace_back(src);
                        }
                        else // Expand and fill up to limit.
                        {
                            auto& next = src.Event.KeyEvent.uChar.UnicodeChar;
                            auto& copy = code ? left : src;
                            if (utf::tocode(next, code)) // BMP or the second part of surrogate pair.
                            {
                                toANSI.clear();
                                codec_proc(code, toANSI);
                                auto queue = qiew{ toANSI };
                                while (queue)
                                {
                                    auto& dst = recbuf.emplace_back(copy);
                                    dst.Event.KeyEvent.uChar.UnicodeChar = (byte)queue.pop_front();
                                }
                                if (recbuf.size() > limit) --head; // Keep the current record until the deferred buffer not empty.
                                code = {};
                            }
                            else // The first part of surrogate pair.
                            {
                                left = src;
                            }
                        }
                    }
                };
                auto& codec = *server.inpenc;
                if (codec.codepage == CP_UTF8) splitter([&](utfx& code, text& toUTF8){ utf::to_utf_from_code(code, toUTF8); });
                else                           splitter([&](utfx& code, text& toANSI){          codec.encode(code, toANSI); });
            }
            auto size = std::min(limit, (ui32)recbuf.size());
            auto peek = packet.input.flags & Payload::peek;
            packet.reply.count = size;
            if (!peek)
            {
                buffer.erase(buffer.begin(), head);
                if (buffer.empty()) ondata.flush();
            }
            if (size == recbuf.size())
            {
                if (server.io_log) logbuf(recbuf);
                answer.send_data<Complete>(server.condrv, recbuf);
                recbuf.clear();
            }
            else
            {
                if (server.io_log) logbuf(std::span{ recbuf.data(), size });
                answer.send_data<Complete>(server.condrv, std::span{ recbuf.data(), size });
                if (peek) recbuf.clear();
                else      recbuf.erase(recbuf.begin(), recbuf.begin() + size);
            }
        }
        template<class Payload>
        auto take(Payload& packet)
        {
            auto lock = std::lock_guard{ locker };
            if (buffer.empty())
            {
                if (server.io_log) log("\tevents buffer is empty");
                if (packet.input.flags & Payload::fast)
                {
                    if (server.io_log) log("\treply.count: 0");
                    packet.reply.count = 0;
                    return;
                }
                else
                {
                    auto token = jobs::token(server.answer, packet.target, faux);
                    worker.add(token, [&, packet](auto& token) mutable
                    {
                        auto lock = std::unique_lock{ locker };
                        auto& answer = std::get<0>(token);
                        auto& cancel = std::get<2>(token);
                        answer.buffer = &packet.reply; // Restore after copy. Payload start address.

                        if (closed || cancel) return;
                        while ((void)signal.wait(lock, [&]{ return buffer.size() || closed || cancel; }), !closed && !cancel && buffer.empty())
                        { }
                        if (closed || cancel) return;

                        readevents<true>(packet, answer);
                        if (server.io_log) log("\tdeferred task complete ", utf::to_hex_0x(packet.taskid));
                    });
                    server.answer = {};
                }
            }
            else
            {
                readevents(packet, server.answer);
            }
        }
    };

    struct decoder
    {
        using buff = std::array<byte, 256>;
        using vecW = std::array<wchr, 65536>;
        using vecA = std::vector<wchr>;
        ui32 codepage{ CP_UTF8 }; // decoder: .
        byte defchar1{}; // decoder: .
        byte defchar2{}; // decoder: .
        byte lastbyte{}; // decoder: .
        buff leadbyte{}; // decoder: .
        vecW BMPtoOEM{}; // decoder: .
        vecA OEMtoBMP{}; // decoder: .
        sz_t charsize{}; // decoder: .

        auto test(byte c)
        {
            return !!leadbyte[c];
        }
        auto load(consrv& server, ui32 cp)
        {
            if (cp == codepage) return true;
            leadbyte.fill(0);
            if (cp == CP_UTF8)
            {
                codepage = cp;
                charsize = {};
                defchar1 = {};
                defchar2 = {};
                return true;
            }
            auto data = CPINFO{};
            if (os::ok(::GetCPInfo(cp, &data), "::GetCPInfo()", os::unexpected_msg))
            {
                codepage = cp;
                charsize = data.MaxCharSize;
                defchar1 = data.DefaultChar[0];
                defchar2 = data.DefaultChar[1];
                auto head = std::begin(data.LeadByte);
                auto tail = std::end(data.LeadByte);
                while (head != tail)
                {
                    auto a = *head++;
                    if (!a) break;
                    auto b = *head++;
                    if (server.io_log) log("\tcodepage range: ", (int)a, "-", (int)b);
                    do leadbyte[a] = 1;
                    while (a++ != b);
                }

                // Forward table.
                if ((charsize == 1))
                {
                    OEMtoBMP.resize(256);
                    auto i = 0;
                    for (auto& c : OEMtoBMP) c = i++;
                }
                else
                {
                    OEMtoBMP.resize(65536, 0);
                    for (auto i = 0; i < 256; i++)
                    {
                        if (test((byte)i))
                        {
                            auto lead = i << 8;
                            auto head = OEMtoBMP.begin() + lead;
                            auto tail = head + 256;
                            *head++ = ++lead/*zero can't be a second byte*/;
                            while (head != tail) *head++ = lead++;
                        }
                        else OEMtoBMP[i] = i;
                    }
                }
                auto oem = std::vector<byte>{};
                oem.reserve(65536 * charsize);
                for (auto c : OEMtoBMP)
                {
                    if (c < 256)
                    {
                        oem.push_back((byte)c);
                    }
                    else
                    {
                        oem.push_back(c >> 8);
                        oem.push_back(c & 0xFF);
                    }
                }
                ::MultiByteToWideChar(codepage, 0, (char *)oem.data(), (si32)oem.size(), OEMtoBMP.data(), (si32)OEMtoBMP.size());
                if (server.io_log)
                {
                    auto t = "OEM to BMP lookup table:\n"s;
                    auto n = 0;
                    //auto u = oem.begin();
                    t += utf::to_hex<true>(n >> 4, 3) + "0: ";
                    for (auto c : OEMtoBMP)
                    {
                        t += ansi::ocx(20 + 6 + (n % 16) * 3);
                        //auto d = *u++;
                        //t += ansi::ocx(20 + 6 + (n % 16) * 8).add(utf::to_hex(d, 4)).add("-");
                        if (c < ' ' || c == 0x7f) t += ansi::fgc(blacklt).add(utf::to_hex<true>(c, 2)).nil();
                        else                      t += ansi::inv(true).scp().add("  ").rcp().add(utf::to_utf(c)).nil();
                        if (++n % 16 == 0)
                        {
                            t += "\n" + utf::to_hex<true>(n >> 4, 3) + "0: ";
                            if (n > 255 && charsize == 1) break;
                        }
                    }
                    log("-------------------------\n", t, "\n-------------------------");
                }

                // Reverse table.
                auto bmp = std::vector<wchr>(65536, 0);
                auto tmp = std::vector<byte>(65536 * 2, 0);
                auto i = 0;
                for (auto& b : bmp) b = i++;
                ::WideCharToMultiByte(codepage, 0, bmp.data(), (si32)bmp.size(), (char *)tmp.data(), (si32)tmp.size(), 0, 0);
                auto ptr = tmp.begin();
                for (auto& d : BMPtoOEM)
                {
                    auto c = *ptr++;
                    if (test(c)) d = ((ui16)c << 8) + *ptr++;
                    else         d = c;
                }
                if constexpr (debugmode && faux)
                {
                    auto t = "BMP to OEM lookup table:\n"s;
                    auto n = 0;
                    t += utf::to_hex<true>(n >> 4, 3) + "0: ";
                    for (auto c : BMPtoOEM)
                    {
                        c = OEMtoBMP[c];
                        t += ansi::ocx(20 + 6 + (n % 16) * 3);
                        if (c < ' ' || c == 0x7f) t += ansi::fgc(blacklt).add(utf::to_hex<true>(c, 2)).nil();
                        else                      t += ansi::inv(true).scp().add("  ").rcp().add(utf::to_utf(c)).nil();
                        if (++n % 16 == 0)
                        {
                            t += "\n" + utf::to_hex<true>(n >> 4, 3) + "0: ";
                        }
                    }
                    log("-------------------------\n", t, "\n-------------------------");
                }

                return true;
            }
            else return faux;
        }
        auto defchar()
        {
            return charsize == 1 ? (utfx)defchar1 : ((utfx)defchar1 << 8) + defchar2;
        }
        auto decode_char(utfx code)
        {
            static constexpr auto c0_wchr = { L'\0',L'☺', L'☻', L'♥', L'♦', L'♣', L'♠', L'•', L'◘', L'○', L'◙', L'♂', L'♀', L'♪', L'♫', L'☼',
                                              L'►', L'◄', L'↕', L'‼', L'¶', L'§', L'▬', L'↨', L'↑', L'↓', L'→', L'←', L'∟', L'↔', L'▲', L'▼',
                                              L'⌂' };
                 if (code < 0x20 || code == 0x7F) code = *(c0_wchr.begin() + std::min<size_t>(code, c0_wchr.size() - 1));
            else if (code < OEMtoBMP.size())      code = OEMtoBMP[code];
            else                                  code = defchar();
            return code;
        }
        auto decode_char(byte lead, byte next)
        {
            auto code = ((ui16)lead << 8) + next;
            return decode_char(code);
        }
        auto decode(utfx code)
        {
            if (code < OEMtoBMP.size()) code = OEMtoBMP[code];
            else                        code = defchar();
            return code;
        }
        auto decode(byte lead, byte next)
        {
            auto code = ((ui16)lead << 8) + next;
            return decode(code);
        }
        auto decode(utfx code, text& toUTF8)
        {
            static constexpr auto c0_view = { " "sv, "☺"sv, "☻"sv, "♥"sv, "♦"sv, "♣"sv, "♠"sv, "•"sv, "◘"sv, "○"sv, "◙"sv, "♂"sv, "♀"sv, "♪"sv, "♫"sv, "☼"sv,
                                              "►"sv, "◄"sv, "↕"sv, "‼"sv, "¶"sv, "§"sv, "▬"sv, "↨"sv, "↑"sv, "↓"sv, "→"sv, "←"sv, "∟"sv, "↔"sv, "▲"sv, "▼"sv,
                                              "⌂"sv };
            if (code < 0x20 || code == 0x7F)
            {
                toUTF8 += *(c0_view.begin() + std::min<size_t>(code, c0_view.size() - 1));
            }
            else
            {
                if (codepage != CP_UTF8)
                {
                    code = decode(code);
                }
                utf::to_utf_from_code(code, toUTF8);
            }
        }
        auto decode(wide& toWIDE, text& toUTF8)
        {
            auto last = toWIDE.back();
            if (last >= 0xd800 && last <= 0xdbff) // First part of surrogate pair.
            {
                toWIDE.pop_back();
                utf::to_utf(toWIDE, toUTF8);
                toWIDE.clear();
                toWIDE.push_back(last);
            }
            else
            {
                utf::to_utf(toWIDE, toUTF8);
                toWIDE.clear();
            }
        }
        auto decode_run(auto& toANSI, text& toUTF8)
        {
            auto head = toANSI.begin();
            auto tail = toANSI.end();
            while (head != tail)
            {
                auto c = (ui16)(byte)*head++;
                if (test((byte)c))
                {
                    c = head != tail ? (c << 8) + (byte)*head++ : defchar();
                }
                auto bmp = OEMtoBMP[c];
                utf::to_utf(bmp, toUTF8);
            }
        }
        auto decode_log(view toANSI)
        {
            auto utf8 = text{};
            decode_run(toANSI, utf8);
            return utf8;
        }
        auto decode(text& toANSI, text& toUTF8)
        {
            auto last = toANSI.back();
            auto hang = test(last);
            if (hang)
            {
                toANSI.pop_back();
                if (toANSI.size() && test(toANSI.back())) // Another hanging lead byte. Fix it.
                {
                    toANSI.back() = defchar1;
                    if (test(defchar1)) toANSI.push_back(defchar2);
                }
            }
            decode_run(toANSI, toUTF8);
            toANSI.clear();
            if (hang) toANSI.push_back(last);
        }
        auto encode(wchr code)
        {
            return BMPtoOEM[code];
        }
        void encode(wchr code, text& ansi)
        {
            if (code >= 65536) code = defchar();
            else               code = BMPtoOEM[code];
            if (code < 256)
            {
                ansi.push_back((byte)code);
            }
            else
            {
                ansi.push_back(code >> 8);
                ansi.push_back(code & 0xFF);
            }
        }
        void reset()
        {
            lastbyte = 0;
        }
        void encode(view& utf8, text& crop, ui32 readstep = ui32max)
        {
            auto done = size_t{};
            auto rest = readstep;
            if (auto iter = utf::cpit{ utf8 })
            {
                if (lastbyte)
                {
                    auto next = iter.next();
                    done += next.utf8len;
                    crop.push_back(lastbyte);
                    rest--;
                    lastbyte = 0;
                }
                while (iter && rest)
                {
                    auto next = iter.next();
                    auto code = next.correct
                             && next.cdpoint < 65536 ? BMPtoOEM[next.cdpoint]
                                                     : defchar();
                    auto size = code < 256 ? 1u : 2u;
                    if (rest < size) // Leave the last code point to indicate that the buffer is not empty.
                    {
                        crop.push_back(code >> 8);
                        lastbyte = code & 0xFF;
                        rest -= 1;
                        assert(rest == 0);
                    }
                    else
                    {
                        if (code < 256)
                        {
                            crop.push_back((byte)code);
                        }
                        else
                        {
                            crop.push_back(code >> 8);
                            crop.push_back(code & 0xFF);
                        }
                        rest -= size;
                        done += next.utf8len;
                    }
                }
            }
            utf8.remove_prefix(done);
        }
        auto encode(view& utf8, ui32 readstep = ui32max)
        {
            auto crop = text{};
            encode(utf8, crop, readstep);
            return crop;
        }

        decoder() = default;
        decoder(consrv& server, ui32 cp)
        {
            load(server, cp);
        }
    };

    //enum type : ui32 { undefined, trueUTF_8, realUTF16, attribute, fakeUTF16 };
    enum type : ui32 { undefined, ansiOEM, realUTF16, attribute, fakeUTF16 };

    #define log(...) if (io_log) log(__VA_ARGS__)

    auto& langmap()
    {
        static const auto langmap = std::unordered_map<ui32, ui16>
        {
            { 932,   MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT            ) },
            { 936,   MAKELANGID(LANG_CHINESE , SUBLANG_CHINESE_SIMPLIFIED ) },
            { 949,   MAKELANGID(LANG_KOREAN  , SUBLANG_KOREAN             ) },
            { 950,   MAKELANGID(LANG_CHINESE , SUBLANG_CHINESE_TRADITIONAL) },
            { 65001, MAKELANGID(LANG_ENGLISH , SUBLANG_ENGLISH_US         ) },
        };
        return langmap;
    }

    void set_cp(ui32 c)
    {
        auto lock = std::lock_guard{ events.locker };
        auto& o = outenc;
        auto& i = inpenc;
        if (o->codepage != c)
        {
            if (i->codepage == c) o = i; // Reuse existing decoder.
            else
            {
                if (auto p = ptr::shared<decoder>(); p->load(*this, c)) o = p;
            }
        }
        i = o;
        for (auto& client : joined) // Reset trailing/hanging bytes.
        for (auto& handle : client.tokens)
        {
            handle.toWIDE.clear();
            handle.toANSI.clear();
            handle.toUTF8.clear();
        }
        inpenc->reset();
    }
    auto attr_to_brush(ui16 attr)
    {
        auto c = cell{ whitespace };
        if constexpr (isreal())
        {
            auto& colors = uiterm.ctrack.color;
            c.fgc(colors[netxs::swap_bits<0, 2>(attr      & 0x000Fu)]) // FOREGROUND_ . . .
             .bgc(colors[netxs::swap_bits<0, 2>(attr >> 4 & 0x000Fu)]) // BACKGROUND_ . . .
             .inv(!!(attr & COMMON_LVB_REVERSE_VIDEO  ))
             .und(!!(attr & COMMON_LVB_UNDERSCORE     ))
             .ovr(!!(attr & COMMON_LVB_GRID_HORIZONTAL));
        }
        return c;
    }
    auto brush_to_attr(cell const& brush)
    {
        auto attr = ui16{};
        auto fgcx = 7_sz; // Fallback for true colors.
        auto bgcx = 0_sz;
        if constexpr (isreal())
        {
            auto frgb = brush.fgc().token;
            auto brgb = brush.bgc().token;
            auto head = std::begin(uiterm.ctrack.color);
            for (auto i = 0; i < 16; i++)
            {
                auto const& c = *head++;
                auto m = netxs::swap_bits<0, 2>(i); // ANSI<->DOS color scheme reindex.
                if (c == frgb) fgcx = m;
                if (c == brgb) bgcx = m;
            }
        }
        attr = static_cast<ui16>(fgcx + (bgcx << 4));
        if (brush.inv()) attr |= COMMON_LVB_REVERSE_VIDEO;
        if (brush.und()) attr |= COMMON_LVB_UNDERSCORE;
        if (brush.ovr()) attr |= COMMON_LVB_GRID_HORIZONTAL;
        return attr;
    }
    auto set_half(auto wdt, auto& attr)
    {
        if (wdt >= 2)
        {
            attr |= (wdt - 1) << 8; // Apply COMMON_LVB_LEADING_BYTE / COMMON_LVB_TRAILING_BYTE.
        }
    }
    template<class RecType, feed Input, class T>
    auto take_buffer(T&& packet)
    {
        auto offset = Input == feed::fwd ? answer.readoffset()
                                         : answer.sendoffset();
        auto length = Input == feed::fwd ? packet.packsz
                                         : packet.echosz;
        auto count = 0_sz;
        if (auto datasize = size_check(length,  offset))
        {
            assert(datasize % sizeof(RecType) == 0);
            buffer.resize(datasize);
            if (answer.recv_data(condrv, buffer))
            {
                count = buffer.size() / sizeof(RecType);
            }
        }
        return wrap<RecType>::cast(buffer, count);
    }
    template<class T, class P>
    auto direct(T* handle_ptr, P proc)
    {
        if (handle_ptr)
        {
            if (handle_ptr->link == &uiterm.target)
            {
                     if (uiterm.target == &uiterm.normal) uiterm.update([&] { proc(uiterm.normal); });
                else if (uiterm.target == &uiterm.altbuf) uiterm.update([&] { proc(uiterm.altbuf); });
                return true;
            }
            else
            {
                auto client_ptr = &handle_ptr->boss;
                if (auto iter = std::find_if(joined.begin(), joined.end(), [&](auto& client){ return client_ptr == &client; });
                    iter != joined.end()) // Client exists.
                {
                    auto& client = handle_ptr->boss;
                    if (auto iter = std::find_if(client.tokens.begin(), client.tokens.end(), [&](auto& token){ return handle_ptr == &token; });
                        iter != client.tokens.end()) // Handle allocated.
                    {
                        auto link = handle_ptr->link;
                        if (auto iter = std::find_if(client.alters.begin(), client.alters.end(), [&](auto& altbuf){ return link == &altbuf; });
                            iter != client.alters.end()) // Buffer exists.
                        {
                            uiterm.update([&] { proc(*iter); });
                            return true;
                        }
                    }
                }
            }
        }
        log("\tabort: invalid handle ", utf::to_hex_0x(handle_ptr));
        answer.status = nt::status::invalid_handle;
        return faux;
    }
    template<class T>
    auto select_buffer(T* handle_ptr)
    {
        using base_ptr = decltype(uiterm.target);
        auto result = base_ptr{};
        if (handle_ptr)
        {
            if (handle_ptr->link == &uiterm.target) result = uiterm.target;
            else
            {
                auto client_ptr = &handle_ptr->boss;
                if (auto iter = std::find_if(joined.begin(), joined.end(), [&](auto& client){ return client_ptr == &client; });
                    iter != joined.end()) // Client exists.
                {
                    auto& client = handle_ptr->boss;
                    if (auto iter = std::find_if(client.tokens.begin(), client.tokens.end(), [&](auto& token){ return handle_ptr == &token; });
                        iter != client.tokens.end()) // Handle allocated.
                    {
                        auto link = handle_ptr->link;
                        if (auto iter = std::find_if(client.alters.begin(), client.alters.end(), [&](auto& altbuf){ return link == &altbuf; });
                            iter != client.alters.end()) // Buffer exists.
                        {
                            result = &(*iter);
                        }
                    }
                }
            }
        }
        if (!result)
        {
            log("\tinvalid handle ", utf::to_hex_0x(handle_ptr));
        }
        return result;
    }
    void check_buffer_size(auto& console, auto& size)
    {
        if (size.x > 165)
        {
            // Just disable wrapping if user application requests too much (Explicit requirement for horizontal scrolling).
            // E.g. wmic requests { x=1500, y=300 }.
            //      Indep stat for dwSize.X = N: max: 1280, 10000, 192, 237, 200, 2500, 500, 600, 640.
            //                                   min: 150, 100, 132, 120, 150, 140, 165, 30, 80.
            log("\ttoo wide buffer requested, turning off wrapping");
            console.style.wrp(faux);
            size.x = console.panel.x;
        }
        if (size.y > 99)
        {
             // Applications usually request real viewport heights: 20, 24, 25, 50
             //         or extra large values for the scrollbuffer: 0x7FFF, 5555, 9000, 9999, 4096, 32767, 32000, 10000, 2500, 2000, 1024, 999, 800, 512, 500, 480, 400, 300, 100 etc. (stat for dwSize.Y = N)
            log("\ttoo long buffer requested, updating scrollback limits");
            uiterm.sb_min(size.y);
            size.y = console.panel.y;
        }
    }
    auto show_page(auto utf16, auto codepage)
    {
        return        utf16 ? "UTF-16"s :
        codepage == CP_UTF8 ? "UTF-8"s  :
                              "OEM-"s + std::to_string(codepage);
    }
    template<class S, class P>
    auto write_block(S& scrollback, core const& block, twod coor, rect clip, P fuse)
    {
        auto size = block.size();
        auto view = block.view();
        auto dest = rect{ coor, view.size };
        clip = dest.clip(clip);
        view -= dest - clip;
        coor = clip.coor;
        auto head = block.iter() + view.coor.y * size.x;
        auto tail = head + view.size.y * size.x;
        auto rest = size.x - (view.coor.x + view.size.x);
        auto save = scrollback.coord;
        assert(rest >= 0);
        while (head != tail)
        {
            head += view.coor.x;
            auto next = head + view.size.x;
            auto line = std::span(head, next);
            scrollback.cup0(coor);
            scrollback._data<true>(view.size.x, line, fuse);
            head = next + rest;
            coor.y++;
        }
        scrollback.cup0(save);
    }
    auto newbuf(auto& client) // MSVC bug; It doesn't see constexpr inside lambdas (even constexpr functions).
    {
        if constexpr (isreal())
        {
            auto& console = client.alters.emplace_back(uiterm);
            console.resize_viewport(uiterm.target->panel);
            console.style = uiterm.target->style;
            return &console;
        }
        else
        {
            auto& console = client.alters.emplace_back();
            return &console;
        }
    }

    auto api_unsupported                     ()
    {
        log(prompt, "Unsupported consrv request code ", upload.fxtype);
        answer.status = nt::status::illegal_function;
    }
    auto api_system_langid_get               ()
    {
        log(prompt, "GetConsoleLangId");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui16 langid;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto winuicp = ::GetACP();
        if (winuicp != 65001 && langmap().contains(winuicp))
        {
            packet.reply.langid = netxs::map_or(langmap(), outenc->codepage, 65001);
            log("\tlang id: ", utf::to_hex_0x(packet.reply.langid));
        }
        else
        {
            answer.status = nt::status::not_supported;
            log("\tlang id not supported");
        }
    }
    auto api_system_mouse_buttons_get_count  ()
    {
        log(prompt, "GetNumberOfConsoleMouseButtons");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.count = ::GetSystemMetrics(SM_CMOUSEBUTTONS);
        log("\treply.count: ", packet.reply.count);
    }
    auto api_process_attach                  ()
    {
        log(prompt, "Attach process to console");
        struct payload : wrap<payload>
        {
            ui64 taskid;
            ui32 procid;
            ui32 _pad_1;
            ui32 thread;
            ui32 _pad_2;
        };
        auto& packet = payload::cast(upload);
        struct exec_info : wrap<exec_info>
        {
            ui32 win_icon_id;
            ui32 used_hotkey;
            ui32 start_flags;
            ui16 fill_colors;
            ui16 window_mode;
            si16 scroll_sz_x, scroll_sz_y;
            si16 window_sz_x, window_sz_y;
            si16 window_at_x, window_at_y;
            ui32 app_groupid;
            byte climode_app;
            byte win_visible;
            ui16 header_size;
            wchr header_data[261];
            ui16 curexe_size;
            wchr curexe_data[128];
            ui16 curdir_size;
            wchr curdir_data[261];
        };
        auto& details = exec_info::cast(buffer);
        if (!answer.recv_data(condrv, buffer)) return;

        details.curexe_size = std::min<ui16>(details.curexe_size, sizeof(details.curexe_data));
        details.header_size = std::min<ui16>(details.header_size, sizeof(details.header_data));
        details.curdir_size = std::min<ui16>(details.curdir_size, sizeof(details.curdir_data));

        auto iter = std::find_if(joined.begin(), joined.end(), [&](auto& client) { return client.procid == packet.procid; });
        auto& client = iter != joined.end() ? *iter
                                            : joined.emplace_front();
        auto& inphndl = client.tokens.emplace_front(client, inpmod, hndl::type::events, &events);
        auto& outhndl = client.tokens.emplace_front(client, outmod, hndl::type::scroll, &uiterm.target);
        client.procid = packet.procid;
        client.thread = packet.thread;
        client.pgroup = details.app_groupid;
        if constexpr (isreal())
        {
            client.backup = uiterm.target->brush;
        }
        client.detail.iconid = details.win_icon_id;
        client.detail.hotkey = details.used_hotkey;
        client.detail.config = details.start_flags;
        client.detail.colors = details.fill_colors;
        client.detail.format = details.window_mode;
        client.detail.cliapp = details.climode_app;
        client.detail.expose = details.win_visible;
        client.detail.scroll = twod{ details.scroll_sz_x, details.scroll_sz_y };
        client.detail.window = rect{{ details.window_at_x, details.window_at_y }, { details.window_sz_x, details.window_sz_y }};
        client.detail.header = utf::to_utf(details.header_data, details.header_size / sizeof(wchr));
        client.detail.curexe = utf::to_utf(details.curexe_data, details.curexe_size / sizeof(wchr));
        client.detail.curdir = utf::to_utf(details.curdir_data, details.curdir_size / sizeof(wchr));
        log("\tprocid: ", client.procid, "\n",
            "\tthread: ", client.thread, "\n",
            "\tpgroup: ", client.pgroup, "\n",
            "\ticonid: ", client.detail.iconid, "\n",
            "\thotkey: ", client.detail.hotkey, "\n",
            "\tconfig: ", client.detail.config, "\n",
            "\tcolors: ", client.detail.colors, "\n",
            "\tformat: ", client.detail.format, "\n",
            "\tscroll: ", client.detail.scroll, "\n",
            "\tcliapp: ", client.detail.cliapp, "\n",
            "\texpose: ", client.detail.expose, "\n",
            "\twindow: ", client.detail.window, "\n",
            "\theader: ", client.detail.header, "\n",
            "\tapname: ", client.detail.curexe, "\n",
            "\tcurdir: ", client.detail.curdir);

        struct connect_info : wrap<connect_info>
        {
            clnt* client_id;
            hndl* events_id;
            hndl* scroll_id;
        };
        auto& info = connect_info::cast(buffer);
        info.client_id = &client;
        info.events_id = &inphndl;
        info.scroll_id = &outhndl;

        answer.buffer = &info;
        answer.length = sizeof(info);
        answer.report = sizeof(info);
        //auto rc = nt::ioctl(nt::console::op::complete_io, condrv, answer);
        //answer = {};
    }
    auto api_process_detach                  ()
    {
        struct payload : drvpacket<payload>
        { };
        auto& packet = payload::cast(upload);
        auto client_ptr = packet.client;
        log(prompt, "Detach process from console: ", utf::to_hex_0x(client_ptr));
        auto iter = std::find_if(joined.begin(), joined.end(), [&](auto& client){ return client_ptr == &client; });
        if (iter != joined.end())
        {
            auto& client = *client_ptr;
            log("\tproc id: ", client.procid);
            for (auto& handle : client.tokens)
            {
                auto handle_ptr = &handle;
                log("\tdeactivate handle: ", utf::to_hex_0x(handle_ptr));
                events.abort(handle_ptr);
            }
            if constexpr (isreal())
            {
                uiterm.target->brush = client.backup;
                for (auto& a : client.alters) // Switch from client's scrollback buffer if it is active.
                {
                    if (uiterm.target == &a)
                    {
                        uiterm.reset_to_normal(a);
                        break;
                    }
                }
            }
            client.tokens.clear();
            joined.erase(iter);
            log("\tprocess ", utf::to_hex_0x(client_ptr), " detached");
        }
        else log("\trequested process ", utf::to_hex_0x(client_ptr), " not found");
    }
    auto api_process_enlist                  ()
    {
        log(prompt, "GetConsoleProcessList");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto avail = size_check(packet.echosz, answer.sendoffset());
        if (!avail)
        {
            return;
        }
        auto count = avail / sizeof(ui32);
        auto recs = wrap<ui32>::cast(buffer, count);
        packet.reply.count = static_cast<ui32>(joined.size());
        log("\treply.count: ", packet.reply.count);
        if (count >= joined.size())
        {
            auto dest = recs.begin();
            for (auto& client : joined)
            {
                *dest++ = client.procid;
                log("\tpid: ", client.procid);
            }
            answer.send_data(condrv, recs);
        }
    }
    auto api_process_create_handle           ()
    {
        log(prompt, "Create console handle");
        enum type : ui32
        {
            undefined,
            dupevents,
            dupscroll,
            newscroll,
            seerights,
        };
        GENERIC_READ | GENERIC_WRITE;
        struct payload : base, wrap<payload>
        {
            struct
            {
                type action;
                ui32 shared; // Unused
                ui32 rights; // GENERIC_READ | GENERIC_WRITE
                ui32 _pad_1;
                ui32 _pad_2;
                ui32 _pad_3;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto& client = *packet.client;
        log("\tclient procid: ", client.procid);
        auto create = [&](auto type, auto msg)
        {
            auto& h = type == hndl::type::events ? client.tokens.emplace_back(client, inpmod, hndl::type::events, &uiterm)
                    : type == hndl::type::scroll ? client.tokens.emplace_back(client, outmod, hndl::type::scroll, &uiterm.target)
                                                 : client.tokens.emplace_back(client, outmod, hndl::type::altbuf, newbuf(client));
            answer.report = reinterpret_cast<ui64>(&h);
            log(msg, &h);
        };
        switch (packet.input.action)
        {
            case type::dupevents: create(hndl::type::events, "\tdup events handle "); break;
            case type::dupscroll: create(hndl::type::scroll, "\tdup scroll handle "); break;
            case type::newscroll: create(hndl::type::altbuf, "\tnew altbuf handle "); break;
            default: if (packet.input.rights & GENERIC_READ) create(hndl::type::events, "\tdup (GENERIC_READ) events handle ");
                     else                                    create(hndl::type::scroll, "\tdup (GENERIC_WRITE) scroll handle ");
        }
    }
    auto api_process_delete_handle           ()
    {
        struct payload : drvpacket<payload>
        { };
        auto& packet = payload::cast(upload);
        log(prompt, "Delete console handle");
        auto handle_ptr = packet.target;
        if (handle_ptr == nullptr)
        {
            log("\tabort: handle_ptr = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto client_ptr = &handle_ptr->boss;
        auto client_iter = std::find_if(joined.begin(), joined.end(), [&](auto& client){ return client_ptr == &client; });
        if (client_iter == joined.end())
        {
            log("\tbad handle: ", utf::to_hex_0x(handle_ptr));
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& client = handle_ptr->boss;
        auto iter = std::find_if(client.tokens.begin(), client.tokens.end(), [&](auto& token){ return handle_ptr == &token; });
        if (iter != client.tokens.end())
        {
            auto a = handle_ptr->link;
            if constexpr (isreal())
            {
                if (uiterm.target == a)
                {
                    uiterm.reset_to_normal(*uiterm.target);
                }
            }
            log("\tdeactivate handle: ", utf::to_hex_0x(handle_ptr));
            events.abort(handle_ptr);
            client.tokens.erase(iter);
        }
        else log("\trequested handle ", utf::to_hex_0x(handle_ptr), " not found");
    }
    auto api_process_codepage_get            ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 code_page;
            }
            reply;
            struct
            {
                byte is_output;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log(prompt, packet.input.is_output ? "GetConsoleOutputCP"
                                           : "GetConsoleCP");
        packet.reply.code_page = packet.input.is_output ? outenc->codepage
                                                        : inpenc->codepage;
        log("\treply.code_page: ", packet.reply.code_page);
    }
    auto api_process_codepage_set            ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 code_page;
                byte is_output;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto c = packet.input.code_page;
        auto is_output = !!packet.input.is_output;
        log(prompt, is_output ? "SetConsoleOutputCP"
                              : "SetConsoleCP", "\n\tinput.code_page ", c);
        auto& o = is_output ? outenc : inpenc;
        auto& i = is_output ? inpenc : outenc;
        if (o->codepage != c)
        {
            if (i->codepage == c) o = i; // Reuse existing decoder.
            else
            {
                if (auto p = ptr::shared<decoder>(); p->load(*this, c)) o = p;
                else answer.status = nt::status::not_supported;
            }
        }
        for (auto& client : joined) // Reset trailing/hanging bytes.
        for (auto& handle : client.tokens)
        if (is_output == (handle.kind != hndl::type::events))
        {
            handle.toWIDE.clear();
            handle.toANSI.clear();
            handle.toUTF8.clear();
        }
        if (!is_output) inpenc->reset();
    }
    auto api_process_mode_get                ()
    {
        log(prompt, "GetConsoleMode");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 mode;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto handle_ptr = packet.target;
        if (handle_ptr == nullptr)
        {
            log("\tabort: handle_ptr = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& handle = *handle_ptr;
        packet.reply.mode = handle.mode;
        log("\treply.mode: ", handle);
    }
    auto api_process_mode_set                ()
    {
        log(prompt, "SetConsoleMode");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 mode;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto handle_ptr = packet.target;
        if (handle_ptr == nullptr)
        {
            log("\tabort: handle_ptr = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& handle = *handle_ptr;
        if (handle.kind != hndl::type::events)
        {
            auto cur_mode = handle.mode       & nt::console::outmode::no_auto_cr;
            auto new_mode = packet.input.mode & nt::console::outmode::no_auto_cr;
            if (cur_mode != new_mode)
            {
                auto autocr = !new_mode;
                if constexpr (isreal())
                {
                    uiterm.normal.set_autocr(autocr);
                }
                log("\tauto_crlf: ", autocr ? "enabled" : "disabled");
            }
        }
        else if (handle.kind == hndl::type::events)
        {
            auto mouse_mode = packet.input.mode & nt::console::inmode::mouse;
            if constexpr (isreal())
            {
                if (mouse_mode)
                {
                    uiterm.mtrack.enable (decltype(uiterm.mtrack)::negative_args);
                    uiterm.mtrack.setmode(decltype(uiterm.mtrack)::w32);
                }
                else
                {
                    uiterm.mtrack.disable(decltype(uiterm.mtrack)::negative_args);
                }
            }
            log("\tmouse_input: ", mouse_mode ? "enabled" : "disabled");
        }
        handle.mode = packet.input.mode;
        log("\tinput.mode: ", handle);
    }
    template<bool RawRead = faux>
    auto api_events_read_as_text             ()
    {
        log(prompt, "ReadConsole");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte utf16;
                byte EOFon;
                ui16 exesz;
                ui32 affix;
                ui32 stops;
            }
            input;
            struct
            {
                ui32 ctrls;
                ui32 bytes;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;
        if constexpr (RawRead)
        {
            log("\tread mode: raw ReadFile emulation");
            packet.input = { .EOFon = 1 };
        }
        auto namesize = static_cast<ui32>(packet.input.exesz * sizeof(wchr));
        if (!size_check(packet.echosz,  packet.input.affix)
         || !size_check(packet.echosz,  answer.sendoffset())) return;
        auto readstep = packet.echosz - answer.sendoffset();
        auto datasize = namesize + packet.input.affix;
        buffer.resize(datasize);
        if (!answer.recv_data(condrv, buffer)) return;

        auto nameview = wiew(reinterpret_cast<wchr*>(buffer.data()), packet.input.exesz);
        auto initdata = view(buffer.data() + namesize, packet.input.affix);
        if (!packet.input.utf16 && inpenc->codepage != CP_UTF8)
        {
            toUTF8.clear();
            inpenc->decode_run(initdata, toUTF8);
            initdata = toUTF8;
        }
        log("\t", show_page(packet.input.utf16, inpenc->codepage),
            "\n\tnamesize: ", namesize,
            "\n\tnameview: ", utf::debase(utf::to_utf(nameview)),
            "\n\treadstep: ", readstep,
            "\n\treadstop: ", utf::to_hex_0x(packet.input.stops),
            "\n\tinitdata: ", ansi::hi(packet.input.utf16 ? utf::debase<faux, faux>(utf::to_utf(wiew((wchr*)initdata.data(), initdata.size() / 2)))
                            : inpenc->codepage == CP_UTF8 ? utf::debase<faux, faux>(initdata)
                                                          : utf::debase<faux, faux>(toUTF8)));
        events.placeorder(packet, nameview, initdata, readstep);
    }
    auto api_events_clear                    ()
    {
        log(prompt, "FlushConsoleInputBuffer");
        events.clear();
    }
    auto api_events_count_get                ()
    {
        log(prompt, "GetNumberOfConsoleInputEvents");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto handle_ptr = packet.target;
        if (handle_ptr == nullptr)
        {
            log("\tabort: handle_ptr = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& handle = *handle_ptr;
        if (handle.kind != hndl::type::events) // GH#305: Python attempts to get the input event number using the hndl::type::scroll handle to determine the handle type.
        {
            log("\tabort: invalid handle type: ", handle.kind);
            answer.status = nt::status::invalid_handle;
            return;
        }
        packet.reply.count = events.count();
        log("\treply.count: ", packet.reply.count);
    }
    auto api_events_get                      ()
    {
        struct payload : drvpacket<payload>
        {
            enum
            {
                take,
                peek,
                fast,
            };
            struct
            {
                ui32 count;
            }
            reply;
            struct
            {
                ui16 flags;
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log(prompt, packet.input.flags & payload::peek ? "PeekConsoleInput"
                                                       : "ReadConsoleInput",
            "\n\tinput.flags: ", utf::to_hex_0x(packet.input.flags),
            "\n\t", show_page(packet.input.utf16, inpenc->codepage));
        auto client_ptr = packet.client;
        if (client_ptr == nullptr)
        {
            log("\tabort: packet.client = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& client = packet.client; //todo validate client
        auto events_handle_ptr = packet.target;
        if (events_handle_ptr == nullptr)
        {
            log("\tabort: events_handle_ptr = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& events_handle = *events_handle_ptr; //todo validate events_handle
        events.take(packet);
    }
    auto api_events_add                      ()
    {
        log(prompt, "WriteConsoleInput");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
            struct
            {
                byte utf16;
                byte totop; // Not used.
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto recs = take_buffer<INPUT_RECORD, feed::fwd>(packet);
        if (recs.size()) events.sendevents(recs, packet.input.utf16);
        packet.reply.count = static_cast<ui32>(recs.size());
        log("\twritten events count: ", packet.reply.count,
            "\n\t", show_page(packet.input.utf16, inpenc->codepage));
    }
    auto api_events_generate_ctrl_event      ()
    {
        log(prompt, "GenerateConsoleCtrlEvent");
        enum type : ui32
        {
            ctrl_c      = CTRL_C_EVENT,
            ctrl_break  = CTRL_BREAK_EVENT,
            close       = CTRL_CLOSE_EVENT,
            logoff      = CTRL_LOGOFF_EVENT,
            shutdown    = CTRL_SHUTDOWN_EVENT,
        };
        struct payload : drvpacket<payload>
        {
            struct
            {
                type event;
                ui32 group;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        events.alert(packet.input.event, packet.input.group);
    }
    template<bool RawWrite = faux>
    auto api_scrollback_write_text           ()
    {
        log(prompt, "WriteConsole");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.count = 0;

        if constexpr (RawWrite)
        {
            log("\traw write emulation");
            packet.input = {};
        }

        auto client_ptr = packet.client;
        if (client_ptr == nullptr)
        {
            log("\tabort: packet.client = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& client = *client_ptr;

        auto scroll_handle_ptr = packet.target;
        if (scroll_handle_ptr == nullptr)
        {
            log("\tabort: packet.target = invalid_value (0)");
            answer.status = nt::status::invalid_handle;
            return;
        }
        auto& scroll_handle = *scroll_handle_ptr;

        if (auto datasize = size_check(packet.packsz,  answer.readoffset()))
        {
            auto& codec = *outenc;
            if (packet.input.utf16)
            {
                auto initsize = scroll_handle.toWIDE.size();
                auto widesize = datasize / sizeof(wchr);
                scroll_handle.toWIDE.resize(initsize + widesize);
                if (!answer.recv_data(condrv, wiew{ scroll_handle.toWIDE.data() + initsize, widesize })) return;
                codec.decode(scroll_handle.toWIDE, scroll_handle.toUTF8);
            }
            else if (codec.codepage == CP_UTF8)
            {
                auto initsize = scroll_handle.toUTF8.size();
                scroll_handle.toUTF8.resize(initsize + datasize);
                if (!answer.recv_data(condrv, view{ scroll_handle.toUTF8.data() + initsize, datasize })) return;
            }
            else
            {
                auto initsize = scroll_handle.toANSI.size();
                scroll_handle.toANSI.resize(initsize + datasize);
                if (!answer.recv_data(condrv, view{ scroll_handle.toANSI.data() + initsize, datasize })) return;
                codec.decode(scroll_handle.toANSI, scroll_handle.toUTF8);
            }

            if (auto crop = ansi::purify(scroll_handle.toUTF8))
            {
                if constexpr (isreal())
                {
                    auto active = scroll_handle.link == &uiterm.target; // Target buffer can be changed during vt execution (eg: switch to altbuf).
                    if (!direct(scroll_handle_ptr, [&](auto& scrollback) { active ? uiterm.ondata(crop)
                                                                                  : uiterm.ondata(crop, &scrollback); }))
                    {
                        datasize = 0;
                    }
                }
                else
                {
                    uiterm.ondata(crop);
                }
                log("\t", show_page(packet.input.utf16, codec.codepage),
                    ": ", ansi::hi(utf::debase<faux, faux>(crop)));
                scroll_handle.toUTF8.erase(0, crop.size()); // Delete processed data.
            }
            else
            {
                log("\trest: ", ansi::hi(utf::debase<faux, faux>(scroll_handle.toUTF8)),
                  "\n\tsize: ", scroll_handle.toUTF8.size());
            }
            packet.reply.count = datasize;
        }
        else log("\tunexpected: packet.packsz=", packet.packsz, " answer.readoffset=", answer.readoffset());
        answer.report = packet.reply.count;
    }
    auto api_scrollback_write_data           ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 coorx, coory;
                type etype;
            }
            input;
            struct
            {
                ui32 count;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        if constexpr (isreal())
        {
            auto& screen = *uiterm.target;
            auto coord = std::clamp(twod{ packet.input.coorx, packet.input.coory }, dot_00, screen.panel - dot_11);
            auto maxsz = static_cast<ui32>(screen.panel.x * (screen.panel.y - coord.y) - coord.x);
            auto saved = screen.coord;
            auto count = ui32{};
            screen.cup0(coord);
            if (packet.input.etype == type::attribute)
            {
                auto recs = take_buffer<ui16, feed::fwd>(packet);
                count = static_cast<ui32>(recs.size());
                if (count > maxsz) count = maxsz;
                log(prompt, "WriteConsoleOutputAttribute",
                            "\n\tinput.coord: ", coord,
                            "\n\tinput.count: ", count);
                filler.size(count, cell{});
                auto iter = filler.iter();
                for (auto& attr : recs)
                {
                    *iter++ = attr_to_brush(attr);
                }
                auto success = direct(packet.target, [&](auto& scrollback)
                {
                    scrollback._data(count, filler.pick(), cell::shaders::meta);
                });
                if (!success)
                {
                    count = 0;
                }
            }
            else
            {
                log(prompt, "WriteConsoleOutputCharacter",
                            "\n\tinput.coor: ", coord,
                            "\n\tinput.type: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage));
                if (packet.input.etype == type::ansiOEM)
                {
                    auto recs = take_buffer<char, feed::fwd>(packet);
                    if (outenc->codepage != CP_UTF8)
                    {
                        toUTF8.clear();
                        outenc->decode(buffer, toUTF8);
                        celler = toUTF8;
                        log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                    }
                    else
                    {
                        celler = buffer;
                        log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(buffer)));
                    }
                }
                else
                {
                    auto recs = take_buffer<wchr, feed::fwd>(packet);
                    toUTF8.clear();
                    utf::to_utf(recs, toUTF8);
                    celler = toUTF8;
                    log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                }
                auto success = direct(packet.target, [&](auto& scrollback)
                {
                    auto& line = celler.content();
                    count = static_cast<ui32>(line.length());
                    if (count > maxsz)
                    {
                        count = maxsz;
                        line.crop(maxsz);
                    }
                    scrollback._data<true>(count, line.pick(), cell::shaders::text);
                });
                if (!success)
                {
                    count = 0;
                }
            }
            screen.cup0(saved);
            packet.reply.count = count;
        }
        else
        {
            auto count = ui32{};
            auto coord = twod{ packet.input.coorx, packet.input.coory };
            if (packet.input.etype == type::attribute)
            {
                auto recs = take_buffer<ui16, feed::fwd>(packet);
                count = static_cast<ui32>(recs.size());
                log(prompt, "WriteConsoleOutputAttribute",
                            "\n\tinput.coord: ", coord,
                            "\n\tinput.count: ", count);
            }
            else
            {
                log(prompt, "WriteConsoleOutputCharacter",
                            "\n\tinput.coor: ", coord,
                            "\n\tinput.type: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage));
                if (packet.input.etype == type::ansiOEM)
                {
                    auto recs = take_buffer<char, feed::fwd>(packet);
                    count = static_cast<ui32>(recs.size());
                    if (outenc->codepage != CP_UTF8)
                    {
                        toUTF8.clear();
                        outenc->decode(buffer, toUTF8);
                        log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                    }
                    else
                    {
                        log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(buffer)));
                    }
                }
                else
                {
                    auto recs = take_buffer<wchr, feed::fwd>(packet);
                    count = static_cast<ui32>(recs.size());
                    toUTF8.clear();
                    utf::to_utf(recs, toUTF8);
                    log ("\tinput.data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                }
            }
            packet.reply.count = count;
        }
    }
    auto api_scrollback_write_block          ()
    {
        log(prompt, "WriteConsoleOutput");
        struct payload : drvpacket<payload>
        {
            union
            {
                struct
                {
                    si16 rectL, rectT, rectR, rectB;
                    byte utf16;
                }
                input;
                struct
                {
                    si16 rectL, rectT, rectR, rectB;
                    byte _pad1;
                }
                reply;
            };
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr)
        {
            packet.reply = {};
            return;
        }
        auto& window = *window_ptr;
        auto view = rect{{ packet.input.rectL, packet.input.rectT },
                         { std::max(0, packet.input.rectR - packet.input.rectL + 1),
                           std::max(0, packet.input.rectB - packet.input.rectT + 1) }};
        auto crop = view;
        auto recs = take_buffer<CHAR_INFO, feed::fwd>(packet);
        if constexpr (isreal())
        {
            crop = view.trunc(window.panel);
            mirror.size(window.panel);
            mirror.view(crop);
            if (recs.size() && crop)
            {
                auto copy = netxs::raster(recs, view);
                auto& codec = *outenc;
                if (!packet.input.utf16 && codec.codepage != CP_UTF8) // Decode from OEM.
                {
                    if (langmap().contains(codec.codepage)) // Decode DBCS to UTF-16.
                    {
                        auto prev = PCHAR_INFO{};
                        auto lead = byte{};
                        netxs::onrect(copy, crop, [&](auto& r)
                        {
                            auto& code = r.Char.UnicodeChar;
                            auto& attr = r.Attributes;
                            if (auto w = !lead && ((attr & COMMON_LVB_LEADING_BYTE) || codec.test(code & 0xff)))
                            {
                                attr &= ~(COMMON_LVB_SBCSDBCS);
                                if (code)
                                {
                                    attr |= COMMON_LVB_LEADING_BYTE;
                                    lead = (byte)code;
                                    prev = &r;
                                }
                            }
                            else
                            {
                                attr &= ~(COMMON_LVB_SBCSDBCS);
                                if (lead) // Trailing byte.
                                {
                                    code = codec.decode_char(lead, (byte)code);
                                    attr |= COMMON_LVB_TRAILING_BYTE;
                                    prev->Char.UnicodeChar = code;
                                    lead = {};
                                }
                                else // Single byte character.
                                {
                                    code = codec.decode_char(code);
                                }
                            }
                        }, [&]
                        {
                            lead = {};
                        });
                    }
                    else // Decode SBCS to UTF-16.
                    {
                        for (auto& r : recs)
                        {
                            auto& code = r.Char.UnicodeChar;
                            code = codec.decode_char(code);
                        }
                    }
                }
                toUTF8.clear();
                auto& dest = (rich&)mirror;
                auto  prev = (cell*)nullptr;
                auto  skip = ui16{};
                auto  code = utfx{};
                auto eolfx = [&] // Reset state on new line.
                {
                    prev = {};
                    code = {};
                    skip = {};
                };
                auto allfx = [&](auto& dst, auto& src)
                {
                    //if (src.Char.UnicodeChar == '\0') // Transparent cell support?
                    //{
                    //    dst.txt('\0');
                    //    eolfx();
                    //    return;
                    //}
                    dst.meta(attr_to_brush(src.Attributes));
                    if (skip) // Right half of wide char.
                    {
                        if (skip == src.Char.UnicodeChar)
                        {
                            dst.txt(toUTF8, 3);
                            skip = {};
                            return;
                        }
                        skip = {};
                    }
                    if (utf::tocode((wchr)src.Char.UnicodeChar, code))
                    {
                        toUTF8.clear();
                        utf::to_utf_from_code(code, toUTF8);
                        auto& prop = unidata::select(code);
                        if (prev) // Surrogate pair.
                        {
                            if (prop.ucwidth == unidata::widths::wide)
                            {
                                prev->txt(toUTF8, 2);
                                dst  .txt(toUTF8, 3);
                            }
                            else // Narrow surrogate pair.
                            {
                                prev->txt(toUTF8, 1);
                                dst.txt(whitespace);
                            }
                            prev = {};
                        }
                        else
                        {
                            if (prop.ucwidth == unidata::widths::wide)
                            {
                                if (src.Attributes & COMMON_LVB_TRAILING_BYTE)
                                {
                                    dst.txt(toUTF8, 3); // Right half of wide char.
                                }
                                else
                                {
                                    dst.txt(toUTF8, 2); // Left half of wide char.
                                    skip = src.Char.UnicodeChar;
                                }
                            }
                            else
                            {
                                dst.txt(toUTF8, 1); // Narrow character.
                            }
                        }
                        code = {};
                    }
                    else
                    {
                        prev = &dst; // Go to the next.
                    }
                };
                netxs::onbody(dest, copy, allfx, eolfx);
                auto success = direct(packet.target, [&](auto& scrollback)
                {
                    write_block(scrollback, dest, crop.coor, rect{ dot_00, window.panel }, cell::shaders::full); // cell::shaders::skipnuls for transparency?
                });
                if (!success) crop = {};
            }
        }
        packet.reply.rectL = crop.coor.x;
        packet.reply.rectT = crop.coor.y;
        packet.reply.rectR = crop.coor.x + crop.size.x - 1;
        packet.reply.rectB = crop.coor.y + crop.size.y - 1;
        log("\tinput.type: ", show_page(packet.input.utf16, outenc->codepage),
            "\n\tinput.rect: ", view,
            "\n\treply.rect: ", crop,
            "\n\twrite data:\n\t", utf::change(ansi::s11n((rich&)mirror, crop), "\n", ansi::pushsgr().nil().add("\n\t").popsgr()));
    }
    auto api_scrollback_attribute_set        ()
    {
        log(prompt, "SetConsoleTextAttribute");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui16 color;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        if constexpr (isreal())
        {
            if (!direct(packet.target, [&](auto& scrollback) { scrollback.brush = attr_to_brush(packet.input.color); }))
            {
                log("\tunexpected result");
            }
            log("\tset default attributes: ", uiterm.target->brush);
        }
        else
        {
            log("\tset default attributes: ", utf::to_hex_0x(packet.input.color));
        }
    }
    auto api_scrollback_fill                 ()
    {
        struct payload : drvpacket<payload>
        {
            union
            {
                struct
                {
                    si16 coorx, coory;
                    type etype;
                    ui16 piece;
                    ui32 count;
                }
                input;
                struct
                {
                    si16 _pad1, _pad2;
                    ui32 _pad3;
                    ui16 _pad4;
                    ui32 count;
                }
                reply;
            };
        };
        auto& packet = payload::cast(upload);
        auto& screen = *uiterm.target;
        auto count = packet.input.count;
        if (!count)
        {
            packet.reply.count = 0;
            return;
        }
        if constexpr (isreal())
        {
            auto coord = std::clamp(twod{ packet.input.coorx, packet.input.coory }, dot_00, screen.panel - dot_11);
            auto piece = packet.input.piece;
            auto maxsz = screen.panel.x * (screen.panel.y - coord.y) - coord.x;
            auto saved = screen.coord;
            screen.cup0(coord);
            if (packet.input.etype == type::attribute)
            {
                log(prompt, "FillConsoleOutputAttribute",
                            "\n\tcoord: ", coord,
                            "\n\tcount: ", count);
                impcls = impcls && coord == dot_00 && count == screen.panel.x * screen.panel.y;
                if (impcls)
                {
                    log("\timplicit screen clearing detected");
                    screen.clear_all();
                }
                else
                {
                    auto c = attr_to_brush(piece);
                    log("\tfill using attributes: ", (int)c);
                    if ((si32)count > maxsz) count = std::max(0, maxsz);
                    filler.kill();
                    filler.size(count, c);
                    if (!direct(packet.target, [&](auto& scrollback) { scrollback._data(count, filler.pick(), cell::shaders::meta); }))
                    {
                        count = 0;
                    }
                }
            }
            else
            {
                log(prompt, "FillConsoleOutputCharacter",
                            "\n\tcodec: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage),
                            "\n\tcoord: ", coord,
                            "\n\tcount: ", count);
                impcls = coord == dot_00 && piece == ' ' && count == screen.panel.x * screen.panel.y;
                if (piece <  ' ' || piece == 0x7F) piece = ' ';
                if (piece == ' ' && (si32)count > maxsz)
                {
                    log("\taction: erase below");
                    screen.ed(0 /*commands::erase::display::below*/);
                }
                else
                {
                    toUTF8.clear();
                    if (packet.input.etype != type::ansiOEM)
                    {
                        utf::to_utf((wchr)piece, toUTF8);
                    }
                    else
                    {
                        if (outenc->codepage == CP_UTF8) toUTF8.push_back(piece & 0xff);
                        else                             outenc->decode(piece & 0xff, toUTF8);
                    }
                    log("\tfiller: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                    auto c = cell{ toUTF8 };
                    auto w = c.wdt();
                    if ((si32)count > maxsz) count = std::max(0, maxsz);
                    count *= w;
                    filler.kill();
                    filler.size(count, c);
                    if (w == 2)
                    {
                        auto head = filler.iter();
                        auto tail = filler.iend();
                        while (head != tail)
                        {
                            (++head)->wdt(3);
                            (++head);
                        }
                    }
                    if (!direct(packet.target, [&](auto& scrollback) { scrollback._data(count, filler.pick(), cell::shaders::text); }))
                    {
                        count = 0;
                    }
                }
            }
            screen.cup0(saved);
        }
        else
        {
            auto coord = twod{ packet.input.coorx, packet.input.coory };
            auto piece = packet.input.piece;
            if (packet.input.etype == type::attribute)
            {
                log(prompt, "FillConsoleOutputAttribute",
                            "\n\tcoord: ", coord,
                            "\n\tcount: ", count,
                            "\tfill using attributes: ", (int)attr_to_brush(piece));
            }
            else
            {
                log(prompt, "FillConsoleOutputCharacter",
                            "\n\tcodec: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage),
                            "\n\tcoord: ", coord,
                            "\n\tcount: ", count);
                toUTF8.clear();
                if (packet.input.etype != type::ansiOEM)
                {
                    utf::to_utf((wchr)piece, toUTF8);
                }
                else
                {
                    if (outenc->codepage == CP_UTF8) toUTF8.push_back(piece & 0xff);
                    else                             outenc->decode(piece & 0xff, toUTF8);
                }
                log("\tfiller: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
            }
        }
        packet.reply.count = count;
    }
    auto api_scrollback_read_data            ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 coorx, coory;
                type etype;
            }
            input;
            struct
            {
                ui32 count;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.count = {};
        log(prompt, packet.input.etype == type::attribute ? "ReadConsoleOutputAttribute"
                                                          : "ReadConsoleOutputCharacter");
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr)
        {
            return;
        }
        auto& window = *window_ptr;
        auto avail = size_check(packet.echosz, answer.sendoffset());
        if (!avail)
        {
            return;
        }
        auto recsz = packet.input.etype == type::ansiOEM ? sizeof(char) : sizeof(ui16);
        auto count = static_cast<si32>(avail / recsz);

        auto coor = twod{ packet.input.coorx, packet.input.coory };
        if constexpr (isreal())
        {
            auto view = rect{{ 0, coor.y }, { window.panel.x, (coor.x + count) / window.panel.x + 1 }};
            view = view.trunc(window.panel);
            count = std::max(0, std::min(view.size.x * view.size.y, coor.x + count) - coor.x);
            if (!view || !count)
            {
                return;
            }
            auto start = coor.x + coor.y * window.panel.x;
            buffer.clear();
            auto mark = cell{};
            auto attr = brush_to_attr(mark);
            mirror.size(window.panel);
            mirror.view(view);
            mirror.fill(mark);
            window.do_viewport_copy(mirror);
            auto& copy = (rich&)mirror;
            if (packet.input.etype == type::attribute)
            {
                log("\tinput.type: attributes");
                auto recs = wrap<ui16>::cast(buffer, count);
                auto iter = recs.begin();
                auto head = mirror.iter() + start;
                auto tail = head + count;
                while (head != tail)
                {
                    auto& src = *head++;
                    auto& dst = *iter++;
                    if (!src.like(mark))
                    {
                        attr = brush_to_attr(src);
                        mark = src;
                    }
                    dst = attr;
                    //set_half(src.wdt(), dst);
                }
                answer.send_data(condrv, recs);
            }
            else
            {
                auto head = mirror.iter() + start;
                auto tail = head + count;
                log("\tinput.type: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage));
                toUTF8.clear();
                while (head != tail)
                {
                    auto& src = *head++;
                    if (src.wdt() != 3) toUTF8 += src.txt();
                }
                if (packet.input.etype == type::ansiOEM)
                {
                    auto& codec = *outenc;
                    auto recs = wrap<char>::cast(buffer, count);
                    if (codec.codepage == CP_UTF8)
                    {
                        count = std::min(count, (si32)toUTF8.size());
                        toUTF8.resize(count);
                        std::copy(toUTF8.begin(), toUTF8.end(), recs.begin());
                        log("\treply data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
                    }
                    else
                    {
                        toANSI.clear();
                        auto utf8 = netxs::view{ toUTF8 };
                        codec.encode(utf8, toANSI, count);
                        count = std::min(count, (si32)toANSI.size());
                        std::copy(toANSI.begin(), toANSI.end(), recs.begin());
                        log("\treply data: ", ansi::hi(utf::debase<faux, faux>(outenc->decode_log(toANSI))));
                    }
                    answer.send_data(condrv, recs);
                }
                else
                {
                    toWIDE.clear();
                    utf::to_utf(toUTF8, toWIDE);
                    auto recs = wrap<wchr>::cast(buffer, count);
                    count = std::min(count, (si32)toWIDE.size());
                    toWIDE.resize(count);
                    std::copy(toWIDE.begin(), toWIDE.end(), recs.begin());
                    log("\treply data: ", ansi::hi(utf::debase<faux, faux>(utf::to_utf(recs))));
                    answer.send_data(condrv, recs);
                }
            }
        }
        else
        {
            buffer.clear();
            auto mark = cell{};
            auto attr = brush_to_attr(mark);
            if (packet.input.etype == type::attribute)
            {
                log("\tinput.type: attributes");
                auto recs = wrap<ui16>::cast(buffer, count);
                auto head = recs.begin();
                auto tail = recs.end();
                while (head != tail)
                {
                    auto& dst = *head++;
                    dst = attr;
                }
                answer.send_data(condrv, recs);
            }
            else
            {
                log("\tinput.type: ", show_page(packet.input.etype != type::ansiOEM, outenc->codepage));
                if (packet.input.etype == type::ansiOEM)
                {
                    toUTF8.clear();
                    toUTF8.assign(count, ' ');
                    auto recs = wrap<char>::cast(buffer, count);
                    std::copy(toUTF8.begin(), toUTF8.end(), recs.begin());
                    answer.send_data(condrv, recs);
                }
                else
                {
                    toWIDE.clear();
                    toWIDE.assign(count, ' ');
                    auto recs = wrap<wchr>::cast(buffer, count);
                    std::copy(toWIDE.begin(), toWIDE.end(), recs.begin());
                    answer.send_data(condrv, recs);
                }
                log("\treply data: ", ansi::hi(utf::debase<faux, faux>(toUTF8)));
            }
        }
        packet.reply.count = count;
        log("\treply.count: ", count);
    }
    auto api_scrollback_read_block           ()
    {
        log(prompt, "ReadConsoleOutput");
        struct payload : drvpacket<payload>
        {
            union
            {
                struct
                {
                    si16 rectL, rectT, rectR, rectB;
                    byte utf16;
                }
                input;
                struct
                {
                    si16 rectL, rectT, rectR, rectB;
                    byte _pad1;
                }
                reply;
            };
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr)
        {
            packet.reply = {};
            return;
        }
        auto& window = *window_ptr;
        auto view = rect{{ packet.input.rectL, packet.input.rectT },
                         { std::max(0, packet.input.rectR - packet.input.rectL + 1),
                           std::max(0, packet.input.rectB - packet.input.rectT + 1) }};
        buffer.clear();
        auto recs = wrap<CHAR_INFO>::cast(buffer, view.size.x * view.size.y);
        auto crop = view;
        auto size = view.size;
        if constexpr (isreal())
        {
            if (recs.size())
            {
                auto mark = cell{};
                auto attr = brush_to_attr(mark);
                size = window.panel;
                mirror.size(window.panel);
                mirror.view(view);
                mirror.fill(mark);
                window.do_viewport_copy(mirror);
                crop = mirror.view();
                auto& copy = (rich&)mirror;
                auto  dest = netxs::raster(recs, view);
                if (packet.input.utf16)
                {
                    netxs::onbody(dest, copy, [&](auto& dst, auto& src)
                    {
                        if (!src.like(mark))
                        {
                            attr = brush_to_attr(src);
                            mark = src;
                        }
                        dst.Attributes = attr;
                        toWIDE.clear();
                        utf::to_utf(src.txt(), toWIDE);
                        auto wdt = src.wdt();
                        auto& c = dst.Char.UnicodeChar;
                        if (toWIDE.size())
                        {
                            if (wdt == 1)
                            {
                                c = toWIDE[0];
                            }
                            else if (wdt == 2) // Left half.
                            {
                                c = toWIDE[0];
                                if (!(c >= 0xd800 && c <= 0xdbff)) // Not the first part of surrogate pair.
                                {
                                    set_half(wdt, dst.Attributes);
                                }
                            }
                            else if (wdt == 3) // Right half.
                            {
                                if (toWIDE.size() > 1)
                                {
                                    dst.Char.UnicodeChar = toWIDE[1];
                                }
                                else
                                {
                                    dst.Char.UnicodeChar = toWIDE[0];
                                }
                                if (!(c >= 0xdc00 && c <= 0xdfff)) // Not the second part of surrogate pair.
                                {
                                    set_half(wdt, dst.Attributes);
                                }
                            }
                        }
                    });
                }
                else
                {
                    auto& codec = *outenc;
                    if (codec.codepage == CP_UTF8) // UTF-8 multibyte: Possible lost data.
                    {
                        netxs::onbody(dest, copy, [&](auto& dst, auto& src)
                        {
                            if (!src.like(mark))
                            {
                                attr = brush_to_attr(src);
                                mark = src;
                            }
                            dst.Attributes = attr;
                            auto utf8 = src.txt();
                            auto wdt = src.wdt();
                            //set_half(wdt, attr);
                            dst.Char.AsciiChar = utf8.size() ? utf8.front() : ' ';
                        });
                    }
                    else // OEM multibyte, DBCS: Possible lost data.
                    {
                        netxs::onbody(dest, copy, [&](auto& dst, auto& src)
                        {
                            if (!src.like(mark))
                            {
                                attr = brush_to_attr(src);
                                mark = src;
                            }
                            dst.Attributes = attr;
                            auto utf8 = src.txt();
                            auto wdt = src.wdt();
                            set_half(wdt, dst.Attributes);
                            toANSI.clear();
                            codec.encode(utf8, toANSI);
                                 if (toANSI.empty())                dst.Char.AsciiChar = ' ';
                            else if (wdt == 3 && toANSI.size() > 1) dst.Char.AsciiChar = toANSI[1];
                            else                                    dst.Char.AsciiChar = toANSI[0];
                        });
                    }
                }
                answer.send_data(condrv, recs);
            }
        }
        else
        {
            if (recs.size())
            {
                answer.send_data(condrv, recs);
            }
        }
        packet.reply.rectL = crop.coor.x;
        packet.reply.rectT = crop.coor.y;
        packet.reply.rectR = crop.coor.x + crop.size.x - 1;
        packet.reply.rectB = crop.coor.y + crop.size.y - 1;
        log("\treply.type: ", show_page(packet.input.utf16, outenc->codepage),
          "\n\tpanel size: ", size,
          "\n\tinput.rect: ", view,
          "\n\treply.rect: ", crop,
          "\n\treply data:\n\t", utf::change(ansi::s11n((rich&)mirror, crop), "\n", ansi::pushsgr().nil().add("\n\t").popsgr()));
    }
    auto api_scrollback_set_active           ()
    {
        log(prompt, "SetConsoleActiveScreenBuffer");
        struct payload : drvpacket<payload>
        { };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        log("\tset active buffer: ", utf::to_hex_0x(packet.target));
        if constexpr (isreal())
        {
            auto& console = *window_ptr;
            uiterm.reset_to_altbuf(console);
        }
    }
    auto api_scrollback_cursor_coor_set      ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 coorx, coory;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log(prompt, "SetConsoleCursorPosition ");
        auto caretpos = twod{ packet.input.coorx, packet.input.coory };
        log("\tinput.cursor_coor: ", caretpos);
        if constexpr (isreal())
        {
            if (auto console_ptr = select_buffer(packet.target))
            {
                console_ptr->cup0(caretpos);
            }
        }
    }
    auto api_scrollback_cursor_info_get      ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 style;
                byte alive;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        if constexpr (isreal())
        {
            auto [form, show] = uiterm.cursor.style();
            packet.reply.style = form ? 100 : 1;
            packet.reply.alive = show;
        }
        else
        {
            packet.reply.style = 1;
            packet.reply.alive = 1;
        }
        log(prompt, "GetConsoleCursorInfo",
            "\n\treply.style: ", packet.reply.style,
            "\n\treply.alive: ", packet.reply.alive ? "true" : "faux");
    }
    auto api_scrollback_cursor_info_set      ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 style;
                byte alive;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log(prompt, "SetConsoleCursorInfo",
            "\n\tinput.style: ", packet.input.style,
            "\n\tinput.alive: ", packet.input.alive ? "true" : "faux");
        if (packet.target && packet.target->link == &uiterm.target)
        {
            if constexpr (isreal())
            {
                uiterm.cursor.style(packet.input.style > 50);
                packet.input.alive ? uiterm.cursor.show()
                                   : uiterm.cursor.hide();
            }
        }
        else
        {
            log("\taborted: inactive buffer: ", utf::to_hex_0x(packet.target));
        }
    }
    auto api_scrollback_info_get             ()
    {
        log(prompt, "GetConsoleScreenBufferInfo");
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 buffersz_x, buffersz_y;
                si16 cursorposx, cursorposy;
                si16 windowposx, windowposy;
                ui16 attributes;
                si16 windowsz_x, windowsz_y;
                si16 maxwinsz_x, maxwinsz_y;
                ui16 popupcolor;
                byte fullscreen;
                ui32 rgbpalette[16];
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        auto& console = *window_ptr;
        auto viewport = dot_00;
        auto caretpos = dot_00;
        if constexpr (isreal())
        {
            viewport = console.panel;
            caretpos = console.coord;
        }
        else
        {
            viewport = os::ttysize;
        }
        packet.reply.cursorposx = caretpos.x;
        packet.reply.cursorposy = caretpos.y;
        packet.reply.buffersz_x = viewport.x;
        packet.reply.buffersz_y = viewport.y;
        packet.reply.windowsz_x = viewport.x;
        packet.reply.windowsz_y = viewport.y;
        packet.reply.maxwinsz_x = viewport.x;
        packet.reply.maxwinsz_y = viewport.y;
        packet.reply.windowposx = 0;
        packet.reply.windowposy = 0;
        packet.reply.fullscreen = faux;
        packet.reply.popupcolor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        auto fgcx = 7_sz; // Fallback for true colors.
        auto bgcx = 0_sz;
        if constexpr (isreal())
        {
            auto& rgbpalette = packet.reply.rgbpalette;
            auto mark = console.brush;
            auto frgb = mark.fgc().token;
            auto brgb = mark.bgc().token;
            auto head = std::begin(uiterm.ctrack.color);
            for (auto i = 0; i < 16; i++)
            {
                auto const& c = *head++;
                auto m = netxs::swap_bits<0, 2>(i); // ANSI<->DOS color scheme reindex.
                rgbpalette[m] = c;
                if (c == frgb) fgcx = m;
                if (c == brgb) bgcx = m + 1;
            }
            if (brgb && !bgcx--) // Reset background if true colors are used.
            {
                bgcx = 0;
                uiterm.ctrack.color[bgcx] = brgb;
                rgbpalette         [bgcx] = brgb;
            }
            packet.reply.attributes = static_cast<ui16>(fgcx + (bgcx << 4));
            if (mark.inv()) packet.reply.attributes |= COMMON_LVB_REVERSE_VIDEO;
            if (mark.und()) packet.reply.attributes |= COMMON_LVB_UNDERSCORE;
            if (mark.ovr()) packet.reply.attributes |= COMMON_LVB_GRID_HORIZONTAL;
        }
        else
        {
            packet.reply.attributes = static_cast<ui16>(fgcx + (bgcx << 4));
        }
        log("\treply.attributes: ", utf::to_hex_0x(packet.reply.attributes),
            "\n\treply.cursor_coor: ", caretpos,
            "\n\treply.window_size: ", viewport);
    }
    auto api_scrollback_info_set             ()
    {
        log(prompt, "SetConsoleScreenBufferInfo");
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 buffersz_x, buffersz_y;
                si16 cursorposx, cursorposy;
                si16 windowposx, windowposy;
                ui16 attributes;
                si16 windowsz_x, windowsz_y;
                si16 maxwinsz_x, maxwinsz_y;
                ui16 popupcolor;
                byte fullscreen;
                ui32 rgbpalette[16];
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        auto& console = *window_ptr;
        auto caretpos = twod{ packet.input.cursorposx, packet.input.cursorposy };
        auto buffsize = twod{ packet.input.buffersz_x, packet.input.buffersz_y };
        auto windowsz = twod{ packet.input.windowsz_x, packet.input.windowsz_y };
        if constexpr (isreal())
        {
            console.cup0(caretpos);
            console.brush.meta(attr_to_brush(packet.input.attributes));
            if (&console != uiterm.target) // If not active buffer.
            {
                check_buffer_size(console, buffsize);
                console.resize_viewport(buffsize);
            }
            else // If active buffer.
            {
                uiterm.window_resize(windowsz);
            }
        }
        log("\tbuffer size: ", buffsize);
        log("\tcursor coor: ", twod{ packet.input.cursorposx, packet.input.cursorposy });
        log("\twindow coor: ", twod{ packet.input.windowposx, packet.input.windowposy });
        log("\tattributes : ", utf::to_hex_0x(packet.input.attributes));
        log("\twindow size: ", windowsz);
        log("\tmaxwin size: ", twod{ packet.input.maxwinsz_x, packet.input.maxwinsz_y });
        log("\tpopup color: ", packet.input.popupcolor);
        log("\tfull screen: ", packet.input.fullscreen);
        log("\trgb palette: ");
        auto i = 0;
        for (auto c : packet.input.rgbpalette)
        {
            log("\t\t", utf::to_hex(i), " ", rgba{ c });
        }
        if constexpr (isreal())
        {
            //todo set palette per buffer
            auto& rgbpalette = packet.input.rgbpalette;
            auto head = std::begin(uiterm.ctrack.color);
            for (auto i = 0; i < 16; i++)
            {
                auto m = netxs::swap_bits<0, 2>(i); // ANSI<->DOS color scheme reindex.
                *head++ = rgbpalette[m];
            }
        }
    }
    auto api_scrollback_size_set             ()
    {
        log(prompt, "SetConsoleScreenBufferSize");
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 buffersz_x, buffersz_y;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        auto& console = *window_ptr;
        auto size = twod{ packet.input.buffersz_x, packet.input.buffersz_y };
        log("\tinput.size: ", size);
        if constexpr (isreal())
        {
            if (packet.target->link != &uiterm.target) // It is additional/alternate buffer.
            {
                console.resize_viewport(size);
            }
            else
            {
                check_buffer_size(console, size);
                uiterm.window_resize(size);
            }
            auto viewport = console.panel;
            packet.input.buffersz_x = viewport.x;
            packet.input.buffersz_y = viewport.y;
        }
    }
    auto api_scrollback_viewport_get_max_size()
    {
        log(prompt, "GetLargestConsoleWindowSize");
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 maxwinsz_x, maxwinsz_y;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        if constexpr (isreal())
        {
            auto& console = *window_ptr;
            auto viewport = console.panel;
            packet.reply.maxwinsz_x = viewport.x;
            packet.reply.maxwinsz_y = viewport.y;
        }
        else
        {
            auto viewport = os::ttysize;
            packet.reply.maxwinsz_x = viewport.x;
            packet.reply.maxwinsz_y = viewport.y;
        }
        log("\treply.maxwin size: ", twod{ packet.reply.maxwinsz_x, packet.reply.maxwinsz_y });
    }
    auto api_scrollback_viewport_set         ()
    {
        log(prompt, "SetConsoleWindowInfo");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte isabsolute;
                si16 rectL, rectT, rectR, rectB;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto area = rect{{ packet.input.rectL, packet.input.rectT },
                         { std::max(0, packet.input.rectR - packet.input.rectL + 1),
                           std::max(0, packet.input.rectB - packet.input.rectT + 1) }};
        log("\tinput.area: ", area,
          "\n\tinput.isabsolute: ", packet.input.isabsolute ? "true" : "faux");
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        if constexpr (isreal())
        {
            auto& console = *window_ptr;
            auto viewport = console.panel;
            packet.input.rectL = 0;
            packet.input.rectT = 0;
            packet.input.rectR = viewport.y - 1;
            packet.input.rectB = viewport.y - 1;
            packet.input.isabsolute = 1;
        }
    }
    auto api_scrollback_scroll               ()
    {
        log(prompt, "ScrollConsoleScreenBuffer");
        struct payload : drvpacket<payload>
        {
            struct
            {
                si16 scrlL, scrlT, scrlR, scrlB;
                si16 clipL, clipT, clipR, clipB;
                byte trunc;
                byte utf16;
                si16 destx, desty;
                union
                {
                wchr wchar;
                char ascii;
                };
                ui16 color;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr)
        {
            return;
        }
        if constexpr (isreal())
        {
            auto& window = *window_ptr;
            if (packet.input.destx == 0 && packet.input.desty ==-window.panel.y
             && packet.input.scrlL == 0 && packet.input.scrlR == window.panel.x
             && packet.input.scrlT == 0 && packet.input.scrlB == window.panel.y)
            {
                log("\timplicit screen clearing detected");
                window.clear_all();
                return;
            }
            auto scrl = rect{{ packet.input.scrlL, packet.input.scrlT },
                             { std::max(0, packet.input.scrlR - packet.input.scrlL + 1),
                               std::max(0, packet.input.scrlB - packet.input.scrlT + 1) }};
            auto clip = !packet.input.trunc ? rect{ dot_00, window.panel }
                                            : rect{{ packet.input.clipL, packet.input.clipT },
                                                   { std::max(0, packet.input.clipR - packet.input.clipL + 1),
                                                     std::max(0, packet.input.clipB - packet.input.clipT + 1) }};
            auto dest = twod{ packet.input.destx, packet.input.desty };
            auto mark = attr_to_brush(packet.input.color).txt(utf::to_utf(packet.input.wchar));
            log("\tinput.scrl.rect: ", scrl,
              "\n\tinput.clip.rect: ", clip,
              "\n\tinput.dest.coor: ", dest,
              "\n\tinput.trunc: ", packet.input.trunc ? "true" : "faux",
              "\n\tinput.utf16: ", packet.input.utf16 ? "true" : "faux",
              "\n\tinput.brush: ", mark);
            scrl = scrl.trunc(window.panel);
            clip = clip.trunc(window.panel);
            mirror.size(window.panel);
            mirror.view(scrl);
            mirror.fill(cell{});
            window.do_viewport_copy(mirror);
            mirror.view(scrl);
            filler.kill();
            filler.mark(mark);
            filler.size(scrl.size);
            auto success = direct(packet.target, [&](auto& scrollback)
            {
                write_block(scrollback, filler, scrl.coor, clip, cell::shaders::full);
                write_block(scrollback, mirror, dest,      clip, cell::shaders::full);
            });
        }
        else
        {
            auto scrl = rect{{ packet.input.scrlL, packet.input.scrlT },
                             { std::max(0, packet.input.scrlR - packet.input.scrlL + 1),
                               std::max(0, packet.input.scrlB - packet.input.scrlT + 1) }};
            auto clip = rect{{ packet.input.clipL, packet.input.clipT },
                             { std::max(0, packet.input.clipR - packet.input.clipL + 1),
                               std::max(0, packet.input.clipB - packet.input.clipT + 1) }};
            auto dest = twod{ packet.input.destx, packet.input.desty };
            auto mark = attr_to_brush(packet.input.color).txt(utf::to_utf(packet.input.wchar));
            log("\tinput.scrl.rect: ", scrl,
              "\n\tinput.clip.rect: ", clip,
              "\n\tinput.dest.coor: ", dest,
              "\n\tinput.trunc: ", packet.input.trunc ? "true" : "faux",
              "\n\tinput.utf16: ", packet.input.utf16 ? "true" : "faux",
              "\n\tinput.brush: ", mark);
        }
    }
    auto api_scrollback_selection_info_get   ()
    {
        log(prompt, "GetConsoleSelectionInfo");
        static constexpr auto mouse_down   = ui32{ CONSOLE_MOUSE_DOWN            };
        static constexpr auto use_mouse    = ui32{ CONSOLE_MOUSE_SELECTION       };
        static constexpr auto no_selection = ui32{ CONSOLE_NO_SELECTION          };
        static constexpr auto in_progress  = ui32{ CONSOLE_SELECTION_IN_PROGRESS };
        static constexpr auto not_empty    = ui32{ CONSOLE_SELECTION_NOT_EMPTY   };
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 flags;
                si16 headx, heady;
                si16 rectL, rectT, rectR, rectB;
            }
            reply;
        };
        auto& packet = payload::cast(upload);

    }
    auto api_window_title_get                ()
    {
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
            struct
            {
                byte utf16;
                byte prime;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log(prompt, packet.input.prime ? "GetConsoleOriginalTitle"
                                       : "GetConsoleTitle");
        //todo differentiate titles by category
        auto title = view{};
        if constexpr (isreal())
        {
            title = uiterm.wtrack.get(ansi::osc_title);
        }
        log("\t", show_page(packet.input.utf16, inpenc->codepage),
            ": ", ansi::hi(utf::debase(title)));
        if (packet.input.utf16)
        {
            toWIDE.clear();
            utf::to_utf(title, toWIDE);
            packet.reply.count = static_cast<ui32>(toWIDE.size() + 1 /*null terminator*/);
            answer.send_data(condrv, toWIDE, true);
        }
        else if (inpenc->codepage == CP_UTF8)
        {
            packet.reply.count = static_cast<ui32>(title.size() + 1 /*null terminator*/);
            answer.send_data(condrv, title, true);
        }
        else
        {
            auto shadow = view{ title };
            auto toANSI = inpenc->encode(shadow);
            packet.reply.count = static_cast<ui32>(toANSI.size() + 1 /*null terminator*/);
            answer.send_data(condrv, toANSI, true);
        }
    }
    auto api_window_title_set                ()
    {
        log(prompt, "SetConsoleTitle");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        auto utf8_title = text{};
        if (packet.input.utf16)
        {
            auto title = take_buffer<wchr, feed::fwd>(packet);
            utf::to_utf(title, utf8_title);
        }
        else
        {
            auto title = take_buffer<char, feed::fwd>(packet);
            if (inpenc->codepage == CP_UTF8) utf8_title = qiew{ title };
            else                             inpenc->decode_run(title, utf8_title);
        }
        if constexpr (isreal())
        {
            uiterm.wtrack.set(ansi::osc_title, utf8_title);
        }
        log("\t", show_page(packet.input.utf16, inpenc->codepage),
            ": ", ansi::hi(utf::debase<faux, faux>(utf8_title)));
    }
    auto api_window_font_size_get            ()
    {
        log(prompt, "GetConsoleFontSize");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 index;
            }
            input;
            struct
            {
                si16 sizex, sizey;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.sizex = 10;
        packet.reply.sizey = 20;
        log("\tinput.index: ", packet.input.index,
          "\n\treply.size : ", packet.reply.sizex, "x", packet.reply.sizey);
    }
    auto api_window_font_get                 ()
    {
        log(prompt, "GetCurrentConsoleFont");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte fullscreen;
            }
            input;
            struct
            {
                ui32 index;
                si16 sizex, sizey;
                ui32 pitch;
                ui32 heavy;
                wchr brand[LF_FACESIZE];
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.index = 0;
        packet.reply.sizex = 10;
        packet.reply.sizey = 20;
        packet.reply.pitch = 0;
        packet.reply.heavy = 0;
        auto brand = L"Consolas"s + L'\0';
        std::copy(std::begin(brand), std::end(brand), std::begin(packet.reply.brand));
        log("\tinput.fullscreen: ", packet.input.fullscreen ? "true" : "faux",
          "\n\treply.index: ",      packet.reply.index,
          "\n\treply.size : ",      packet.reply.sizex, "x", packet.reply.sizey,
          "\n\treply.pitch: ",      packet.reply.pitch,
          "\n\treply.heavy: ",      packet.reply.heavy,
          "\n\treply.brand: ", utf::to_utf(brand));
    }
    auto api_window_font_set                 ()
    {
        log(prompt, "SetConsoleCurrentFont");
        struct payload : drvpacket<payload>
        {
            union
            {
                struct
                {
                    byte fullscreen;
                    ui32 index;
                    si16 sizex, sizey;
                    ui32 pitch;
                    ui32 heavy;
                    wchr brand[LF_FACESIZE];
                }
                input;
                struct
                {
                    byte _pad1;
                    ui32 index;
                    si16 sizex, sizey;
                    ui32 pitch;
                    ui32 heavy;
                    wchr brand[LF_FACESIZE];
                }
                reply;
            };
        };
        auto& packet = payload::cast(upload);
        log("\tinput.fullscreen: ",        packet.input.fullscreen ? "true" : "faux",
          "\n\tinput.index: ",             packet.input.index,
          "\n\tinput.pitch: ",             packet.input.pitch,
          "\n\tinput.heavy: ",             packet.input.heavy,
          "\n\tinput.size : ",       twod{ packet.input.sizex, packet.input.sizey },
          "\n\tinput.brand: ", utf::to_utf(packet.input.brand));
    }
    auto api_window_mode_get                 ()
    {
        log(prompt, "GetConsoleDisplayMode");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 flags;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.flags = CONSOLE_WINDOWED_MODE;
        log("\treply.flags: ", packet.reply.flags);
    }
    auto api_window_mode_set                 ()
    {
        log(prompt, "SetConsoleDisplayMode");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 flags;
            }
            input;
            struct
            {
                si16 buffersz_x, buffersz_y;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        auto window_ptr = select_buffer(packet.target);
        if (!window_ptr) return;
        if constexpr (isreal())
        {
            auto& console = *window_ptr;
            packet.reply.buffersz_x = console.panel.x;
            packet.reply.buffersz_y = console.panel.y;
        }
        else
        {
            auto viewport = os::ttysize;
            packet.reply.buffersz_x = viewport.x;
            packet.reply.buffersz_y = viewport.y;
        }
        log("\tinput.flags: ", packet.input.flags);
        log("\treply.buffer size: ", twod{ packet.reply.buffersz_x, packet.reply.buffersz_y });
    }
    auto api_window_handle_get               ()
    {
        log(prompt, "GetConsoleWindow");
        struct payload : drvpacket<payload>
        {
            struct
            {
                HWND handle;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.handle = winhnd; // - Fake window handle to tell powershell that everything is under console control.
                                      // - GH#268: "git log" launches "less.exe" which crashes if reply=NULL.
                                      // - "Far.exe" set their icon to all windows in the system if reply=-1.
                                      // - msys uses the handle to determine what processes are running in the same session.
                                      // - vim sets the icon of its hosting window.
                                      // - The handle is used to show/hide GUI console window.
                                      // - Used for SetConsoleTitle().
        log("\tfake window handle: ", utf::to_hex_0x(packet.reply.handle));
    }
    auto api_window_xkeys                    ()
    {
        log(prompt, "SetConsoleKeyShortcuts");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte enabled;
                byte keyflag;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        log("\trequest ", packet.input.enabled ? "set" : "unset", " xkeys\n",
                          packet.input.keyflag & 0x01 ? "\t\tAlt+Tab\n"   : "",
                          packet.input.keyflag & 0x02 ? "\t\tAlt+Esc\n"   : "",
                          packet.input.keyflag & 0x04 ? "\t\tAlt+Space\n" : "",
                          packet.input.keyflag & 0x08 ? "\t\tAlt+Enter\n" : "",
                          packet.input.keyflag & 0x10 ? "\t\tAlt+Prtsc\n" : "",
                          packet.input.keyflag & 0x20 ? "\t\tPrtsc\n"     : "",
                          packet.input.keyflag & 0x40 ? "\t\tCtrl+Esc\n"  : "");
    }
    auto api_alias_get                       ()
    {
        log(prompt, "GetConsoleAlias");
        struct payload : drvpacket<payload>
        {
            union
            {
                struct
                {
                    ui16 srcsz;
                    ui16 _pad1;
                    ui16 exesz;
                    byte utf16;
                }
                input;
                struct
                {
                    ui16 _pad1;
                    ui16 dstsz;
                    ui16 _pad2;
                    byte _pad3;
                }
                reply;
            };
        };
        auto& packet = payload::cast(upload);
        packet.reply.dstsz = 0; // Far crashed if it is not set.
    }
    auto api_alias_add                       ()
    {
        log(prompt, "AddConsoleAlias");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui16 srcsz;
                ui16 dstsz;
                ui16 exesz;
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);

    }
    auto api_alias_exes_get_volume           ()
    {
        log(prompt, "GetConsoleAliasExesLength");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 bytes;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;

    }
    auto api_alias_exes_get                  ()
    {
        log(prompt, "GetConsoleAliasExes");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 bytes;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;

    }
    auto api_aliases_get_volume              ()
    {
        log(prompt, "GetConsoleAliasesLength");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 bytes;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;

    }
    auto api_aliases_get                     ()
    {
        log(prompt, "GetConsoleAliases");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte utf16;
            }
            input;
            struct
            {
                ui32 bytes;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;

    }
    auto api_input_history_clear             ()
    {
        log(prompt, "ClearConsoleCommandHistory");
        struct payload : drvpacket<payload>
        {
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);

    }
    auto api_input_history_limit_set         ()
    {
        log(prompt, "SetConsoleNumberOfCommands");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);

    }
    auto api_input_history_get_volume        ()
    {
        log(prompt, "GetConsoleCommandHistoryLength");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 count;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.count = 0; // Requires by the "doskey /history".

    }
    auto api_input_history_get               ()
    {
        log(prompt, "GetConsoleCommandHistory");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 bytes;
            }
            reply;
            struct
            {
                byte utf16;
            }
            input;
        };
        auto& packet = payload::cast(upload);
        packet.reply.bytes = 0;

    }
    auto api_input_history_info_get          ()
    {
        log(prompt, "GetConsoleHistory");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 limit;
                ui32 count;
                ui32 flags;
            }
            reply;
        };
        auto& packet = payload::cast(upload);
        packet.reply.flags = 0;
        packet.reply.limit = 0;
        packet.reply.count = 0;

    }
    auto api_input_history_info_set          ()
    {
        log(prompt, "SetConsoleHistory");
        struct payload : drvpacket<payload>
        {
            struct
            {
                ui32 limit;
                ui32 count;
                ui32 flags;
            }
            input;
        };
        auto& packet = payload::cast(upload);

    }

    using apis = std::vector<void(consrv::*)()>;
    using list = std::list<clnt>;
    using face = ui::face;
    using fire = netxs::os::io::fire;
    using xlat = netxs::sptr<decoder>;

    Term&       uiterm; // consrv: Terminal reference.
    fd_t&       condrv; // consrv: Console driver handle.
    bool&       io_log; // consrv: Stdio logging state.
    evnt        events; // consrv: Input event list.
    text        prompt; // consrv: Log prompt.
    list        joined; // consrv: Attached processes list.
    std::thread server; // consrv: Main thread.
    std::thread window; // consrv: Win32 window message loop.
    HWND        winhnd; // consrv: Win32 window handle.
    fire        signal; // consrv: Win32 window message loop unblocker.
    cdrw        answer; // consrv: Reply cue.
    task        upload; // consrv: Console driver request.
    text        buffer; // consrv: Temp buffer.
    text        toUTF8; // consrv: Buffer for UTF-16-UTF-8 conversion.
    text        toANSI; // consrv: Buffer for ANSICP-UTF-8 conversion.
    wide        toWIDE; // consrv: Buffer for UTF-8-UTF-16 conversion.
    rich        filler; // consrv: Buffer for filling operations.
    apis        apimap; // consrv: Fx reference.
    ui32        inpmod; // consrv: Events mode flag set.
    ui32        outmod; // consrv: Scrollbuffer mode flag set.
    face        mirror; // consrv: Viewport bitmap buffer.
    bool        impcls; // consrv: Implicit scroll buffer clearing detection.
    para        celler; // consrv: Buffer for converting raw text to cells.
    xlat        inpenc; // consrv: Current code page decoder for input stream.
    xlat        outenc; // consrv: Current code page decoder for output stream.

    void start()
    {
        reset();
        events.reset();
        signal.flush();
        window = std::thread{ [&]
        {
            auto wndname = text{ "vtmConsoleWindowClass" };
            auto wndproc = [](auto hwnd, auto uMsg, auto wParam, auto lParam)
            {
                ok<faux>(!debugmode, netxs::prompt::win32, "GUI message: hwnd=", utf::to_hex_0x(hwnd), " uMsg=", utf::to_hex_0x(uMsg), " wParam=", utf::to_hex_0x(wParam), " lParam=", utf::to_hex_0x(lParam));
                switch (uMsg)
                {
                    case WM_CREATE: break;
                    case WM_DESTROY: ::PostQuitMessage(0); break;
                    case WM_CLOSE: //todo revise (see taskkill /pid <processID>)
                    default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
                }
                return (LRESULT) NULL;
            };
            auto wnddata = WNDCLASSEXA
            {
                .cbSize        = sizeof(WNDCLASSEX),
                .lpfnWndProc   = wndproc,
                .lpszClassName = wndname.c_str(),
            };
            if (ok(::RegisterClassExA(&wnddata) || os::error() == ERROR_CLASS_ALREADY_EXISTS, "unexpected result from ::RegisterClassExA()")
               && (winhnd = ::CreateWindowExA(0, wndname.c_str(), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))
            {
                auto next = MSG{};
                while (next.message != WM_QUIT)
                {
                    auto abort = fd_t{ signal };
                    if (auto yield = ::MsgWaitForMultipleObjects(1, &abort, FALSE, INFINITE, QS_ALLINPUT);
                             yield == WAIT_OBJECT_0)
                    {
                        ::DestroyWindow(winhnd);
                        return;
                    }
                    while (::PeekMessageA(&next, NULL, 0, 0, PM_REMOVE) && next.message != WM_QUIT)
                    {
                        ::DispatchMessageA(&next);
                    }
                }
            }
            else
            {
                os::fail(prompt, "Failed to create win32 window object");
                winhnd = reinterpret_cast<HWND>(-1);
                return;
            }
        }};
        while (!winhnd) // Waiting for a win32 window to be created.
        {
            std::this_thread::yield();
        }
        server = std::thread{ [&]
        {
            while (condrv != os::invalid_fd)
            {
                auto rc = nt::ioctl(nt::console::op::read_io, condrv, answer, upload);
                answer = { .taskid = upload.taskid, .status = nt::status::success };
                switch (rc)
                {
                    case ERROR_SUCCESS:
                    {
                        if (upload.fxtype == nt::console::fx::subfx)
                        {
                            upload.fxtype = upload.callfx / 0x55555 + upload.callfx;
                            answer.buffer = upload.argbuf;
                            answer.length = upload.arglen;
                            answer._pad_1 = upload.arglen + sizeof(ui32) * 2 /*sizeof(drvpacket payload)*/;
                        }
                        auto proc = apimap[upload.fxtype & 255];
                        uiterm.update([&]
                        {
                            auto lock = std::lock_guard{ events.locker };
                            (this->*proc)();
                        });
                        break;
                    }
                    case ERROR_IO_PENDING:         log(prompt, "Operation has not completed"); ::WaitForSingleObject(condrv, 0); break;
                    case ERROR_PIPE_NOT_CONNECTED: log(prompt, "Client disconnected"); return;
                    default:                       log(prompt, "Unexpected nt::ioctl result ", rc); break;
                }
            }
            log(prompt, "Server thread ended");
        }};
    }
    void resize(twod const& newsize)
    {
        events.winsz(newsize); // We inform about viewport resize only.
    }
    void stop()
    {
        events.stop();
        signal.reset();
        if (window.joinable()) window.join();
        if (server.joinable()) server.join();
        log(prompt, "Console API server shut down");
    }
    template<class T>
    auto size_check(T upto, T from)
    {
        auto test = upto >= from;
        if (!test)
        {
            log("\tabort: negative size");
            answer.status = nt::status::unsuccessful;
            return T{};
        }
        return upto - from;
    }
    void reset()
    {
        inpmod = nt::console::inmode::preprocess & inpmod // Keep the `preprocess` mode, since WSL depends on it (e.g. Ctrl+C after reset in WSL).
               | nt::console::inmode::cooked
               | nt::console::inmode::echo
             //| nt::console::inmode::mouse // Should be disabled in order to selection mode be enabled on startup.
               | nt::console::inmode::insert
               | nt::console::inmode::vt & inpmod // Keep the `vt` mode, since WSL depends on it (e.g. ArrowKeys after reset in WSL).
               | nt::console::inmode::quickedit;
        outmod = nt::console::outmode::preprocess
               | nt::console::outmode::wrap_at_eol
               | nt::console::outmode::vt;
        if constexpr (isreal())
        {
            uiterm.normal.set_autocr(!(outmod & nt::console::outmode::no_auto_cr));
            if (inpmod & nt::console::inmode::mouse)
            {
                uiterm.mtrack.enable (decltype(uiterm.mtrack)::negative_args);
                uiterm.mtrack.setmode(decltype(uiterm.mtrack)::w32);
            }
        }
    }

    consrv(Term& uiterm, fd_t& condrv)
        : uiterm{ uiterm },
          condrv{ condrv },
          io_log{ uiterm.io_log },
          events{ *this  },
          impcls{ faux   },
          answer{        },
          winhnd{        },
          inpmod{ nt::console::inmode::preprocess },
          outmod{        },
          prompt{ utf::concat(netxs::prompt::win32) },
          inpenc{ std::make_shared<decoder>(*this, os::codepage) },
          outenc{ inpenc }
    {
        using _ = consrv;
        apimap.resize(0xFF, &_::api_unsupported);
        apimap[0x38] = &_::api_system_langid_get;
        apimap[0x91] = &_::api_system_mouse_buttons_get_count;
        apimap[0x01] = &_::api_process_attach;
        apimap[0x02] = &_::api_process_detach;
        apimap[0xB9] = &_::api_process_enlist;
        apimap[0x03] = &_::api_process_create_handle;
        apimap[0x04] = &_::api_process_delete_handle;
        apimap[0x30] = &_::api_process_codepage_get;
        apimap[0x64] = &_::api_process_codepage_set;
        apimap[0x31] = &_::api_process_mode_get;
        apimap[0x32] = &_::api_process_mode_set;
        apimap[0x06] = &_::api_events_read_as_text<true>;
        apimap[0x35] = &_::api_events_read_as_text;
        apimap[0x08] = &_::api_events_clear;
        apimap[0x63] = &_::api_events_clear;
        apimap[0x33] = &_::api_events_count_get;
        apimap[0x34] = &_::api_events_get;
        apimap[0x70] = &_::api_events_add;
        apimap[0x61] = &_::api_events_generate_ctrl_event;
        apimap[0x05] = &_::api_scrollback_write_text<true>;
        apimap[0x36] = &_::api_scrollback_write_text;
        apimap[0x72] = &_::api_scrollback_write_data;
        apimap[0x71] = &_::api_scrollback_write_block;
        apimap[0x6D] = &_::api_scrollback_attribute_set;
        apimap[0x60] = &_::api_scrollback_fill;
        apimap[0x6F] = &_::api_scrollback_read_data;
        apimap[0x73] = &_::api_scrollback_read_block;
        apimap[0x62] = &_::api_scrollback_set_active;
        apimap[0x6A] = &_::api_scrollback_cursor_coor_set;
        apimap[0x65] = &_::api_scrollback_cursor_info_get;
        apimap[0x66] = &_::api_scrollback_cursor_info_set;
        apimap[0x67] = &_::api_scrollback_info_get;
        apimap[0x68] = &_::api_scrollback_info_set;
        apimap[0x69] = &_::api_scrollback_size_set;
        apimap[0x6B] = &_::api_scrollback_viewport_get_max_size;
        apimap[0x6E] = &_::api_scrollback_viewport_set;
        apimap[0x6C] = &_::api_scrollback_scroll;
        apimap[0xB8] = &_::api_scrollback_selection_info_get;
        apimap[0x74] = &_::api_window_title_get;
        apimap[0x75] = &_::api_window_title_set;
        apimap[0x93] = &_::api_window_font_size_get;
        apimap[0x94] = &_::api_window_font_get;
        apimap[0xBC] = &_::api_window_font_set;
        apimap[0xA1] = &_::api_window_mode_get;
        apimap[0x9D] = &_::api_window_mode_set;
        apimap[0xAF] = &_::api_window_handle_get;
        apimap[0xAC] = &_::api_window_xkeys;
        apimap[0xA3] = &_::api_alias_get;
        apimap[0xA2] = &_::api_alias_add;
        apimap[0xA5] = &_::api_alias_exes_get_volume;
        apimap[0xA7] = &_::api_alias_exes_get;
        apimap[0xA4] = &_::api_aliases_get_volume;
        apimap[0xA6] = &_::api_aliases_get;
        apimap[0xA8] = &_::api_input_history_clear;
        apimap[0xA9] = &_::api_input_history_limit_set;
        apimap[0xAA] = &_::api_input_history_get_volume;
        apimap[0xAB] = &_::api_input_history_get;
        apimap[0xBA] = &_::api_input_history_info_get;
        apimap[0xBB] = &_::api_input_history_info_set;
    }
};

#undef log