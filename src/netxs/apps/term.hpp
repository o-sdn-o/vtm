// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

#include "../desktopio/application.hpp"
#include "../desktopio/terminal.hpp"

namespace netxs::events::userland
{
    struct terminal
    {
        EVENTPACK( terminal, ui::e2::extra::slot3 )
        {
            EVENT_XS( cmd    , si32 ),
            GROUP_XS( preview, si32 ),
            GROUP_XS( release, si32 ),
            GROUP_XS( data   , si32 ),
            GROUP_XS( search , input::hids ),

            SUBSET_XS( preview )
            {
                EVENT_XS( align    , si32 ),
                EVENT_XS( wrapln   , si32 ),
                EVENT_XS( io_log   , bool ),
                EVENT_XS( cwdsync  , bool ),
                EVENT_XS( rawkbd   , bool ),
                GROUP_XS( selection, si32 ),
                GROUP_XS( colors   , argb ),

                SUBSET_XS( selection )
                {
                    EVENT_XS( mode, si32 ),
                    EVENT_XS( box , si32 ),
                    EVENT_XS( shot, si32 ),
                };
                SUBSET_XS( colors )
                {
                    EVENT_XS( bg, argb ),
                    EVENT_XS( fg, argb ),
                };
            };
            SUBSET_XS( release )
            {
                EVENT_XS( align    , si32 ),
                EVENT_XS( wrapln   , si32 ),
                EVENT_XS( io_log   , bool ),
                EVENT_XS( cwdsync  , bool ),
                EVENT_XS( rawkbd   , bool ),
                GROUP_XS( selection, si32 ),
                GROUP_XS( colors   , argb ),

                SUBSET_XS( selection )
                {
                    EVENT_XS( mode, si32 ),
                    EVENT_XS( box , si32 ),
                    EVENT_XS( shot, si32 ),
                };
                SUBSET_XS( colors )
                {
                    EVENT_XS( bg, argb ),
                    EVENT_XS( fg, argb ),
                };
            };
            SUBSET_XS( data )
            {
                EVENT_XS( in     , view        ),
                EVENT_XS( out    , view        ),
                EVENT_XS( paste  , input::hids ),
                EVENT_XS( copy   , input::hids ),
                EVENT_XS( prnscrn, input::hids ),
            };
            SUBSET_XS( search )
            {
                EVENT_XS( forward, input::hids ),
                EVENT_XS( reverse, input::hids ),
                EVENT_XS( status , si32        ),
            };
        };
    };
}

// term: Teletype Console.
namespace netxs::app::teletype
{
    static constexpr auto id = "teletype";
    static constexpr auto name = "Teletype Console";
}
// term: Terminal Console.
namespace netxs::app::terminal
{
    static constexpr auto id = "terminal";
    static constexpr auto name = "Terminal Console";

    namespace attr
    {
        static constexpr auto cwdsync   = "/config/terminal/cwdsync";
        static constexpr auto borders   = "/config/terminal/border";
    }

    using events = netxs::events::userland::terminal;

    namespace
    {
        using namespace app::shared;
        auto _update(ui::item& boss, menu::item& item)
        {
            auto& look = item.views[item.taken];
            boss.bell::signal(tier::release, e2::data::utf8,              look.label);
            boss.bell::signal(tier::preview, e2::form::prop::ui::tooltip, look.tooltip);
            boss.reflow();
        }
        auto _update_gear(ui::item& boss, menu::item& item, hids& gear)
        {
            auto& look = item.views[item.taken];
            gear.set_tooltip(look.tooltip, true);
            _update(boss, item);
        }
        auto _update_to(ui::item& boss, menu::item& item, ui64 i)
        {
            item.select(i);
            _update(boss, item);
        }
        template<bool AutoUpdate = faux, class P>
        auto _submit(ui::item& boss, menu::item& item, P proc)
        {
            if (item.type == menu::type::Repeat)
            {
                auto& tick = boss.plugins<pro::timer>();
                boss.LISTEN(tier::release, hids::events::mouse::button::down::left, gear, -, (proc))
                {
                    if (item.views.size())
                    {
                        item.taken = (item.taken + 1) % item.views.size();
                    }
                    if (gear.capture(boss.id))
                    {
                        proc(boss, item, gear);
                        tick.actify(0, skin::globals().repeat_delay, [&, proc](auto)
                        {
                            proc(boss, item, gear);
                            tick.actify(1, skin::globals().repeat_rate, [&, proc](auto)
                            {
                                proc(boss, item, gear);
                                return true; // Repeat forever.
                            });
                            return faux; // One shot call (first).
                        });
                        gear.dismiss(true);
                    }
                    if (item.views.size())
                    {
                        _update_gear(boss, item, gear);
                    }
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::up::left, gear)
                {
                    tick.pacify();
                    gear.setfree();
                    gear.dismiss(true);
                    if (item.views.size() && item.taken)
                    {
                        item.taken = 0;
                        _update_gear(boss, item, gear);
                    }
                };
                boss.LISTEN(tier::release, e2::form::state::mouse, active)
                {
                    if (!active && tick)
                    {
                        tick.pacify();
                        if (item.views.size() && item.taken)
                        {
                            item.taken = 0;
                            _update(boss, item);
                        }
                    }
                };
            }
            else
            {
                boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, -, (proc))
                {
                    proc(boss, item, gear);
                    if constexpr (AutoUpdate)
                    {
                        if (item.type == menu::type::Option) _update_gear(boss, item, gear);
                    }
                    gear.nodbl = true;
                };
            }
        };
        auto construct_menu(xmls& config)
        {
            //auto highlight_color = skin::color(tone::highlight);
            //auto c3 = highlight_color;

            using term = ui::term;
            using preview = terminal::events::preview;
            using release = terminal::events::release;

            static const auto proc_map = menu::action_map_t
            {
                { term::action::Noop, [](ui::item& /*boss*/, menu::item& /*item*/){ } }, 
                { term::action::ExclusiveKeyboardMode, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take_or<bool>(utf8, faux); });
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::rawkbd, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::rawkbd, state)
                    {
                        _update_to(boss, item, state);
                    };
                }},
                { term::action::TerminalWrapMode, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value() ? wrap::on : wrap::off; });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::wrapln, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::wrapln, wrapln)
                    {
                        _update_to(boss, item, wrapln);
                    };
                }},
                { term::action::TerminalAlignMode, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return netxs::get_or(xml::options::align, utf8, bias::left); });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::align, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::align, align)
                    {
                        _update_to(boss, item, align);
                    };
                }},
                { term::action::TerminalFindPrev, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value(); });
                    _submit(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::search::reverse, gear);
                    });
                    boss.LISTEN(tier::anycast, terminal::events::search::status, status)
                    {
                        _update_to(boss, item, (status & 2) ? 1 : 0);
                    };
                }},
                { term::action::TerminalFindNext, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value(); });
                    _submit(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::search::forward, gear);
                    });
                    boss.LISTEN(tier::anycast, terminal::events::search::status, status)
                    {
                        _update_to(boss, item, (status & 1) ? 1 : 0);
                    };
                }},
                { term::action::TerminalOutput, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::data::in, view{ item.views[item.taken].data });
                    });
                }},
                { term::action::TerminalSendKey, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::data::out, view{ item.views[item.taken].data });
                    });
                }},
                { term::action::TerminalQuit, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::cmd, ui::term::commands::ui::commands::sighup);
                    });
                }},
                { term::action::TerminalFullscreen, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.base::riseup(tier::preview, e2::form::size::enlarge::fullscreen, gear);
                    });
                }},
                { term::action::TerminalMaximize, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.base::riseup(tier::preview, e2::form::size::enlarge::maximize, gear);
                    });
                }},
                { term::action::TerminalMinimize, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.base::riseup(tier::release, e2::form::size::minimize, gear);
                    });
                }},
                { term::action::TerminalRestart, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::cmd, ui::term::commands::ui::commands::restart);
                    });
                }},
                { term::action::TerminalUndo, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::cmd, ui::term::commands::ui::commands::undo);
                    });
                }},
                { term::action::TerminalRedo, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::cmd, ui::term::commands::ui::commands::redo);
                    });
                }},
                { term::action::TerminalClipboardPaste, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::data::paste, gear);
                    });
                }},
                { term::action::TerminalClipboardWipe, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& /*boss*/, auto& /*item*/, auto& gear)
                    {
                        gear.clear_clipboard();
                    });
                }},
                { term::action::TerminalClipboardCopy, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::data::copy, gear);
                    });
                }},
                { term::action::TerminalClipboardFormat, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return netxs::get_or(xml::options::format, utf8, mime::disabled); });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::selection::mode, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::selection::mode, mode)
                    {
                        _update_to(boss, item, mode);
                    };
                }},
                { term::action::TerminalSelectionOneShot, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return netxs::get_or(xml::options::format, utf8, mime::disabled); });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::selection::shot, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::selection::shot, mode)
                    {
                        _update_to(boss, item, mode);
                    };
                }},
                { term::action::TerminalSelectionRect, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value(); });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::selection::box, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::selection::box, selbox)
                    {
                        _update_to(boss, item, selbox);
                    };
                }},
                { term::action::TerminalSelectionCancel, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::cmd, ui::term::commands::ui::commands::deselect);
                    });
                }},
                { term::action::TerminalViewportCopy, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& gear)
                    {
                        boss.bell::signal(tier::anycast, terminal::events::data::prnscrn, gear);
                    });
                }},
                { term::action::TerminalScrollViewportByPage, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        auto delta = xml::take_or<twod>(item.views[item.taken].data, dot_00);
                        boss.bell::signal(tier::anycast, e2::form::upon::scroll::bypage::v, { .vector = delta });
                    });
                }},
                { term::action::TerminalScrollViewportByCell, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        auto delta = xml::take_or<twod>(item.views[item.taken].data, dot_00);
                        boss.bell::signal(tier::anycast, e2::form::upon::scroll::bystep::v, { .vector = delta });
                    });
                }},
                { term::action::TerminalScrollViewportToTop, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, e2::form::upon::scroll::to_top::y);
                    });
                }},
                { term::action::TerminalScrollViewportToEnd, [](ui::item& boss, menu::item& item)
                {
                    _submit<true>(boss, item, [](auto& boss, auto& /*item*/, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, e2::form::upon::scroll::to_end::y);
                    });
                }},
                { term::action::TerminalStdioLog, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value(); });
                    _submit<true>(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::io_log, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::io_log, state)
                    {
                        _update_to(boss, item, state);
                    };
                }},
                { term::action::TerminalCwdSync, [](ui::item& boss, menu::item& item)
                {
                    item.reindex([](auto& utf8){ return xml::take<bool>(utf8).value(); });
                    _submit(boss, item, [](auto& boss, auto& item, auto& /*gear*/)
                    {
                        boss.bell::signal(tier::anycast, preview::cwdsync, item.views[item.taken].value);
                    });
                    boss.LISTEN(tier::anycast, release::cwdsync, state)
                    {
                        _update_to(boss, item, state);
                    };
                }},
                { term::action::TerminalLogStart, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalLogPause, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalLogStop, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalLogAbort, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalLogRestart, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoRecStart, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoRecStop, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoRecPause, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoRecAbort, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoRecRestart, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoPlay, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoPause, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoStop, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoForward, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoBackward, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoHome, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
                { term::action::TerminalVideoEnd, [](ui::item& /*boss*/, menu::item& /*item*/)
                {

                }},
            };

            config.cd("/config/terminal", "/config/defapp");
            return menu::load(config, proc_map);
        }
    }

    auto ui_term_events = [](ui::term& boss, eccc& appcfg)
    {
        boss.LISTEN(tier::anycast, e2::form::proceed::quit::any, fast)
        {
            boss.bell::signal(tier::preview, e2::form::proceed::quit::one, fast);
        };
        boss.LISTEN(tier::preview, e2::form::proceed::quit::one, fast)
        {
            boss.close(fast);
        };
        boss.LISTEN(tier::anycast, terminal::events::cmd, cmd)
        {
            boss.exec_cmd(static_cast<ui::term::commands::ui::commands>(cmd));
        };
        boss.LISTEN(tier::anycast, terminal::events::data::in, data)
        {
            boss.data_in(data);
        };
        boss.LISTEN(tier::anycast, terminal::events::data::out, data)
        {
            boss.data_out(data);
        };
        //todo add color picker to the menu
        boss.LISTEN(tier::anycast, terminal::events::preview::colors::bg, bg)
        {
            boss.set_bg_color(bg);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::colors::fg, fg)
        {
            boss.set_fg_color(fg);
        };
        boss.LISTEN(tier::anycast, e2::form::prop::colors::any, clr)
        {
            auto deed = boss.bell::protos(tier::anycast);
                 if (deed == e2::form::prop::colors::bg.id) boss.bell::signal(tier::anycast, terminal::events::preview::colors::bg, clr);
            else if (deed == e2::form::prop::colors::fg.id) boss.bell::signal(tier::anycast, terminal::events::preview::colors::fg, clr);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::selection::mode, selmod)
        {
            boss.set_selmod(selmod);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::selection::shot, selmod)
        {
            boss.set_oneshot(selmod);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::selection::box, selbox)
        {
            boss.set_selalt(selbox);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::rawkbd, rawkbd)
        {
            boss.set_rawkbd(rawkbd + 1);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::wrapln, wrapln)
        {
            boss.set_wrapln(wrapln);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::io_log, state)
        {
            boss.set_log(state);
        };
        boss.LISTEN(tier::anycast, terminal::events::preview::align, align)
        {
            boss.set_align(align);
        };
        boss.LISTEN(tier::release, e2::form::upon::started, root, -, (appcfg))
        {
            if (root) // root is empty when d_n_d.
            {
                boss.start(appcfg);
            }
        };
        boss.LISTEN(tier::anycast, e2::form::upon::started, root)
        {
            boss.bell::signal(tier::release, e2::form::upon::started, root);
        };
        boss.LISTEN(tier::anycast, terminal::events::search::forward, gear)
        {
            boss.selection_search(gear, feed::fwd);
        };
        boss.LISTEN(tier::anycast, terminal::events::search::reverse, gear)
        {
            boss.selection_search(gear, feed::rev);
        };
        boss.LISTEN(tier::anycast, terminal::events::data::paste, gear)
        {
            boss.paste(gear);
        };
        boss.LISTEN(tier::anycast, terminal::events::data::copy, gear)
        {
            boss.copy(gear);
        };
        boss.LISTEN(tier::anycast, terminal::events::data::prnscrn, gear)
        {
            boss.prnscrn(gear);
        };
        boss.LISTEN(tier::anycast, e2::form::upon::scroll::any, i)
        {
            auto info = e2::form::upon::scroll::bypage::y.param();
            auto deed = boss.bell::protos(tier::anycast);
            boss.base::raw_riseup(tier::request, e2::form::upon::scroll::any.id, info);
            info.vector = i.vector;
            boss.base::raw_riseup(tier::preview, deed, info);
        };
    };
    auto build_teletype = [](eccc appcfg, xmls& config)
    {
        auto window_clr = skin::color(tone::window_clr);
        auto window = ui::cake::ctor()
            ->plugin<pro::focus>()
            ->invoke([&](auto& boss)
            {
                closing_on_quit(boss);
            });
        window//->plugin<pro::track>()
            //->plugin<pro::acryl>()
            ->plugin<pro::cache>();
        auto defclr = config.take("/config/terminal/colors/default", cell{}.fgc(whitelt).bgc(blackdk));
        auto layers = window->attach(ui::cake::ctor())
                            ->colors(window_clr)
                            ->limits(dot_11);
        auto scroll = layers->attach(ui::rail::ctor())
                            ->limits({ 10,1 }); // mc crashes when window is too small
        if (appcfg.cmd.empty()) appcfg.cmd = os::env::shell();//todo revise + " -i";
        auto term = scroll->attach(ui::term::ctor(config))
            ->plugin<pro::focus>(pro::focus::mode::focused)
            ->colors(defclr.fgc(), defclr.bgc())
            ->invoke([&](auto& boss)
            {
                ui_term_events(boss, appcfg);
            });
        layers->attach(app::shared::scroll_bars(scroll));
        return window;
    };
    auto build_terminal = [](eccc appcfg, xmls& config)
    {
        auto window_clr = skin::color(tone::window_clr);
        auto border = std::max(0, config.take(attr::borders, 0));
        auto borders = dent{ border, border, 0, 0 };
        auto menu_height = ptr::shared(0);
        auto gradient = [menu_height, borders, bground = core{}](face& parent_canvas, si32 /*param*/, base& /*boss*/) mutable
        {
            static constexpr auto grad_vsize = 32;
            auto full = parent_canvas.full();
            auto clip = parent_canvas.clip();
            auto region = full;
            if (region.size.x != bground.size().x)
            {
                auto spline = netxs::spline01{ -0.30f };
                auto mx = std::max(2, region.size.x);
                auto my = std::min(3, region.size.y);
                bground.size({ mx, my }, skin::color(tone::winfocus));
                auto it = bground.begin();
                for (auto y = 0.f; y < my; y++)
                {
                    auto y0 = (y + 1) / grad_vsize;
                    auto sy = spline(y0);
                    for (auto x = 0.f; x < mx; x++)
                    {
                        auto& c = it++->bgc();
                        auto mirror = x < mx / 2.f ? x : mx - x;
                        auto x0 = (mirror + 2) / (mx - 1.f);
                        auto sx = spline(x0);
                        auto xy = sy * sx;
                        c.chan.a = (byte)std::round(255.0 * (1.f - xy));
                    }
                }
            }
            auto menu_size = twod{ region.size.x, std::min(bground.size().y, *menu_height) };
            auto stat_size = twod{ region.size.x, 1 };
            // Menu background.
            auto dest = clip.trim({ region.coor, menu_size });
            parent_canvas.clip(dest);
            bground.move(region.coor);
            parent_canvas.plot(bground, cell::shaders::blend);
            // Hz scrollbar background.
            bground.step({ 0, region.size.y - 1 });
            parent_canvas.clip(clip);
            parent_canvas.plot(bground, cell::shaders::blend);
            // Left/right border background.
            auto color = bground[dot_00];
            full -= dent{ 0, 0, menu_size.y, stat_size.y };
            parent_canvas.cage(full, borders, cell::shaders::blend(color));
            // Restore clipping area.
            parent_canvas.clip(clip);
        };

        auto window = ui::cake::ctor();
        window->plugin<pro::focus>()
            //->plugin<pro::track>()
            //->plugin<pro::acryl>()
            ->plugin<pro::cache>()
            ->shader(gradient, e2::form::state::focus::count);

        auto object = window->attach(ui::fork::ctor(axis::Y));
        auto term_stat_area = object->attach(slot::_2, ui::fork::ctor(axis::Y))
            ->setpad(borders)
            ->invoke([&](auto& boss)
            {
                if (borders)
                boss.LISTEN(tier::release, e2::render::background::any, parent_canvas, -, (borders, window_clr)) // Shade left/right borders.
                {
                    auto full = parent_canvas.full();
                    parent_canvas.cage(full, borders, [&](cell& c){ c.fuse(window_clr); });
                };
            });
        auto layers = term_stat_area->attach(slot::_1, ui::cake::ctor())
                                    ->limits(dot_11);
        auto scroll = layers->attach(ui::rail::ctor()->smooth(faux));
        auto min_size = twod{ 12,1 }; // mc crashes when window is too small
        auto max_size = -dot_11;
        scroll->limits(min_size, max_size)
            ->invoke([](auto& boss)
            {
                boss.LISTEN(tier::preview, e2::form::prop::window::size, new_size)
                {
                    // Axis x/y (see XTWINOPS):
                    //   -1 -- preserve
                    //    0 -- maximize (toggle)
                    if (new_size == dot_00) // Toggle fullscreen terminal (only if it is focused by someone).
                    {
                        auto gates = boss.base::riseup(tier::request, e2::form::state::keybd::enlist);
                        if (gates.size())
                        if (auto gate_ptr = boss.bell::getref(gates.back()))
                        {
                            gate_ptr->bell::signal(tier::release, e2::form::proceed::onbehalf, [&](auto& gear)
                            {
                                boss.base::riseup(tier::preview, e2::form::size::enlarge::fullscreen, gear);
                            });
                        }
                    }
                    else if (boss.base::size() != new_size)
                    {
                        auto panel = boss.base::size();
                        new_size = new_size.less(dot_11, panel, std::max(dot_11, new_size));
                        auto warp = rect{ dot_00, new_size } - rect{ dot_00, panel };
                        boss.base::locked = faux; // Unlock resizing.
                        boss.base::resize(new_size);
                        boss.base::locked = true; // Lock resizing until reflow is complete.
                        boss.base::riseup(tier::preview, e2::form::layout::swarp, warp);
                    }
                };
                boss.LISTEN(tier::release, e2::area, new_area)
                {
                    boss.base::locked = faux; // Unlock resizing.
                };
            });

        if (appcfg.cmd.empty()) appcfg.cmd = os::env::shell();//todo revise + " -i";
        auto term = scroll->attach(ui::term::ctor(config))
            ->plugin<pro::focus>(pro::focus::mode::focused)
            ->invoke([&](auto& boss)
            {
                auto cwd_commands = config.take(attr::cwdsync, ""s);
                auto cwd_sync_ptr = ptr::shared<bool>();
                auto cwd_path_ptr = ptr::shared<os::fs::path>();
                auto& cwd_sync = *cwd_sync_ptr;
                auto& cwd_path = *cwd_path_ptr;
                boss.LISTEN(tier::preview, ui::term::events::toggle::cwdsync, state, -)
                {
                    boss.bell::signal(tier::anycast, terminal::events::preview::cwdsync, !cwd_sync);
                };
                boss.LISTEN(tier::anycast, terminal::events::preview::cwdsync, state, -, (cwd_commands))
                {
                    if (cwd_sync != state)
                    {
                        cwd_sync = state;
                        boss.bell::signal(tier::anycast, terminal::events::release::cwdsync, state);
                        if (cwd_sync)
                        {
                            auto cmd = cwd_commands;
                            utf::replace_all(cmd, "$P", ".");
                            boss.data_out(cmd); // Trigger command prompt reprint.
                        }
                    }
                };
                boss.LISTEN(tier::preview, e2::form::prop::cwd, path, -, (cwd_sync_ptr, cwd_path_ptr))
                {
                    if (cwd_sync)
                    {
                        boss.bell::expire(tier::preview, true);
                        cwd_path = path;
                    }
                };
                if (cwd_commands.size())
                {
                    boss.LISTEN(tier::anycast, e2::form::prop::cwd, path, -, (cwd_commands))
                    {
                        if (cwd_sync && path.size() && cwd_path != path)
                        {
                            cwd_path = path;
                            auto cwd = cwd_path.string();
                            if (cwd.find(' ') != text::npos) cwd = '\"' + cwd + '\"';
                            auto cmd = cwd_commands;
                            utf::replace_all(cmd, "$P", cwd);
                            boss.data_out(cmd);
                        }
                    };
                }
            });
        auto sb = layers->attach(ui::fork::ctor());
        auto vt = sb->attach(slot::_2, ui::grip<axis::Y>::ctor(scroll));
        static constexpr auto drawfx = [](auto& boss, auto& canvas, auto handle, auto /*object_len*/, auto handle_len, auto region_len, auto wide)
        {
            static auto box1 = "▄"sv;
            static auto box2 = ' ';
            auto window_clr = skin::color(tone::window_clr);
            auto term_bgc = boss.base::color().bgc();
            if (handle_len != region_len) // Show only if it is oversized.
            {
                if (wide) // Draw full scrollbar on mouse hover.
                {
                    canvas.fill([&](cell& c){ c.txt(box2).link(boss.bell::id).xlight().bgc().mix(window_clr.bgc()); });
                    canvas.fill(handle, [&](cell& c){ c.bgc().xlight(2); });
                }
                else
                {
                    canvas.fill([&](cell& c){ c.txt(box1).fgc(c.bgc()).bgc(term_bgc).fgc().mix(window_clr.bgc()); });
                    canvas.fill(handle, [&](cell& c){ c.link(boss.bell::id).fgc().xlight(2); });
                }
            }
            else canvas.fill([&](cell& c){ c.txt(box1).fgc(c.bgc()).bgc(term_bgc).fgc().mix(window_clr.bgc()); });
        };
        auto hz = term_stat_area->attach(slot::_2, ui::gripfx<axis::X, drawfx>::ctor(scroll))
            ->limits({ -1,1 }, { -1,1 })
            ->invoke([&](auto& boss)
            {
                boss.color(boss.color().bgc(term->color().bgc()));
                term->LISTEN(tier::release, e2::form::prop::filler, brush, -)
                {
                    boss.color(boss.color().bgc(brush.bgc()));
                };
            });

        auto [slot1, cover, menu_data] = construct_menu(config);
        auto menu = object->attach(slot::_1, slot1)
            ->colors(window_clr)
            ->invoke([&](auto& boss)
            {
                boss.LISTEN(tier::release, e2::area, new_area, -, (menu_height))
                {
                    *menu_height = new_area.size.y;
                };
            });
        cover->invoke([&, &slot1 = slot1](auto& boss) //todo clang 15.0.0 still disallows capturing structured bindings (wait for clang 16.0.0)
        {
            auto bar = cell{ "▀"sv }.link(slot1->id);
            auto term_bgc_ptr = ptr::shared(term->color().bgc());
            auto& term_bgc = *term_bgc_ptr;
            auto winsz = ptr::shared(dot_00);
            auto visible = ptr::shared(slot1->back() != boss.This());
            auto check_state = ptr::function([state = true, winsz, visible](base& boss) mutable
            {
                if (std::exchange(state, *visible || winsz->y != 1) != state)
                {
                    boss.base::riseup(tier::preview, e2::form::prop::ui::cache, state);
                }
            });
            boss.LISTEN(tier::release, e2::form::state::visible, menu_visible, -, (visible, check_state))
            {
                *visible = menu_visible;
                (*check_state)(boss);
            };
            boss.LISTEN(tier::anycast, e2::form::upon::resized, new_area, -, (winsz, check_state))
            {
                *winsz = new_area.size;
                (*check_state)(boss);
            };
            term->LISTEN(tier::release, e2::form::prop::filler, clr, -, (term_bgc_ptr))
            {
                term_bgc = clr.bgc();
            };
            boss.LISTEN(tier::release, e2::render::any, parent_canvas, -, (bar, winsz, term_bgc_ptr, borders))
            {
                auto full = parent_canvas.full();
                if (winsz->y != 1 && borders)
                {
                    parent_canvas.cage(full, borders, [&](cell& c){ c.txt(whitespace).link(bar); });
                    full -= borders;
                }
                auto bgc = winsz->y != 1 ? term_bgc : 0;
                parent_canvas.fill(full, [&](cell& c){ c.fgc(c.bgc()).bgc(bgc).txt(bar).link(bar); });
            };
        });

        term->attach_property(ui::term::events::colors::bg,      terminal::events::release::colors::bg)
            ->attach_property(ui::term::events::colors::fg,      terminal::events::release::colors::fg)
            ->attach_property(ui::term::events::selmod,          terminal::events::release::selection::mode)
            ->attach_property(ui::term::events::onesht,          terminal::events::release::selection::shot)
            ->attach_property(ui::term::events::selalt,          terminal::events::release::selection::box)
            ->attach_property(ui::term::events::rawkbd,          terminal::events::release::rawkbd)
            ->attach_property(ui::term::events::io_log,          terminal::events::release::io_log)
            ->attach_property(ui::term::events::layout::wrapln,  terminal::events::release::wrapln)
            ->attach_property(ui::term::events::layout::align,   terminal::events::release::align)
            ->attach_property(ui::term::events::search::status,  terminal::events::search::status)
            ->invoke([&](auto& boss)
            {
                ui_term_events(boss, appcfg);
            });
        return window;
    };

    app::shared::initialize teletype_builder{ app::teletype::id, build_teletype };
    app::shared::initialize terminal_builder{ app::terminal::id, build_terminal };
}