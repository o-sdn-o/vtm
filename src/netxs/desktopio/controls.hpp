// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

#include "input.hpp"

namespace netxs::ui
{
    // controls: UI extensions.
    namespace pro
    {
        // pro: Base class for extension/plugin.
        struct skill
        {
            base& boss;
            subs  memo;

            skill(base&&) = delete;
            skill(base& boss) : boss{ boss } { }
            virtual ~skill() = default; // In order to allow man derived class via base ptr.

            template<class T>
            struct socks
            {
                struct sock : public T
                {
                    id_t    id; // sock: Hids ID.
                    si32 count; // sock: Clients count.

                    sock(id_t ctrl)
                        :    id{ ctrl },
                          count{ 0    }
                    { }

                    operator bool () { return T::operator bool(); }
                };

                std::vector<sock> items; // sock: Registered hids.
                subs              token; // sock: Hids subscriptions.

                socks(base& boss)
                {
                    boss.LISTEN(tier::general, hids::events::die, gear, token)
                    {
                        del(gear);
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::hover::any, gear, token)
                    {
                             if (gear.cause == hids::events::mouse::hover::enter.id) add(gear);
                        else if (gear.cause == hids::events::mouse::hover::leave.id) dec(gear);
                    };
                }
                template<bool ConstWarn = true>
                auto& take(hids& gear)
                {
                    for (auto& item : items) // Linear search, because a few items.
                    {
                        if (item.id == gear.id) return item;
                    }

                    if constexpr (ConstWarn)
                    {
                        log(prompt::sock, "Access to unregistered input device, ", gear.id);
                    }

                    return items.emplace_back(gear.id);
                }
                template<class P>
                void foreach(P proc)
                {
                    for (auto& item : items)
                    {
                        if (item) proc(item);
                    }
                }
                void add(hids& gear)
                {
                    auto& item = take<faux>(gear);
                    ++item.count;
                }
                void dec(hids& gear)
                {
                    auto& item = take(gear);
                    if (--item.count < 1) // item.count could be equal to 0 due to unregistered access.
                    {
                        if (items.size() > 1) item = items.back(); // Remove an item without allocations.
                        items.pop_back();
                    }
                }
                void del(hids& gear)
                {
                    for (auto& item : items) // Linear search, because a few items.
                    {
                        if (item.id == gear.id)
                        {
                            if (items.size() > 1) item = items.back(); // Remove an item without allocations.
                            items.pop_back();
                            return;
                        }
                    }
                }
            };
        };

        // pro: Resizer.
        class sizer
            : public skill
        {
            using list = socks<netxs::misc::szgrips>;
            using skill::boss,
                  skill::memo;

            list items;
            dent outer;
            dent inner;
            bool alive; // pro::sizer: The sizer state.

        public:
            void props(dent outer_rect = { 2, 2, 1, 1 }, dent inner_rect = {})
            {
                outer = outer_rect;
                inner = inner_rect;
            }
            auto get_props()
            {
                return std::pair{ outer, inner };
            }

            sizer(base&&) = delete;
            sizer(base& boss, dent outer_rect = { 2, 2, 1, 1 }, dent inner_rect = {})
                : skill{ boss          },
                  items{ boss          },
                  outer{ outer_rect    },
                  inner{ inner_rect    },
                  alive{ true          }
            {
                // Drop it in favor of changing the cell size in GUI mode.
                //boss.LISTEN(tier::release, hids::events::mouse::scroll::act, gear, memo)
                //{
                //    if (gear.meta(hids::anyCtrl) && !gear.meta(hids::ScrlLock) && gear.whlsi)
                //    {
                //        auto& g = items.take(gear);
                //        if (!g.zoomon)// && g.inside)
                //        {
                //            g.zoomdt = {};
                //            g.zoomon = true;
                //            g.zoomsz = boss.base::area();
                //            g.zoomat = gear.coord;
                //            gear.capture(boss.id);
                //        }
                //        static constexpr auto warp = dent{ 2, 2, 1, 1 } * 2;
                //        //todo respect pivot
                //        auto prev = g.zoomdt;
                //        auto coor = boss.base::coor();
                //        auto deed = boss.bell::protos(tier::release);
                //        g.zoomdt += warp * gear.whlsi;
                //        auto viewport = gear.owner.bell::signal(tier::request, e2::form::prop::viewport);
                //        auto next = g.zoomsz + g.zoomdt;
                //        next.size = std::max(dot_00, next.size);
                //        next.trimby(viewport);
                //        auto step = boss.base::extend(next);
                //        if (!step.size) // Undo if can't zoom.
                //        {
                //            g.zoomdt = prev;
                //            boss.base::moveto(coor);
                //        }
                //    }
                //};
                boss.LISTEN(tier::release, e2::config::plugins::sizer::alive, state, memo)
                {
                    alive = state;
                };
                boss.LISTEN(tier::release, e2::postrender, canvas, memo)
                {
                    if (!alive) return;
                    auto area = canvas.full() + outer;
                    auto bord = outer - inner;
                    canvas.cage(area, bord, [&](cell& c)
                    {
                        c.link(boss.id);
                        if (c.bga() == 0) c.bga(1); // Active transparent.
                    });
                    items.foreach([&](auto& item)
                    {
                        item.draw(canvas, area, cell::shaders::xlight);
                    });
                };
                boss.LISTEN(tier::preview, e2::form::layout::swarp, warp, memo)
                {
                    auto area = boss.base::area();
                    auto next = area + warp;
                    boss.extend(next);
                };
                boss.LISTEN(tier::release, e2::config::plugins::sizer::outer, outer_rect, memo)
                {
                    outer = outer_rect;
                };
                boss.LISTEN(tier::release, e2::config::plugins::sizer::inner, inner_rect, memo)
                {
                    inner = inner_rect;
                };
                boss.LISTEN(tier::request, e2::config::plugins::sizer::inner, inner_rect, memo)
                {
                    inner_rect = inner;
                };
                boss.LISTEN(tier::request, e2::config::plugins::sizer::outer, outer_rect, memo)
                {
                    outer_rect = outer;
                };
                boss.LISTEN(tier::release, hids::events::mouse::move, gear, memo)
                {
                    auto& g = items.take(gear);
                    if (g.zoomon && !gear.meta(hids::anyCtrl))
                    {
                        g.zoomon = faux;
                        gear.setfree();
                    }
                    auto area = boss.base::area();
                    auto coor = area.coor + gear.coord;
                    if (g.calc(area, coor, outer, inner))
                    {
                        boss.base::deface(); // Deface only if mouse moved.
                    }
                };
                engage<hids::buttons::left>();
                engage<hids::buttons::leftright>();
            }
            // pro::sizer: Configuring the mouse button to operate.
            template<hids::buttons Button>
            void engage()
            {
                boss.bell::signal(tier::release, e2::form::draggable::_<Button>, true);
                boss.LISTEN(tier::release, e2::form::drag::start::_<Button>, gear, memo)
                {
                    auto area = boss.base::area();
                    auto coor = area.coor + gear.coord;
                    if (items.take(gear).grab(area, coor, outer))
                    {
                        gear.dismiss();
                        boss.bell::expire(tier::release); // To prevent d_n_d triggering.
                    }
                };
                boss.LISTEN(tier::release, e2::form::drag::pull::_<Button>, gear, memo)
                {
                    auto& g = items.take(gear);
                    if (g.seized)
                    {
                        auto zoom = gear.meta(hids::anyCtrl);
                        auto area = boss.base::area();
                        auto coor = area.coor + gear.coord;
                        auto [preview_area, size_delta] = g.drag(area, coor, outer, zoom);
                        boss.bell::signal(tier::preview, e2::area, preview_area);
                        if (auto dxdy = boss.sizeby(size_delta))
                        {
                            auto step = g.move(dxdy, zoom);
                            boss.moveby(step);
                            boss.bell::signal(tier::preview, e2::form::upon::changed, dxdy);
                        }
                        gear.dismiss();
                    }
                };
                boss.LISTEN(tier::release, e2::form::drag::cancel::_<Button>, gear, memo)
                {
                    items.take(gear).drop();
                };
                boss.LISTEN(tier::release, e2::form::drag::stop::_<Button>, gear, memo)
                {
                    items.take(gear).drop();
                    boss.bell::signal(tier::release, e2::form::upon::dragged, gear);
                };
            }
        };

        // pro: Moving by dragging support.
        class mover
            : public skill
        {
            struct sock
            {
                fp2d drag_origin; // sock: Drag origin.
                twod drag_center; // sock: Master center.
                void grab(base const& master, fp2d coord)
                {
                    drag_center = master.base::size() / 2;
                    drag_origin = coord - drag_center;
                }
                auto drag(base& master, fp2d coord)
                {
                    auto center = master.base::size() / 2;
                    auto delta = twod{ coord } - twod{ drag_origin } - center;
                    if (delta)
                    {
                        drag_origin += center - drag_center; // Keep origin tied to the master center.
                        drag_center = center;
                        master.base::moveby(delta);
                    }
                    return delta;
                }
            };

            using list = socks<sock>;
            using skill::boss,
                  skill::memo;

            list items;
            wptr dest_shadow;
            sptr dest_object;

        public:
            mover(base&&) = delete;
            mover(base& boss, sptr subject)
                : skill{ boss },
                  items{ boss },
                  dest_shadow{ subject }
            {
                engage<hids::buttons::left>();
            }
            mover(base& boss)
                : mover{ boss, boss.This() }
            { }
            // pro::mover: Configuring the mouse button to operate.
            template<hids::buttons Button>
            void engage()
            {
                boss.bell::signal(tier::release, e2::form::draggable::_<Button>, true);
                boss.LISTEN(tier::release, e2::form::drag::start::_<Button>, gear, memo)
                {
                    if ((dest_object = dest_shadow.lock()))
                    {
                        items.take(gear).grab(*dest_object, gear.coord);
                        gear.dismiss();
                    }
                };
                boss.LISTEN(tier::release, e2::form::drag::pull::_<Button>, gear, memo)
                {
                    if (dest_object)
                    {
                        if (auto delta = items.take(gear).drag(*dest_object, gear.coord))
                        {
                            dest_object->bell::signal(tier::preview, e2::form::upon::changed, delta);
                        }
                        gear.dismiss();
                    }
                };
                boss.LISTEN(tier::release, e2::form::drag::cancel::_<Button>, gear, memo)
                {
                    if (dest_object)
                    {
                        dest_object.reset();
                        gear.dismiss();
                    }
                };
                boss.LISTEN(tier::release, e2::form::drag::stop::_<Button>, gear, memo)
                {
                    if (dest_object)
                    {
                        dest_object->bell::signal(tier::release, e2::form::upon::dragged, gear);
                        dest_object.reset();
                        gear.dismiss();
                    }
                };
            }
        };

        // pro: Mouse cursor glow (it is needed to apply pro::acryl after it).
        class track
            : public skill
        {
            struct sock
            {
                twod cursor{}; // sock: Coordinates of the active cursor.
                bool inside{}; // sock: Is active.

                operator bool () { return inside; }
                auto calc(base const& master, twod curpos)
                {
                    auto area = rect{ dot_00, master.base::size() };
                    cursor = curpos;
                    inside = area.hittest(curpos);
                }
            };

            //using pool = std::list<id_t>;
            using list = socks<sock>;
            using skill::boss,
                  skill::memo;

            //pool focus; // track: Is keybd focused.
            list items; // track: .
            bool alive; // track: Is active.
/*
            void add_keybd(id_t gear_id)
            {
                if (gear_id)
                {
                    auto stat = focus.empty();
                    auto iter = std::find(focus.begin(), focus.end(), gear_id);
                    if (iter == focus.end())
                    {
                        focus.push_back(gear_id);
                        if (stat) boss.deface();
                    }
                }
            }
            void del_keybd(id_t gear_id)
            {
                if (gear_id)
                {
                    auto stat = focus.size();
                    auto iter = std::find(focus.begin(), focus.end(), gear_id);
                    if (iter != focus.end())
                    {
                        focus.erase(iter);
                        if (stat) boss.deface();
                    }
                }
            }
*/
            static auto& glow_overlay()
            {
                static auto bitmap = []
                {
                    auto r = 5;
                    auto blob = core{};
                    auto area = rect{ dot_00, dot_21 * (r * 2 + 1) };
                    auto func = netxs::spline01{ 0.65f };
                    blob.core::area(area, cell{}.bgc(0xFFffffff));
                    auto iter = blob.begin();
                    for (auto y = 0; y < area.size.y; y++)
                    {
                        auto y0 = (y - area.size.y / 2) / (area.size.y - 2 -     1.6f);
                        y0 *= y0;
                        for (auto x = 0; x < area.size.x; x++)
                        {
                            auto& c = iter++->bgc();
                            auto x0 = (x - area.size.x / 2) / (area.size.x - 4 - 2 * 1.6f);
                            auto dr = std::sqrt(x0 * x0 + y0);
                            if (dr > 1) c.chan.a = 0;
                            else
                            {
                                auto a = std::round(255.0 * func(1.0f - dr));
                                c.chan.a = (byte)std::clamp((si32)(a * 0.16f), 0, 255);
                            }
                        }
                    }
                    return blob;
                }();
                return bitmap;
            }

        public:
            track(base&&) = delete;
            track(base& boss)
                : skill{ boss },
                  items{ boss },
                  alive{ true }
            {
                // Keybd focus.
                //boss.LISTEN(tier::release, hids::events::focus::set::on, seed, memo)
                //{
                //    add_keybd(seed.gear_id);
                //};
                //boss.LISTEN(tier::release, hids::events::focus::set::off, seed, memo)
                //{
                //    del_keybd(seed.gear_id);
                //};
                //boss.LISTEN(tier::release, hids::events::die, gear, memo) // Gen by pro::focus.
                //{
                //    del_keybd(gear.id);
                //};
                // Mouse focus.
                //if (!skin::globals().tracking) return;
                boss.LISTEN(tier::release, hids::events::mouse::move, gear, memo)
                {
                    items.take(gear).calc(boss, gear.coord);
                };
                boss.LISTEN(tier::release, e2::render::background::prerender, parent_canvas, memo)
                {
                    if (!alive) return;
                    auto& glow = glow_overlay();
                    auto  coor = parent_canvas.coor();
                    auto  full = parent_canvas.full();
                    auto  base = full.coor - coor - glow.size() / 2;
                    items.foreach([&](sock& item)
                    {
                        glow.move(base + item.cursor);
                        parent_canvas.plot(glow, cell::shaders::blend);
                    });
                };
            }
        };

        // pro: Runtime animation support (time-based).
        class robot
            : public skill
        {
            using subs = std::map<id_t, hook>;
            using skill::boss;

            subs memo;

        public:
            using skill::skill; // Inherits ctors.

            // pro::robot: Every timer tick, yield the
            //             delta from the flow and, if delta,
            //             Call the proc (millisecond precision).
            template<class P, class S>
            void actify(id_t ID, S flow, P proc)
            {
                auto init = datetime::now();
                boss.LISTEN(tier::general, e2::timer::any, p, memo[ID], (ID, proc, flow, init))
                {
                    auto now = datetime::round<si32>(p - init);
                    if (auto data = flow(now))
                    {
                        static constexpr auto zero = std::decay_t<decltype(data.value())>{};
                        auto& v = data.value();
                        if (v != zero) proc(v);
                    }
                    else
                    {
                        pacify(ID);
                    }
                };
                boss.bell::signal(tier::release, e2::form::animate::start, ID);
            }
            // pro::robot: Optional proceed every timer tick,
            //             yield the delta from the flow and,
            //             if delta, Call the proc (millisecond precision).
            template<class P, class S>
            void actify(id_t ID, std::optional<S> flow, P proc)
            {
                if (flow)
                {
                    actify(ID, flow.value(), proc);
                }
            }
            template<class P, class S>
            void actify(S flow, P proc)
            {
                actify(bell::noid, flow, proc);
            }
            template<class P, class S>
            void actify(std::optional<S> flow, P proc)
            {
                if (flow)
                {
                    actify(bell::noid, flow.value(), proc);
                }
            }
            // pro::robot: Cancel tick activity.
            void pacify(id_t id = bell::noid)
            {
                if (id == bell::noid) memo.clear(); // Stop all animations.
                else                  memo.erase(id);
                boss.bell::signal(tier::release, e2::form::animate::stop, id);
            }
            // pro::robot: Check activity by id.
            bool active(id_t id)
            {
                return memo.contains(id);
            }
            // pro::robot: Check any activity.
            operator bool ()
            {
                return !memo.empty();
            }
        };

        // pro: Scheduler (timeout based).
        class timer
            : public skill
        {
            using subs = std::map<id_t, hook>;
            using skill::boss;

            subs memo;

        public:
            using skill::skill; // Inherits ctors.

            // pro::timer: Start countdown for specified ID.
            template<class P>
            void actify(id_t ID, span timeout, P lambda)
            {
                auto alarm = datetime::now() + timeout;
                boss.LISTEN(tier::general, e2::timer::any, now, memo[ID], (ID, timeout, lambda, alarm))
                {
                    if (now > alarm)
                    {
                        alarm = now + timeout;
                        if (!lambda(ID)) pacify(ID);
                    }
                };
            }
            // pro::timer: Start countdown.
            template<class P>
            void actify(span timeout, P lambda)
            {
                actify(bell::noid, timeout, lambda);
            }
            // pro::timer: Cancel timer ('id=noid' for all).
            void pacify(id_t id = bell::noid)
            {
                if (id == bell::noid) memo.clear(); // Stop all timers.
                else                  memo.erase(id);
                //boss.bell::signal(tier::release, e2::form::animate::stop, id);
            }
            // pro::timer: Check activity by id.
            bool active(id_t id)
            {
                return memo.contains(id);
            }
            // pro::timer: Check any activity.
            operator bool ()
            {
                return !memo.empty();
            }
        };

        // pro: Text cursor.
        class caret
            : public skill
        {
            using skill::boss,
                  skill::memo;

            subs conf; // caret: Configuration subscriptions.
            bool live; // caret: Should the cursor be drawn.
            bool done; // caret: Is the cursor already drawn.
            bool unfocused; // caret: Is the cursor suppressed (lost focus).
            rect body; // caret: Cursor position.
            si32 form; // caret: Cursor style (netxs::text_cursor).
            si32 original_form; // caret: Original cursor form.
            span step; // caret: Blink interval. span::zero() if steady.
            time next; // caret: Time of next blinking.
            cell mark; // caret: Cursor brush.

        public:
            caret(base&&) = delete;
            caret(base& boss, bool visible = faux, si32 cursor_style = text_cursor::I_bar, twod position = dot_00, span freq = skin::globals().blink_period, cell default_color = cell{})
                : skill{ boss },
                   live{ faux },
                   done{ faux },
                   unfocused{ true },
                   body{ position, dot_11 }, // Cursor is always one cell size (see the term::scrollback definition).
                   form{ cursor_style },
                   original_form{ cursor_style },
                   step{ freq },
                   mark{ default_color }
            {
                boss.LISTEN(tier::release, e2::form::state::focus::count, count, conf)
                {
                    unfocused = !count;
                };
                boss.LISTEN(tier::request, e2::config::cursor::blink, req_step, conf)
                {
                    req_step = step;
                };
                boss.LISTEN(tier::request, e2::config::cursor::style, req_style, conf)
                {
                    req_style = form;
                };
                boss.LISTEN(tier::general, e2::config::cursor::blink, new_step, conf)
                {
                    blink_period(new_step);
                };
                boss.LISTEN(tier::preview, e2::config::cursor::blink, new_step, conf)
                {
                    blink_period(new_step);
                };
                boss.LISTEN(tier::general, e2::config::cursor::style, new_style, conf)
                {
                    style(new_style);
                };
                boss.LISTEN(tier::preview, e2::config::cursor::style, new_style, conf)
                {
                    style(new_style);
                };
                if (visible) show();
            }

            operator bool () const { return memo.count(); }

            // pro::caret: Set cursor background color.
            void bgc(argb c)
            {
                if (mark.bgc() != c)
                {
                    hide();
                    mark.bgc(c);
                    show();
                }
            }
            // pro::caret: Get cursor background color.
            auto bgc()
            {
                return mark.bgc();
            }
            // pro::caret: Set cursor color.
            void color(cell c)
            {
                if (mark != c)
                {
                    hide();
                    mark = c;
                    show();
                }
            }
            // pro::caret: Set cursor style.
            void style(si32 new_form)
            {
                if (form != new_form)
                {
                    hide();
                    form = new_form;
                    show();
                }
            }
            // pro::caret: Set blink period.
            void blink_period(span new_step = skin::globals().blink_period)
            {
                auto changed = (step == span::zero()) != (new_step == span::zero());
                step = new_step;
                if (changed)
                {
                    hide();
                    show();
                }
            }
            void decscusr(si32 mode)
            {
                switch (mode)
                {
                    case 1: // n = 1  blinking box (default)
                        blink_period();
                        style(text_cursor::block);
                        break;
                    case 2: // n = 2  steady box
                        blink_period(span::zero());
                        style(text_cursor::block);
                        break;
                    case 3: // n = 3  blinking underline
                        blink_period();
                        style(text_cursor::underline);
                        break;
                    case 4: // n = 4  steady underline
                        blink_period(span::zero());
                        style(text_cursor::underline);
                        break;
                    case 0: // n = 0  blinking I-bar
                    case 5: // n = 5  blinking I-bar
                        blink_period();
                        style(text_cursor::I_bar);
                        break;
                    case 6: // n = 6  steady I-bar
                        blink_period(span::zero());
                        style(text_cursor::I_bar);
                        break;
                    default:
                        log(prompt::term, "Unsupported cursor style requested, ", mode);
                        break;
                }
            }
            void toggle()
            {
                if (original_form == text_cursor::underline) style(form != text_cursor::block ? text_cursor::block : text_cursor::underline);
                else                                         style(form != text_cursor::block ? text_cursor::block : text_cursor::I_bar);
                reset();
            }
            // pro::caret: Set cursor position.
            void coor(twod coor)
            {
                if (body.coor != coor)
                {
                    reset();
                    body.coor = coor;
                }
            }
            // pro::caret: Get cursor position.
            auto& coor() const
            {
                return body.coor;
            }
            // pro::caret: Get cursor style.
            auto style() const
            {
                return std::pair{ form, !!(*this) };
            }
            // pro::caret: Force to redraw cursor.
            void reset()
            {
                if (step != span::zero())
                {
                    live = faux;
                    next = {};
                }
            }
            // pro::caret: Enable cursor.
            void show()
            {
                if (!*this)
                {
                    done = faux;
                    auto blinking = step != span::zero();
                    if (blinking)
                    {
                        live = faux;
                        boss.LISTEN(tier::general, e2::timer::tick, timestamp, memo)
                        {
                            if (timestamp > next)
                            {
                                next = timestamp + step;
                                live = !live;
                                boss.deface(body);
                            }
                        };
                    }
                    else
                    {
                        live = true;
                    }
                    boss.LISTEN(tier::release, e2::postrender, canvas, memo)
                    {
                        done = true;
                        auto blinking = step != span::zero();
                        auto visible = (unfocused == blinking) || (blinking && live);
                        auto clip = canvas.core::clip();
                        auto area = clip.trim(body);
                        if (area && form != text_cursor::I_bar)
                        {
                            auto& test = canvas.peek(body.coor);
                            //todo >2x1 matrix support
                            auto [w, h, x, y] = test.whxy();
                            if (w == 2 && x == 1) // Extend cursor to adjacent halves.
                            {
                                if (clip.hittest(body.coor + dot_10))
                                {
                                    auto& next = canvas.peek(body.coor + dot_10);
                                    auto [nw, nh, nx, ny] = next.whxy();
                                    if (nw == 2 && nx == 2 && test.same_txt(next))
                                    {
                                        area.size.x++;
                                    }
                                }
                            }
                            else if (w == 2 && x == 2)
                            {
                                if (clip.hittest(body.coor - dot_10))
                                {
                                    auto& prev = canvas.peek(body.coor - dot_10);
                                    auto [pw, ph, px, py] = prev.whxy();
                                    if (pw == 2 && px == 1 && test.same_txt(prev))
                                    {
                                        area.size.x++;
                                        area.coor.x--;
                                    }
                                }
                            }
                        }
                        if (visible)
                        {
                            if (area)
                            {
                                canvas.fill(area, [&](auto& c){ c.set_cursor(form, mark); });
                            }
                            else if (area.size.y)
                            {
                                auto chr = area.coor.x ? '>' : '<';
                                area.coor.x -= area.coor.x ? 1 : 0;
                                area.size.x = 1;
                                canvas.fill(area, [&](auto& c){ c.txt(chr).fgc(cell::shaders::contrast.invert(c.bgc())).cur(text_cursor::none); });
                            }
                        }
                        else
                        {
                            if (area)
                            {
                                canvas.fill(area, [&](auto& c){ c.cur(text_cursor::none); });
                            }
                            else if (area.size.y)
                            {
                                area.size.x = 1;
                                area.coor.x -= area.coor.x ? 1 : 0;
                                canvas.fill(area, [&](auto& c){ c.cur(text_cursor::none); });
                            }
                        }
                    };
                }
            }
            // pro::caret: Disable cursor.
            void hide()
            {
                if (*this)
                {
                    memo.clear();
                    if (done)
                    {
                        boss.deface(body);
                        done = faux;
                    }
                }
            }
        };

        // pro: Title/footer support.
        class title
            : public skill
        {
            using skill::boss,
                  skill::memo;

        public:
            page head_page; // title: Owner's caption header.
            page foot_page; // title: Owner's caption footer.
            escx head_foci; // title: Original header + foci status.
            text head_text; // title: Original header.
            text foot_text; // title: Original footer.
            twod head_size; // title: Header page size.
            twod foot_size; // title: Footer page size.
            bool head_live; // title: Handle header events.
            bool foot_live; // title: Handle footer events.
            flow ooooooooo; // title: .

            struct user
            {
                id_t gear_id;
                text icon;
            };
            std::list<user> user_icon;

            bool live = true; // title: Title visibility.

            //todo use face::calc_page_height
            auto recalc(page& object, twod& size)
            {
                auto cp = dot_00;
                ooooooooo.flow::reset();
                ooooooooo.flow::size(size);
                auto publish = [&](auto const& combo)
                {
                    cp = ooooooooo.flow::print(combo);
                };
                object.stream(publish);
                size.y = ooooooooo.flow::minmax().size.y;
                return cp;
            }
            void recalc(twod new_size)
            {
                head_size = new_size;
                foot_size = new_size;
                if (head_live) recalc(head_page, head_size);
                if (foot_live) recalc(foot_page, foot_size);
            }
            void header(view newtext)
            {
                head_text = newtext;
                rebuild();
            }
            void footer(view newtext)
            {
                foot_text = newtext;
                foot_page = foot_text;
                recalc(foot_page, foot_size);
                boss.bell::signal(tier::release, e2::form::prop::ui::footer, foot_text);
            }
            void rebuild()
            {
                head_foci = head_text;
                if (user_icon.size())
                {
                    head_foci.add(text(user_icon.size() * 2, '\0')); // Reserv space for focus markers.
                    //if (head_live) // Add a new line if there is no space for focus markers.
                    //{
                    //    head_page = head_foci;
                    //    auto cp = recalc(head_page, head_size);
                    //    if (cp.x + user_icon.size() * 2 - 1 < head_size.x) head_foci.eol();
                    //}
                    head_foci.nop().pushsgr().chx(0).jet(bias::right);
                    for (auto& gear : user_icon)
                    {
                        head_foci.add(gear.icon);
                    }
                    head_foci.nop().popsgr();
                }
                if (head_live)
                {
                    head_page = head_foci;
                    recalc(head_page, head_size);
                }
                boss.bell::signal(tier::release, e2::form::prop::ui::header, head_text);
                boss.bell::signal(tier::release, e2::form::prop::ui::title , head_foci);
            }

            title(base&&) = delete;
            title(base& boss, view title = {}, view foots = {}, bool visible = true,
                                                                bool on_header = true,
                                                                bool on_footer = true)
                : skill{ boss },
                  head_live{ on_header },
                  foot_live{ on_footer },
                  live{ visible }
            {
                head_text = title;
                foot_text = foots;
                head_page = head_text;
                foot_page = foot_text;
                boss.LISTEN(tier::anycast, e2::form::upon::started, root, memo, (subs_ptr = ptr::shared<subs>()))
                {
                    if (head_live) header(head_text);
                    if (foot_live) footer(foot_text);
                    subs_ptr->clear();
                    if (auto focusable_parent = boss.base::riseup(tier::request, e2::config::plugins::focus::owner))
                    {
                        focusable_parent->LISTEN(tier::release, e2::form::state::focus::on, gear_id, *subs_ptr)
                        {
                            if (!gear_id) return;
                            auto iter = std::find_if(user_icon.begin(), user_icon.end(), [&](auto& a){ return a.gear_id == gear_id; });
                            if (iter == user_icon.end())
                            if (auto gear_ptr = boss.bell::getref<hids>(gear_id))
                            {
                                auto index = gear_ptr->user_index;
                                auto color = argb::vt256[4 + index % (256 - 4)];
                                auto image = ansi::fgc(color).add("\0▀"sv);
                                user_icon.push_front({ gear_id, image });
                                rebuild();
                            }
                        };
                        focusable_parent->LISTEN(tier::release, e2::form::state::focus::off, gear_id, *subs_ptr)
                        {
                            if (!gear_id) return;
                            auto iter = std::find_if(user_icon.begin(), user_icon.end(), [&](auto& a){ return a.gear_id == gear_id; });
                            if (iter != user_icon.end())
                            {
                                user_icon.erase(iter);
                                rebuild();
                            }
                        };
                    }
                };
                boss.LISTEN(tier::release, e2::area, new_area, memo)
                {
                    if (boss.base::size() != new_area.size)
                    {
                        recalc(new_area.size);
                    }
                };
                boss.LISTEN(tier::release, e2::postrender, canvas, memo)
                {
                    if (live)
                    {
                        auto saved_context = canvas.bump(dent{ 0,0,head_size.y,foot_size.y });
                        if (head_live)
                        {
                            canvas.cup(dot_00);
                            canvas.output(head_page, cell::shaders::contrast);
                        }
                        if (foot_live)
                        {
                            canvas.cup({ 0, head_size.y + boss.size().y });
                            canvas.output(foot_page, cell::shaders::contrast);
                        }
                        canvas.bump(saved_context);
                    }
                };
                boss.LISTEN(tier::preview, e2::form::prop::ui::header, newtext, memo)
                {
                    header(newtext);
                };
                boss.LISTEN(tier::request, e2::form::prop::ui::header, curtext, memo)
                {
                    curtext = head_text;
                };
                boss.LISTEN(tier::request, e2::form::prop::ui::title, curtext, memo)
                {
                    curtext = head_foci;
                };
                boss.LISTEN(tier::preview, e2::form::prop::ui::footer, newtext, memo)
                {
                    footer(newtext);
                };
                boss.LISTEN(tier::request, e2::form::prop::ui::footer, curtext, memo)
                {
                    curtext = foot_text;
                };
            }
        };

        // pro: Deprecated. Perform graceful shutdown functionality. LIMIT in seconds, ESC_THRESHOLD in milliseconds.
        class guard
            : public skill
        {
            using skill::boss,
                  skill::memo;

            static constexpr auto threshold = 500ms; // guard: Double escape threshold.

            bool wait; // guard: Ready to close.
            time stop; // guard: Timeout for single Esc.

        public:
            guard(base&&) = delete;
            guard(base& boss) : skill{ boss },
                wait{ faux }
            {
                // Suspected early completion.
                boss.LISTEN(tier::release, e2::conio::preclose, pre_close, memo)
                {
                    if ((wait = pre_close))
                    {
                        stop = datetime::now() + threshold;
                    }
                };
                // Double escape catcher.
                boss.LISTEN(tier::general, e2::timer::any, timestamp, memo)
                {
                    if (wait && (timestamp > stop))
                    {
                        wait = faux;
                        auto shadow = boss.This();
                        log(prompt::gate, "Shutdown by double escape");
                        boss.bell::signal(tier::preview, e2::conio::quit);
                        memo.clear();
                    }
                };
            }
        };

        // pro: Close owner on mouse inactivity timeout.
        class watch
            : public skill
        {
            using skill::boss,
                  skill::memo;

            static constexpr auto limit = 600s; //todo unify // watch: Idle timeout in seconds.

            hook pong; // watch: Alibi subsciption token.
            hook ping; // watch: Zombie check countdown token.
            time stop; // watch: Timeout for zombies.

        public:
            watch(base&&) = delete;
            watch(base& boss) : skill{ boss }
            {
                stop = datetime::now() + limit;

                // No mouse events watchdog.
                boss.LISTEN(tier::preview, hids::events::mouse::any, something, pong)
                {
                    stop = datetime::now() + limit;
                };
                boss.LISTEN(tier::general, e2::timer::any, something, ping)
                {
                    if (datetime::now() > stop)
                    {
                        auto backup = boss.This();
                        log(prompt::gate, "No mouse clicking events");
                        boss.base::riseup(tier::release, e2::form::proceed::quit::one, true);
                        ping.reset();
                        memo.clear();
                    }
                };
            }
        };

        // pro: Text input focus tree.
        class focus
            : public skill
        {
            //
            //     ┌─── input gate 1 (vtm::gate<ui::gate, k>) -> base::root(true)
            //     │      ↓   ↑
            //     │      │   └─ gears (input_t)...
            //     │      └─ applet (Taskbar/Standalone app (ui::base, f))... 
            //     │
            //     │   ┌─── input gate M (vtm::gate<ui::gate, k>) -> base::root(true)                        ↑ Outside
            //     │   │      ↓   ↑                                                                         ██
            //     │   │      │   └─ gears (input_t)                                                        ██
            //     │   │      │       ├─ ... ├─ keybdN                                                    ██  ░░
            //     │   │      │       ├─ ... ├─ mouseN                                                    ██  ░░ Dead (unfocused, focusable)
            //     │   │      │       ├─ ... ├─ focusN                                                    ██
            //     │   │      │       └─ ... └─ clipboardN                                                ██ Live (focused, hub)
            //     │   │      │                                                                         ██  ██
            //     │   │      └─ applet (Taskbar/Standalone app (ui::base, f))                          ██  ██ Live (focused, focusable)
            //     │   │                                                                                ██
            //     ↓ : ↓                                                                                ██ Live (focused, hub)
            // vtm desktop (vtm::hall<ui::host, f<mode::focusable>>)                                  ██  ░░
            //  ↓ ↓  : ↓                                                                              ██  ░░ Dead (unfocused, hub)
            //  │ │    └─── window 1 (ui::cake, f) -> base::kind(reflow_root), base::root(true)       ██    ▓▓
            //  │ │          ↓                                                                        ██    ▓▓
            //  │ │          └─ applet (ui::dtvt, f<mode::relay>)                                   ██  ██  ▓▓ Idle (unfocused, focusable)
            //  │ │                                                                                 ██  ██...
            //  │ └─── window N (ui::cake, f) -> base::kind(reflow_root), base::root(true)          ██
            //  │       ↓                                                                           ██ Live (focused, focusable)
            //  │       └─ applet (tile (ui::fork, f, k)...)                                         ↓ Inside
            //  │           ↓
            //  │           └─ ...
            //  └─── window N+1 (ui::cake, f) -> base::kind(reflow_root), base::root(true)
            //        ↓
            //        └─ applet (info (ui::cake, f<focused>, k)...)
            //            ↓
            //            └─ ...
            struct state
            {
                static constexpr auto dead = 0;
                static constexpr auto live = 1;
                static constexpr auto idle = 2;
            };
            struct chain_t
            {
                struct dest_t
                {
                    wptr next_wptr; // next hop wptr.
                    si32 status{}; // dead, live or idle.
                };

                si32              active{}; // focus: The endpoint focus state.
                hook              token;    // focus: Cleanup token.
                std::list<dest_t> next;     // focus: Focus next hop list.

                template<class P>
                auto foreach(P proc)
                {
                    auto head = next.begin();
                    while (head != next.end())
                    {
                        if (auto nexthop = head->next_wptr.lock(); nexthop && (proc(nexthop, head->status), nexthop))
                        {
                            head++;
                        }
                        else
                        {
                            head = next.erase(head);
                        }
                    }
                }
            };

            using umap = std::unordered_map<id_t, chain_t>; // Each gear has its own focus tree.
            using skill::boss,
                  skill::memo;

            //todo kb navigation type: transit, cyclic, plain, disabled, closed
            umap gears; // focus: Registered gears.
            si32 node_type; // focus: .
            si32 count{}; // focus: The number of active gears.
            si64 treeid = datetime::uniqueid(); // focus: .
            ui64 digest = ui64{}; // focus: .

            auto add_chain(id_t gear_id, chain_t new_chain = { .active = state::dead })
            {
                auto iter = gears.emplace(gear_id, std::move(new_chain)).first;
                if (gear_id)
                {
                    if (auto gear_ptr = boss.bell::getref<hids>(gear_id))
                    {
                        auto& chain = iter->second;
                        gear_ptr->LISTEN(tier::release, hids::events::die, gear, chain.token)
                        {
                            auto iter = gears.find(gear.id);
                            if (iter != gears.end()) // Make the current branch default.
                            {
                                auto& chain = iter->second;
                                auto  token = std::move(chain.token);
                                if (notify_focus_state(state::idle, chain, gear.id))
                                {
                                    chain.active = state::live; // Keep chain state.
                                }
                                gears[id_t{}] = std::move(chain);
                                boss.bell::signal(tier::release, hids::events::die, gear); // Notify pro::keybd.
                                gears.erase(iter);
                            }
                        };
                    }
                }
                return iter;
            }
            auto& get_chain(id_t gear_id)
            {
                auto iter = gears.find(gear_id);
                if (iter == gears.end())
                {
                    iter = add_chain(gear_id);
                }
                return iter->second;
            }
            bool notify_focus_state(si32 active, chain_t& chain, id_t gear_id)
            {
                auto changed = (chain.active == state::live) != (active == state::live);
                chain.active = active;
                if (gear_id && changed)
                {
                    if (active == state::live)
                    {
                        count++;
                        boss.bell::signal(tier::release, e2::form::state::focus::on, gear_id);
                    }
                    else
                    {
                        count--;
                        boss.bell::signal(tier::release, e2::form::state::focus::off, gear_id);
                    }
                    boss.bell::signal(tier::release, e2::form::state::focus::count, count);
                }
                return changed;
            }

        public:
            struct mode
            {
                static constexpr auto focusable = 0; // The object can be focused and active, it is unfocused by default. It cuts the  focus tree when focus is set on it.
                static constexpr auto focused   = 1; // The object can be focused and active, it is focused by default. It cuts the  focus tree when focus is set on it.
                static constexpr auto hub       = 2; // The object can't be focused, only active, it is inactive by default. It doesn't cut the focus tree when focus is set on it, it just activate a whole branch.
                static constexpr auto active    = 3; // The object can't be focused, only active, it is active by default. It doesn't cut the focus tree when focus is set on it, it just activate a whole branch.
                static constexpr auto relay     = 4; // The object is on the process/event domain boundary and can't be focused (gui and ui::dtvt). Always has default focus.
            };

            template<class T>
            static void set(sptr item_ptr, T&& gear_id, si32 focus_type, bool just_activate_only = faux) // just_activate_only means don't focus just activate only.
            {
                auto lock = item_ptr->bell::sync();
                auto fire = [&](auto id)
                {
                    item_ptr->base::riseup(tier::preview, hids::events::focus::set::on, { .gear_id = id, .focus_type = focus_type, .just_activate_only = just_activate_only });
                };
                if constexpr (std::is_same_v<id_t, std::decay_t<T>>)
                {
                    fire(gear_id);
                }
                else
                {
                    for (auto next_id : gear_id)
                    {
                        fire(next_id);
                    }
                }
            }
            template<class T>
            static void off(sptr item_ptr, T&& gear_id)
            {
                auto lock = item_ptr->bell::sync();
                auto fire = [&](auto id)
                {
                    item_ptr->base::riseup(tier::preview, hids::events::focus::set::off, { .gear_id = id });
                };
                if constexpr (std::is_same_v<id_t, std::decay_t<T>>)
                {
                    fire(gear_id);
                }
                else
                {
                    for (auto next_id : gear_id)
                    {
                        fire(next_id);
                    }
                }
            }
            static auto off(sptr item_ptr) // Used by d_n_d::drop (tile.hpp:586).
            {
                auto lock = item_ptr->bell::sync();
                auto gear_id_list = item_ptr->base::riseup(tier::request, e2::form::state::keybd::enlist);
                pro::focus::off(item_ptr, gear_id_list);
            }
            // pro::focus: Defocus all gears except specified.
            static auto one(sptr item_ptr, id_t gear_id)
            {
                auto lock = item_ptr->bell::sync();
                auto gear_id_list = item_ptr->base::riseup(tier::request, e2::form::state::keybd::enlist);
                std::erase_if(gear_id_list, [&](auto& id){ return gear_id == id; });
                pro::focus::off(item_ptr, gear_id_list);
            }
            // pro::focus: Defocus all gears and clear chains (optionally remove_default route from parent) and return deleted active gear id's.
            static auto cut(sptr item_ptr, bool remove_default = faux)
            {
                auto lock = item_ptr->bell::sync();
                auto gear_id_list = hids::events::focus::cut.param();
                if (auto parent = item_ptr->parent())
                {
                    parent->base::riseup(tier::request, hids::events::focus::cut, gear_id_list);
                    if (remove_default)
                    {
                        parent->base::riseup(tier::request, hids::events::focus::dry, { .item = item_ptr });
                    }
                }
                return gear_id_list;
            }
            // pro::focus: Switch all foci at the predecessor hub from the prev_ptr to the next_ptr (which must have a pro::focus on board).
            static auto hop(sptr prev_item_ptr, sptr next_item_ptr)
            {
                auto lock = prev_item_ptr->bell::sync();
                if (auto parent = prev_item_ptr->parent())
                {
                    parent->base::riseup(tier::request, hids::events::focus::hop, { .item = prev_item_ptr, .next = next_item_ptr });
                }
            }
            static auto test(base& item, input::hids& gear)
            {
                auto gear_test = item.base::riseup(tier::request, e2::form::state::keybd::find, { gear.id, 0 });
                return gear_test.second;
            }
            auto is_focused(id_t gear_id)
            {
                auto iter = gears.find(gear_id);
                auto result = iter != gears.end() && iter->second.active == state::live;
                return result;
            }

            focus(base&&) = delete;
            focus(base& boss, si32 focus_mode = mode::hub, bool set_default_focus = true)
                : skill{ boss },
                  node_type{ focus_mode }
            {
                if (set_default_focus)
                if (node_type == mode::focused || node_type == mode::active || node_type == mode::relay) // Pave default focus path at startup.
                {
                    boss.LISTEN(tier::anycast, e2::form::upon::started, parent_ptr, memo)
                    {
                        if (parent_ptr) // Parent_ptr is always empty when the boss is dropped via d_n_d.
                        {
                            pro::focus::set(boss.This(), id_t{}, solo::off);
                        }
                    };
                }
                // pro::focus: Return focus owner ptr.
                boss.LISTEN(tier::request, e2::config::plugins::focus::owner, owner_ptr, memo)
                {
                    owner_ptr = boss.This();
                };
                // pro::focus: Set unique focus on left click. Set group focus on Ctrl+LeftClick.
                boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear, memo)
                {
                    if (gear.meta(hids::anyCtrl))
                    {
                        if (pro::focus::test(boss, gear))
                        {
                            pro::focus::off(boss.This(), gear.id);
                        }
                        else
                        {
                            pro::focus::set(boss.This(), gear.id, solo::off);
                        }
                    }
                    else
                    {
                        pro::focus::set(boss.This(), gear.id, solo::on);
                    }
                    gear.dismiss();
                };
                // pro::focus: Subscribe on keybd events.
                boss.LISTEN(tier::preview, hids::events::keybd::key::post, gear, memo) // preview: Run after any.
                {
                    auto sent = faux;
                    auto& chain = get_chain(gear.id);
                    auto handled = gear.handled;
                    auto new_handled = handled;
                    chain.foreach([&](auto& nexthop, auto& status)
                    {
                        if (status == state::live)
                        {
                            sent = true;
                            gear.handled = handled;
                            nexthop->bell::signal(tier::preview, hids::events::keybd::key::post, gear);
                            new_handled |= gear.handled;
                        }
                    });
                    gear.handled = new_handled;
                    if (!sent && node_type != mode::relay) // Send key::post event back. The relays themselves will later send it back.
                    {
                        auto parent_ptr = boss.This();
                        while ((!gear.handled || gear.keystat == input::key::released) && parent_ptr) // Always pass released key events.
                        {
                            parent_ptr->bell::signal(tier::release, hids::events::keybd::key::post, gear);
                            parent_ptr = parent_ptr->parent();
                        }
                    }
                };
                // all tier::previews going to outside (upstream)
                // all tier::releases going to inside (downstream)
                // pro::focus: Off focus to inside.
                boss.LISTEN(tier::release, hids::events::focus::set::off, seed, memo)
                {
                    auto& chain = get_chain(seed.gear_id);
                    if (notify_focus_state(state::idle, chain, seed.gear_id))
                    {
                        chain.foreach([&](auto& nexthop, auto& status)
                        {
                            if (status == state::live)
                            {
                                status = state::idle;
                                nexthop->bell::signal(tier::release, hids::events::focus::set::off, seed);
                            }
                        });
                    }
                };
                // pro::focus: Make copy from default.
                boss.LISTEN(tier::request, hids::events::focus::dup, seed, memo)
                {
                    auto iter = gears.find(id_t{});
                    if (iter != gears.end())
                    {
                        iter = add_chain(seed.gear_id, iter->second); // Create a new chain which is based on default.
                        auto& chain = iter->second;
                        if (chain.active == state::live)
                        {
                            chain.active = state::idle;
                            notify_focus_state(state::live, chain, seed.gear_id);
                            if (node_type == mode::relay)
                            {
                                seed.item = boss.This();
                                boss.bell::signal(tier::release, hids::events::focus::set::on, seed);
                            }
                        }
                        chain.foreach([&](auto& nexthop, auto& /*status*/)
                        {
                            nexthop->bell::signal(tier::request, hids::events::focus::dup, seed);
                        });
                    }
                };
                // pro::focus: Set focus to inside.
                boss.LISTEN(tier::release, hids::events::focus::set::on, seed, memo)
                {
                    auto iter = gears.find(seed.gear_id);
                    if (iter == gears.end()) // No route to inside.
                    {
                        auto first_step = !seed.item; // No focused item yet. We are in the the first riseup iteration (pro::focus::set just called and catched the first plugin<pro::focus> owner). A focus leaf is not necessarily a visual tree leaf.
                        if (seed.gear_id && first_step && (iter = gears.find(id_t{}), iter != gears.end())) // Check if the default chain exists.
                        {
                            boss.bell::signal(tier::request, hids::events::focus::dup, seed);
                            return;
                        }
                        else
                        {
                            iter = add_chain(seed.gear_id);
                        }
                    }
                    auto& chain = iter->second;
                    auto prev_state = chain.active;
                    notify_focus_state(state::live, chain, seed.gear_id);
                    if (node_type != mode::relay)
                    {
                        auto allow_focusize = node_type == mode::focused || node_type == mode::focusable;
                        chain.foreach([&](auto& nexthop, auto& status)
                        {
                            if (status != state::dead || (!allow_focusize && prev_state == state::dead)) // Focusing a dead item activates a whole dead branch upto a focusable item.
                            {
                                status = state::live;
                                seed.item = boss.This();
                                nexthop->bell::signal(tier::release, hids::events::focus::set::on, seed);
                            }
                        });
                    }
                };
                // pro::focus: Set focus to outside.
                boss.LISTEN(tier::preview, hids::events::focus::set::on, seed, memo)
                {
                    seed.treeid = treeid;
                    seed.digest = ++digest;
                    auto first_step = !seed.item; // No focused item yet. We are in the the first riseup iteration (pro::focus::set just called and catched the first plugin<pro::focus> owner). A focus leaf is not necessarily a visual tree leaf.
                    if (first_step)
                    {
                        auto allow_focusize = seed.just_activate_only ? faux : (node_type == mode::focused || node_type == mode::focusable); // Ignore focusablity if it is requested.
                        if (seed.gear_id && (!allow_focusize || seed.focus_type != solo::on)) // Hub or group focus.
                        {
                            auto release_seed = seed;
                            boss.bell::signal(tier::release, hids::events::focus::set::on, release_seed); // Turn on a default downstream branch.
                        }
                        else
                        {
                            auto& chain = get_chain(seed.gear_id);
                            if (allow_focusize && seed.focus_type == solo::on) // Cut a downstream focus branch.
                            {
                                chain.foreach([&](auto& nexthop, auto& status)
                                {
                                    if (status == state::live)
                                    {
                                        status = state::dead;
                                        nexthop->bell::signal(tier::release, hids::events::focus::set::off, seed);
                                    }
                                });
                            }
                            notify_focus_state(state::live, chain, seed.gear_id);
                        }
                    }
                    else // Build focus tree (we are in the middle of the focus tree).
                    {
                        auto& chain = get_chain(seed.gear_id);
                        if (seed.focus_type == solo::on)
                        {
                            auto exists = faux;
                            chain.foreach([&](auto& nexthop, auto& status)
                            {
                                if (nexthop == seed.item)
                                {
                                    status = state::live;
                                    exists = true;
                                }
                                else
                                {
                                    status = state::dead;
                                    nexthop->bell::signal(tier::release, hids::events::focus::set::off, seed);
                                }
                            });
                            if (!exists)
                            {
                                chain.next.push_back({ wptr{ seed.item }, state::live });
                            }
                        }
                        else // Group focus.
                        {
                            auto iter = std::find_if(chain.next.begin(), chain.next.end(), [&](auto& n){ return n.next_wptr.lock() == seed.item; });
                            if (iter == chain.next.end())
                            {
                                chain.next.push_back({ wptr{ seed.item }, state::live });
                            }
                            else
                            {
                                iter->status = state::live;
                                if (seed.gear_id) // Seal the || branches.
                                {
                                    chain.foreach([&](auto& /*nexthop*/, auto& status)
                                    {
                                        if (status == state::idle)
                                        {
                                            status = state::dead;
                                        }
                                    });
                                }
                            }
                            if (chain.active == state::live) // Stop group focusing if the branch is already active.
                            {
                                return;
                            }
                        }
                        notify_focus_state(state::live, chain, seed.gear_id);
                    }
                    if (auto parent = boss.parent())
                    {
                        seed.item = boss.This();
                        parent->base::riseup(tier::preview, hids::events::focus::set::on, seed);
                    }
                };
                // pro::focus: Off focus to outside. Truncate the maximum path without branches.
                boss.LISTEN(tier::preview, hids::events::focus::set::off, seed, memo)
                {
                    seed.treeid = treeid;
                    seed.digest = ++digest;
                    auto first_step = !seed.item; // No unfocused item yet. We are in the the first riseup iteration (pro::focus::off just called and catched the first plugin<pro::focus> owner). A focus leaf is not necessarily a visual tree leaf.
                    auto& chain = get_chain(seed.gear_id);
                    if (first_step)
                    {
                        if (chain.active == state::live)
                        {
                            boss.bell::signal(tier::release, hids::events::focus::set::off, seed);
                        }
                    }
                    else //if (!first_step)
                    {
                        chain.foreach([&](auto& nexthop, auto& status)
                        {
                            if (nexthop == seed.item)
                            {
                                status = state::idle;
                            }
                        });
                        auto focusable = node_type == mode::focused || node_type == mode::focusable;
                        if (chain.next.size() > 1 || focusable) // Stop unfocusing on hub or focusable.
                        {
                            boss.bell::expire(tier::preview); // Don't let the hall send the event to the gate.
                            return;
                        }
                        notify_focus_state(state::idle, chain, seed.gear_id);
                    }
                    if (auto parent_ptr = boss.parent())
                    {
                        seed.item = boss.This();
                        parent_ptr->base::riseup(tier::preview, hids::events::focus::set::off, seed);
                    }
                };
                // pro::focus: Initiate focus setting toward outside (used by gui and dtvt).
                boss.LISTEN(tier::request, hids::events::focus::add, seed, memo)
                {
                    auto& chain = get_chain(seed.gear_id);
                    notify_focus_state(state::live, chain, seed.gear_id);
                    if (auto parent = boss.parent())
                    {
                        seed.item = boss.This();
                        parent->base::riseup(tier::preview, hids::events::focus::set::on, seed);
                    }
                };
                // pro::focus: Initiate focus unsetting toward outside (used by gui and dtvt).
                boss.LISTEN(tier::request, hids::events::focus::rem, seed, memo)
                {
                    auto& chain = get_chain(seed.gear_id);
                    auto boss_ptr = boss.This();
                    if (notify_focus_state(state::idle, chain, seed.gear_id))
                    {
                        if (auto parent_ptr = boss.parent())
                        {
                            seed.item = boss_ptr;
                            parent_ptr->base::riseup(tier::preview, hids::events::focus::set::off, seed);
                        }
                    }
                };
                // pro::focus: Drop all downlinks (toward inside) from the boss and unfocus boss. Return dropped active gears.
                boss.LISTEN(tier::request, hids::events::focus::cut, gear_id_list, memo)
                {
                    for (auto& [gear_id, chain] : gears)
                    {
                        auto live = faux;
                        chain.foreach([&, gear_id = gear_id](auto& nexthop, auto& status) // Drop all downlinks (toward inside) from the boss. //todo Apple clang can't capture gear_id in lambda
                        {
                            if (gear_id && status == state::live)
                            {
                                live = true;
                                nexthop->bell::signal(tier::release, hids::events::focus::set::off, { .gear_id = gear_id });
                            }
                            nexthop = {};
                        });
                        if (gear_id)
                        {
                            boss.bell::signal(tier::preview, hids::events::focus::set::off, { .gear_id = gear_id }); // The cutting object is changing its host along with focus.
                            if (live) gear_id_list.push_back(gear_id); // Backup dropped active gears.
                        }
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::request, hids::events::focus::dry, seed, memo)
                {
                    for (auto& [gear_id, chain] : gears)
                    {
                        chain.next.remove_if([&](auto& next){ return next.next_wptr.lock() == seed.item; });
                    }
                };
                // pro::focus: Switch all foci from the prev_ptr to the next_ptr (which must have a pro::focus on board).
                boss.LISTEN(tier::request, hids::events::focus::hop, seed, memo)
                {
                    auto prev_ptr = seed.item;
                    auto next_ptr = seed.next;
                    for (auto& [gear_id, chain] : gears)
                    {
                        auto iter = chain.next.begin();
                        while (iter != chain.next.end())
                        {
                            auto& r = *iter++;
                            auto item_ptr = r.next_wptr.lock();
                            if (!item_ptr || item_ptr == next_ptr)
                            {
                                iter = chain.next.erase(iter);
                            }
                            else if (item_ptr == prev_ptr)
                            {
                                r.next_wptr = next_ptr;
                                if (gear_id && r.status == state::live)
                                {
                                    prev_ptr->bell::signal(tier::release, hids::events::focus::set::off, { .gear_id = gear_id, .treeid = treeid, .digest = ++digest });
                                    next_ptr->bell::signal(tier::release, hids::events::focus::set::on,  { .gear_id = gear_id, .treeid = treeid, .digest = ++digest });
                                }
                            }
                        }
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::request, e2::form::state::keybd::enlist, gear_id_list, memo)
                {
                    for (auto& [gear_id, chain] : gears)
                    {
                        if (gear_id && chain.active == state::live)
                        {
                            gear_id_list.push_back(gear_id);
                        }
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::request, e2::form::state::focus::count, gear_count, memo)
                {
                    gear_count = count;
                };
                // pro::focus: .
                boss.LISTEN(tier::request, e2::form::state::keybd::find, gear_test, memo)
                {
                    auto iter = gears.find(gear_test.first);
                    if (iter != gears.end() && iter->second.active == state::live)
                    {
                        gear_test.second++;
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::request, e2::form::state::keybd::next, gear_test, memo)
                {
                    auto iter = gears.find(gear_test.first);
                    if (iter != gears.end())
                    {
                        auto& chain = iter->second;
                        if (chain.active == state::live)
                        {
                            chain.foreach([&](auto& /*nexthop*/, auto& status)
                            {
                                if (status == state::live)
                                {
                                    gear_test.second++;
                                }
                            });
                        }
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::general, e2::form::proceed::functor, function, memo)
                {
                    for (auto& [gear_id, chain] : gears)
                    {
                        //todo revise
                        if (gear_id && chain.next.empty() && chain.active == state::live)
                        {
                            function(boss.This());
                        }
                    }
                };
                // pro::focus: .
                boss.LISTEN(tier::general, ui::e2::command::request::inputfields, inputfield_request, memo)
                {
                    if (auto iter = gears.find(inputfield_request.gear_id); iter != gears.end())
                    {
                        auto& chain = iter->second;
                        if (chain.active == state::live)
                        {
                            boss.bell::signal(tier::release, ui::e2::command::request::inputfields, inputfield_request);
                        }
                    }
                };
            }
        };

        // pro: Mouse events.
        class mouse
            : public skill
        {
            struct sock
            {
                operator bool () { return true; }
            };

            using list = socks<sock>;
            using skill::boss,
                  skill::memo;

            sptr soul; // mouse: Boss cannot be removed while it has active gears.
            list mice; // mouse: List of active mice.
            bool omni; // mouse: Ability to accept all hover events (true) or only directly over the object (faux).
            si32 rent; // mouse: Active gears count.
            si32 full; // mouse: All gears count. Counting to keep the entire chain of links in the visual tree.
            si32 drag; // mouse: Bitfield of buttons subscribed to mouse drag.
            std::map<si32, subs> dragmemo; // mouse: Draggable subs.

        public:
            mouse(base&&) = delete;
            mouse(base& boss, bool take_all_events = true)
                : skill{ boss            },
                   mice{ boss            },
                   omni{ take_all_events },
                   rent{ 0               },
                   full{ 0               },
                   drag{ 0               }
            {
                // pro::mouse: Refocus all active mice on detach (to keep the mouse event tree consistent).
                boss.LISTEN(tier::release, e2::form::upon::vtree::detached, parent_ptr, memo)
                {
                    if (parent_ptr)
                    {
                        auto& parent = *parent_ptr;
                        mice.foreach([&](auto& gear)
                        {
                            if (auto gear_ptr = boss.bell::getref<hids>(gear.id))
                            {
                                gear_ptr->redirect_mouse_focus(parent);
                            }
                        });
                    }
                };
                // pro::mouse: Forward preview to all parents.
                boss.LISTEN(tier::preview, hids::events::mouse::any, gear, memo)
                {
                    auto& offset = boss.base::coor();
                    gear.pass(tier::preview, boss.parent(), offset);

                    if (gear) gear.okay(boss);
                    else      boss.bell::expire(tier::preview);
                };
                // pro::mouse: Forward all not expired mouse events to all parents.
                boss.LISTEN(tier::release, hids::events::mouse::any, gear, memo)
                {
                    if ((gear && !gear.captured()) || gear.cause == hids::events::mouse::hover::enter.id || gear.cause == hids::events::mouse::hover::leave.id)
                    {
                        auto& offset = boss.base::coor();
                        gear.pass(tier::release, boss.parent(), offset);
                    }
                };
                // pro::mouse: Amplify mouse hover on any button press.
                boss.LISTEN(tier::release, hids::events::mouse::button::any, gear, memo)
                {
                    if (events::subevent(gear.cause, hids::events::mouse::button::down::any.id)
                     || events::subevent(gear.cause, hids::events::mouse::button::up::any.id))
                    {
                        boss.bell::signal(tier::release, e2::form::state::hover, rent + gear.mouse::pressed_count);
                    }
                };
                // pro::mouse: Notify about change in number of mouse hovering clients.
                boss.LISTEN(tier::release, hids::events::mouse::hover::any, gear, memo)
                {
                    if (gear.cause == hids::events::mouse::hover::enter.id) // Notify when the number of clients is positive.
                    {
                        if (!full++)
                        {
                            soul = boss.This();
                        }
                        if (gear.direct<true>(boss.bell::id) || omni)
                        {
                            if (!rent++)
                            {
                                boss.bell::signal(tier::release, e2::form::state::mouse, rent);
                            }
                            boss.bell::signal(tier::release, e2::form::state::hover, rent);
                        }
                    }
                    else if (gear.cause == hids::events::mouse::hover::leave.id) // Notify when the number of clients is zero.
                    {
                        if (gear.direct<faux>(boss.bell::id) || omni)
                        {
                            if (!--rent)
                            {
                                boss.bell::signal(tier::release, e2::form::state::mouse, rent);
                            }
                            boss.bell::signal(tier::release, e2::form::state::hover, rent);
                        }
                        if (!--full)
                        {
                            soul->base::strike();
                            soul.reset();
                        }
                    }
                };
                boss.LISTEN(tier::request, e2::form::state::mouse, state, memo)
                {
                    state = rent;
                };
                boss.LISTEN(tier::request, e2::form::state::hover, state, memo)
                {
                    state = rent;
                };
                boss.LISTEN(tier::release, e2::form::draggable::any, enabled, memo)
                {
                    auto deed = boss.bell::protos(tier::release);
                    switch (deed)
                    {
                        default:
                        case e2::form::draggable::left     .id: draggable<hids::buttons::left     >(enabled); break;
                        case e2::form::draggable::right    .id: draggable<hids::buttons::right    >(enabled); break;
                        case e2::form::draggable::middle   .id: draggable<hids::buttons::middle   >(enabled); break;
                        case e2::form::draggable::xbutton1 .id: draggable<hids::buttons::xbutton1 >(enabled); break;
                        case e2::form::draggable::xbutton2 .id: draggable<hids::buttons::xbutton2 >(enabled); break;
                        case e2::form::draggable::leftright.id: draggable<hids::buttons::leftright>(enabled); break;
                    }
                };
            }
            void reset()
            {
                auto lock = boss.bell::sync();
                if (full)
                {
                    full = 0;
                    soul.reset();
                }
            }
            void take_all_events(bool b)
            {
                omni = b;
            }
            template<hids::buttons Button>
            void draggable(bool enabled)
            {
                if (!enabled)
                {
                    dragmemo[Button].clear();
                    drag &= ~(1 << Button);
                }
                else if (!(drag & 1 << Button))
                {
                    drag |= 1 << Button;
                    //using bttn = hids::events::mouse::button; //MSVC 16.9.4 don't get it
                    boss.LISTEN(tier::release, hids::events::mouse::button::drag::start::_<Button>, gear, dragmemo[Button])
                    {
                        if (gear.capture(boss.bell::id))
                        {
                            boss.bell::signal(tier::release, e2::form::drag::start::_<Button>, gear);
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::drag::pull::_<Button>, gear, dragmemo[Button])
                    {
                        if (gear.captured(boss.bell::id))
                        {
                            boss.bell::signal(tier::release, e2::form::drag::pull::_<Button>, gear);
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::drag::cancel::_<Button>, gear, dragmemo[Button])
                    {
                        if (gear.captured(boss.bell::id))
                        {
                            boss.bell::signal(tier::release, e2::form::drag::cancel::_<Button>, gear);
                            gear.setfree();
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::general, hids::events::halt, gear, dragmemo[Button])
                    {
                        if (gear.captured(boss.bell::id))
                        {
                            boss.bell::signal(tier::release, e2::form::drag::cancel::_<Button>, gear);
                            gear.setfree();
                            gear.dismiss();
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::drag::stop::_<Button>, gear, dragmemo[Button])
                    {
                        if (gear.captured(boss.bell::id))
                        {
                            boss.bell::signal(tier::release, e2::form::drag::stop::_<Button>, gear);
                            gear.setfree();
                            gear.dismiss();
                        }
                    };
                }
            }
        };

        // pro: Keyboard events.
        class keybd
            : public skill
        {
            using func = std::function<void(hids&, txts&)>;
            using wptr = netxs::wptr<func>;
            using sptr = netxs::sptr<func>;
            using skill::boss,
                  skill::memo;

            std::unordered_map<text, std::pair<std::list<std::pair<wptr, netxs::sptr<txts>>>, bool>, qiew::hash, qiew::equal> handlers;
            std::unordered_map<text, sptr, qiew::hash, qiew::equal> api_map;
            subs tokens;
            bool interrupt_key_proc;
            std::unordered_map<id_t, time> last_key;
            si64 instance_id;

            auto _get_chord_list(qiew chord_str = {}) -> std::optional<std::invoke_result_t<decltype(input::key::kmap::chord_list), qiew>>
            {
                auto chords = input::key::kmap::chord_list(chord_str);
                if (chords.size())
                {
                    return std::optional{ chords };
                }
                else
                {
                    if (chord_str) log("%%Unknown key chord: '%chord%'", prompt::user, chord_str);
                    return std::optional<decltype(chords)>{};
                }
            }
            auto _get_chords(qiew chord_list_str)
            {
                auto chord_qiew_list = utf::split<true>(chord_list_str, " | ");
                if (chord_qiew_list.size())
                {
                    auto head = chord_qiew_list.begin();
                    auto tail = chord_qiew_list.end();
                    if (auto first_chord_list = _get_chord_list(*head++))
                    {
                        auto chords = first_chord_list.value();
                        while (head != tail)
                        {
                            auto chord_qiew = *head++;
                            if (auto next_chord_list = _get_chord_list(chord_qiew))
                            {
                                auto& c = next_chord_list.value();
                                chords.insert(chords.end(), c.begin(), c.end());
                            }
                        }
                        return std::optional{ chords };
                    }
                }
                return _get_chord_list();
            }
            auto _proceed(hids& gear, auto iter)
            {
                auto& [procs, run_preview] = iter->second;
                std::erase_if(procs, [&](auto& rec)
                {
                    auto& [proc_wptr, args_ptr] = rec;
                    auto proc_ptr = proc_wptr.lock();
                    if (proc_ptr)
                    {
                        if (!interrupt_key_proc) (*proc_ptr)(gear, *args_ptr);
                    }
                    return !proc_ptr;
                });
                if (procs.empty()) handlers.erase(iter);
            }
            void _dispatch(hids& gear, bool preview_mode, qiew chord)
            {
                auto iter = handlers.find(chord);
                if (iter != handlers.end())
                {
                    auto& [procs, run_preview] = iter->second;
                    if (!preview_mode || run_preview)
                    {
                        _proceed(gear, iter);
                    }
                    else
                    {
                        gear.touched = instance_id;
                    }
                }
            }

        public:
            keybd(base&&) = delete;
            keybd(base& boss)
                : skill{ boss },
                  interrupt_key_proc{ faux },
                  instance_id{ datetime::now().time_since_epoch().count() }
            {

                boss.LISTEN(tier::anycast, e2::form::upon::started, root, memo)
                {
                    tokens.clear();
                    if (auto focusable_parent_ptr = boss.base::riseup(tier::request, e2::config::plugins::focus::owner))
                    {
                        focusable_parent_ptr->LISTEN(tier::release, hids::events::die, gear, tokens)
                        {
                            last_key.erase(gear.id);
                        };
                        focusable_parent_ptr->LISTEN(tier::release, hids::events::keybd::key::any, gear, tokens)
                        {
                            gear.shared_event = gear.touched && gear.touched != instance_id;
                            auto& timecod = last_key[gear.id];
                            if (gear.timecod > timecod)
                            {
                                timecod = gear.timecod;
                                if (gear.payload == input::keybd::type::keypress)
                                {
                                    interrupt_key_proc = faux;
                                    if (!gear.handled) _dispatch(gear, faux, input::key::kmap::any_key);
                                    if (!gear.handled) _dispatch(gear, faux, gear.vkchord);
                                    if (!gear.handled) _dispatch(gear, faux, gear.chchord);
                                    if (!gear.handled) _dispatch(gear, faux, gear.scchord);
                                }
                            }
                            else
                            {
                                gear.set_handled();
                            }
                        };
                        focusable_parent_ptr->LISTEN(tier::preview, hids::events::keybd::key::any, gear, tokens)
                        {
                            gear.shared_event = gear.touched && gear.touched != instance_id;
                            if (gear.payload == input::keybd::type::keypress)
                            {
                                if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.vkchord);
                                if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.chchord);
                                if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.scchord);
                                if (!gear.touched && !gear.handled) _dispatch(gear, true, input::key::kmap::any_key);
                            }
                        };
                    }
                };
                proc("Noop",           [&](hids& gear, txts&){ gear.set_handled(); interrupt_key_proc = true; });
                proc("DropAutoRepeat", [&](hids& gear, txts&){ if (gear.keystat == input::key::repeated) { gear.set_handled(); interrupt_key_proc = true; }});
            }

            auto filter(hids& gear)
            {
                if (gear.payload == input::keybd::type::keypress)
                {
                    if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.vkchord);
                    if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.chchord);
                    if (!gear.touched && !gear.handled) _dispatch(gear, true, gear.scchord);
                }
            }
            void proc(qiew name, func proc)
            {
                api_map[name] = ptr::shared(std::move(proc));
            }
            auto bind(qiew chord_str, auto&& proc_names, bool preview = faux)
            {
                if (!chord_str) return;
                if (auto chord_list = _get_chords(chord_str))
                {
                    auto& chords = chord_list.value();
                    auto set = [&](qiew proc_name, auto args_ptr)
                    {
                        if (proc_name)
                        {
                            if (auto iter = api_map.find(proc_name); iter != api_map.end())
                            {
                                auto handler_ptr = iter->second;
                                for (auto& chord : chords)
                                {
                                    auto& r = handlers[chord];
                                    r.first.emplace_back(handler_ptr, args_ptr);
                                    r.second = preview;
                                }
                            }
                            else log("%%Action '%proc%' not found", prompt::user, proc_name);
                        }
                        else
                        {
                            for (auto& chord : chords)
                            {
                                handlers.erase(chord);
                            }
                        }
                    };
                    if constexpr (std::is_same_v<char, std::decay_t<decltype(proc_names[0])>>) // The case it is a plain string.
                    {
                        auto args_ptr = ptr::shared(txts{});
                        set(proc_names, args_ptr);
                    }
                    else
                    {
                        for (auto& proc_name : proc_names)
                        {
                            auto args_ptr = ptr::shared(std::move(proc_name.args));
                            set(proc_name.action, args_ptr);
                        }
                    }
                }
            }
            auto bind(auto& bindings)
            {
                for (auto& r : bindings)
                {
                    bind(r.chord, r.actions, r.preview);
                }
            }
            static auto load(xmls& config, qiew section)
            {
                auto bindings = input::key::keybind_list_t{};
                if (section)
                {
                    auto path = "/config/hotkeys/" + section.str() + "/key";
                    auto keybinds = config.list(path);
                    for (auto keybind_ptr : keybinds)
                    {
                        auto chord = keybind_ptr->take_value();
                        auto preview = keybind_ptr->take("preview", faux);
                        auto action_ptr_list = keybind_ptr->list("action");
                        bindings.push_back({ .chord = chord, .preview = preview });
                        auto& rec = bindings.back();
                        //if constexpr (debugmode) log("chord=%% preview=%%", chord, preview);
                        for (auto action_ptr : action_ptr_list)
                        {
                            rec.actions.push_back({ .action = action_ptr->take_value() });
                            auto& action = rec.actions.back();
                            //if constexpr (debugmode) log("  action=", action.action);
                            auto arg_ptr_list = action_ptr->list("data");
                            for (auto arg_ptr : arg_ptr_list)
                            {
                                action.args.push_back(arg_ptr->take_value());
                                //if constexpr (debugmode) log("    data=", action.args.back());
                            }
                        }
                    }
                }
                return bindings;
            }
        };

        // pro: Glow gradient filter.
        class grade
            : public skill
        {
            using skill::boss,
                  skill::memo;

        public:
            grade(base&&) = delete;
            grade(base& boss) : skill{ boss }
            {
                boss.LISTEN(tier::release, e2::postrender, parent_canvas, memo)
                {
                    auto size = si32{ 5 }; // grade: Vertical gradient size.
                    auto step = si32{ 2 }; // grade: Vertical gradient step.
                    auto shadow = argb{0xFF000000};
                    auto bright = argb{0xFFffffff};

                    //todo optimize - don't fill the head and foot twice
                    auto area = parent_canvas.clip();
                    auto n = std::clamp(size, 0, area.size.y / 2) + 1;
                    //auto n = std::clamp(size, 0, boss.base::size().y / 2) + 1;

                    auto head = area;
                    head.size.y = 1;
                    auto foot = head;
                    head.coor.y = area.coor.y + n - 2;
                    foot.coor.y = area.coor.y + area.size.y - n + 1;

                    for (auto i = 1; i < n; i++)
                    {
                        bright.alpha(i * step);
                        shadow.alpha(i * step);

                        parent_canvas.core::fill(head, [&](cell& c){ c.bgc().mix(bright); });
                        parent_canvas.core::fill(foot, [&](cell& c){ c.bgc().mix(shadow); });

                        head.coor.y--;
                        foot.coor.y++;
                    }
                };
            }
        };

        // pro: Fader animation.
        class fader
            : public skill
        {
            using skill::boss,
                  skill::memo;

            robot robo; // fader: .
            span  fade;
            cell  c1;
            cell  c2;
            cell  c2_orig;
            si32  transit;
            bool  fake = faux;

            //todo use lambda
            void work(si32 balance)
            {
                auto brush = boss.base::color();
                brush.avg(c1, c2, balance);
                fake = true;
                boss.base::color(brush);
                fake = faux;
                boss.base::deface();
            }

        public:
            fader(base&&) = delete;
            fader(base& boss, cell default_state, cell highlighted_state, span fade_out = 250ms, sptr tracking_object = {})
                : skill{ boss },
                   robo{ boss },
                   fade{ fade_out },
                     c1{ default_state },
                     c2{ highlighted_state },
                c2_orig{ highlighted_state },
                transit{ 0 }
            {
                boss.base::color(c1);
                boss.LISTEN(tier::release, e2::form::prop::filler, filler)
                {
                    if (!fake)
                    {
                        auto& fgc = filler.fgc();
                        auto& bgc = filler.bgc();
                        c1.fgc(fgc);
                        c1.bgc(bgc);
                        if (filler.fga()) c2.fgc(fgc);
                        else              c2.fgc(c2_orig.fgc());
                        if (filler.bga()) c2.bgc(bgc);
                        else              c2.bgc(c2_orig.bgc());
                        work(transit);
                    }
                };
                auto& root = tracking_object ? *tracking_object : boss;
                root.LISTEN(tier::release, e2::form::state::mouse, active, memo)
                {
                    robo.pacify();
                    if (active)
                    {
                        transit = 256;
                        work(transit);
                    }
                    else
                    {
                        if (fade != fade.zero())
                        {
                            auto range = transit;
                            auto limit = datetime::round<si32>(fade);
                            auto start = 0;
                            robo.actify(constlinearAtoB<si32>(range, limit, start), [&](auto step)
                            {
                                transit -= step;
                                work(transit);
                            });
                        }
                        else work(transit = 0);
                    }
                };
            }
        };

        // pro: UI-control cache.
        class cache
            : public skill
        {
            using skill::boss,
                  skill::memo;

            netxs::sptr<face> coreface; //todo revise necessity
            face&             bosscopy; // cache: Boss bitmap cache.
            bool              usecache; // cacheL .
            si32              lucidity; // cacheL .

        public:
            cache(base&&) = delete;
            cache(base& boss, bool rendered = true)
                : skill{ boss },
                  bosscopy{*(coreface = ptr::shared<face>())},
                  usecache{ true },
                  lucidity{ 0xFF }
            {
                bosscopy.link(boss.bell::id);
                bosscopy.size(boss.base::size());
                boss.LISTEN(tier::preview, e2::form::prop::ui::cache, state, memo)
                {
                    usecache = state;
                };
                boss.LISTEN(tier::anycast, e2::form::prop::lucidity, value, memo)
                {
                    if (value < 0)
                    {
                        value = lucidity;
                    }
                    else
                    {
                        lucidity = value;
                    }
                };
                boss.LISTEN(tier::release, e2::area, new_area, memo)
                {
                    if (bosscopy.size() != new_area.size)
                    {
                        bosscopy.size(new_area.size);
                    }
                };
                boss.LISTEN(tier::request, e2::form::canvas, canvas_ptr, memo)
                {
                    canvas_ptr = coreface;
                };
                if (rendered)
                {
                    boss.LISTEN(tier::release, e2::render::background::prerender, parent_canvas, memo)
                    {
                        if (!usecache) return;
                        if (boss.base::ruined())
                        {
                            bosscopy.wipe();
                            boss.base::ruined(faux);
                            boss.bell::signal(tier::release, e2::render::background::any, bosscopy);
                        }
                        auto full = parent_canvas.full();
                        bosscopy.move(full.coor);
                        if (lucidity == 0xFF) parent_canvas.fill(bosscopy, cell::shaders::overlay);
                        else                  parent_canvas.fill(bosscopy, cell::shaders::transparent(lucidity));
                        bosscopy.move(dot_00);
                        boss.bell::expire(tier::release);
                    };
                }
            }
        };

        // pro: Acrylic blur.
        class acryl
            : public skill
        {
            using skill::boss,
                  skill::memo;

            si32 width; // acryl: Blur radius.
            bool alive; // acryl: Is active.
            vrgb cache; // acryl: Boxblur temp buffer.

        public:
            acryl(base&&) = delete;
            acryl(base& boss, si32 size = 3)
                : skill{ boss },
                  width{ size },
                  alive{ true }
            {
                boss.LISTEN(tier::preview, e2::form::prop::ui::acryl, state, memo)
                {
                    alive = state;
                };
                boss.LISTEN(tier::anycast, e2::form::prop::lucidity, lucidity, memo)
                {
                    if (lucidity != -1) alive = lucidity == 0xFF;
                };
                boss.LISTEN(tier::release, e2::render::background::prerender, parent_canvas, memo)
                {
                    if (!alive || boss.base::filler.bga() == 0xFF) return;
                    parent_canvas.blur(width, cache, [&](cell& c){ c.alpha(0xFF); });
                };
            }
        };

        //todo deprecated: use form::shader instead
        // pro: Background highlighter.
        class light
            : public skill
        {
            using skill::boss,
                  skill::memo;

            bool highlighted = faux; // light: .
            argb title_fg_color = 0xFFffffff;

        public:
            light(base&&) = delete;
            light(base& boss)
                : skill{ boss }
            {
                boss.LISTEN(tier::release, e2::form::state::highlight, state, memo)
                {
                    highlighted = state;
                    boss.base::deface();
                };
                boss.LISTEN(tier::release, e2::render::background::prerender, parent_canvas, memo)
                {
                    if (highlighted)
                    {
                        auto area = parent_canvas.full();
                        auto mark = skin::color(tone::brighter);
                             mark.fgc(title_fg_color); //todo unify, make it more contrast
                        auto fill = [&](cell& c) { c.fuse(mark); };
                        parent_canvas.fill(area, fill);
                    }
                };
            }
        };

        // pro: Custom highlighter.
        template<auto fx>
        class shade
            : public skill
        {
            using skill::boss,
                  skill::memo;

            bool highlighted = faux; // light2: .

        public:
            shade(base&&) = delete;
            shade(base& boss)
                : skill{ boss }
            {
                boss.LISTEN(tier::release, e2::form::state::mouse, active, memo)
                {
                    highlighted = active;
                    boss.base::deface();
                };
                boss.LISTEN(tier::release, e2::postrender, parent_canvas, memo)
                {
                    if (highlighted)
                    {
                        auto area = parent_canvas.full();
                        parent_canvas.fill(area, fx);
                    }
                };
            }
        };

        // pro: UI-control shadow.
        class ghost
            : public skill
        {
            using skill::boss,
                  skill::memo;

            auto draw_shadow(face& canvas)
            {
                if (skin::globals().shadow_enabled)
                {
                    static auto shadow = netxs::misc::shadow<core>{};
                    if (!shadow.sync) shadow.generate(skin::globals().shadow_bias,
                                                      skin::globals().shadow_opacity,
                                                      skin::globals().shadow_blur * 2,
                                                      skin::globals().shadow_offset,
                                                      dot_21,
                                                      [](cell& c, auto a){ c.alpha(a); });
                    shadow.render(canvas, canvas.area(), rect{ .size = boss.base::size() }, cell::shaders::blend);
                }
            }

        public:
            ghost(base&&) = delete;
            ghost(base& boss)
                : skill{ boss }
            {
                boss.LISTEN(tier::release, e2::render::background::prerender, parent_canvas, memo)
                {
                    draw_shadow(parent_canvas);
                };
                //test
                //boss.LISTEN(tier::release, hids::events::mouse::scroll::any, gear)
                //{
                //    boss.base::deface();
                //};
            }
        };

        // pro: Drag&roll support.
        class glide
            : public skill
        {
            using skill::boss,
                  skill::memo;

        public:
            glide(base&&) = delete;
            glide(base& boss)
                : skill{ boss }
            {

            }
        };

        // pro: Tooltip support.
        class notes
            : public skill
        {
            using skill::boss,
                  skill::memo;

            text note;

        public:
            notes(base&&) = delete;
            notes(base& boss, view data = {}, dent wrap = { si32max })
                : skill{ boss },
                  note { data }
            {
                boss.LISTEN(tier::release, hids::events::mouse::hover::enter, gear, memo, (wrap, full = wrap.l == si32max))
                {
                    if (gear.tooltip_set) return; // Prevent parents from setting tooltip.
                    if (full || !(boss.area() + wrap).hittest(gear.coord + boss.coor()))
                    {
                        gear.set_tooltip(note);
                    }
                };
                boss.LISTEN(tier::preview, e2::form::prop::ui::tooltip, new_note, memo)
                {
                    note = new_note;
                };
            }
            void update(view new_note)
            {
                note = new_note;
            }
        };
    }

    auto& tui_domain()
    {
        static auto indexer = netxs::events::auth{};
        return indexer;
    }

    // controls: base UI element.
    template<class T>
    class form
        : public base
    {
        std::map<std::type_index, uptr<pro::skill>> depo;
        std::map<id_t, subs> memomap; // form: Token set for dependent subscriptions.

    public:
        pro::mouse mouse{ *this }; // form: Mouse controller.
        //pro::keybd keybd{ *this }; // form: Keybd controller.

        form(size_t nested_count = 0)
            : base{ ui::tui_domain(), nested_count }
        { }

        auto This() { return base::This<T>(); }
        template<class TT = T, class ...Args>
        static auto ctor(Args&&... args)
        {
            auto item = ui::tui_domain().template create<TT>(std::forward<Args>(args)...);
            return item;
        }
        // form: Attach feature and return itself.
        template<class S, class ...Args>
        auto plugin(Args&&... args)
        {
            auto backup = This();
            depo[std::type_index(typeid(S))] = std::make_unique<S>(*backup, std::forward<Args>(args)...);
            return backup;
        }
        // form: Detach feature and return itself.
        template<class S>
        auto unplug()
        {
            auto backup = This();
            depo.erase(std::type_index(typeid(S)));
            return backup;
        }
        // form: Fill object region using parametrized fx.
        template<auto Tier = tier::release, auto RenderOrder = e2::render::background::any, class Fx, class Event = noop, bool fixed = std::is_same_v<Event, noop>>
        auto shader(Fx&& fx, Event sync = {}, sptr source_ptr = {}, netxs::sptr<subs> tokens_ptr = {})
        {
            static constexpr auto is_cell = std::is_same_v<cell, std::decay_t<Fx>>;
            auto& tokens = tokens_ptr ? *tokens_ptr : bell::tracker;
            if constexpr (fixed)
            {
                LISTEN(tier::release, RenderOrder, parent_canvas, tokens, (fx))
                {
                    parent_canvas.fill(fx);
                };
            }
            else
            {
                auto param_ptr = ptr::shared(Event::param());
                auto& param = *param_ptr;
                auto& source = source_ptr ? *source_ptr : *this;
                source.bell::signal(tier::request, sync, param);
                source.LISTEN(Tier, sync, new_value, tokens, (param_ptr))
                {
                    param = new_value;
                    base::deface();
                };
                if constexpr (is_cell) fx.link(bell::id);
                LISTEN(tier::release, RenderOrder, parent_canvas, tokens, (fx))
                {
                    static constexpr auto is_func = requires{ fx(parent_canvas, param, *this); };
                    if (param)
                    {
                             if constexpr (is_func) fx(parent_canvas, param, *this);
                        else if constexpr (is_cell) parent_canvas.fill(cell::shaders::fuseid(fx));
                        else                        parent_canvas.fill(fx[param]);
                    }
                };
            }
            return This();
        }
        // form: deprecated in favor of pro::brush. Set colors and return itself.
        template<class ...Args>
        auto colors(Args&&... args)
        {
            base::color(std::forward<Args>(args)...);
            return This();
        }
        // form: Set control as root.
        auto isroot(bool isroot, si32 ofkind = base::client)
        {
            base::root(isroot);
            base::kind(ofkind);
            return This();
        }
        // form: Set the form visible for mouse.
        auto active(cell brush)
        {
            base::color(brush.txt(whitespace).link(bell::id));
            return This();
        }
        auto active()
        {
            return active(base::color());
        }
        // form: Return plugin reference of specified type. Add the specified plugin (using specified args) if it is missing.
        template<class S, class ...Args>
        auto& plugins(Args&&... args)
        {
            const auto it = depo.find(std::type_index(typeid(S)));
            if (it == depo.end())
            {
                plugin<S>(std::forward<Args>(args)...);
            }
            auto ptr = static_cast<S*>(depo[std::type_index(typeid(S))].get());
            return *ptr;
        }
        // form: Invoke arbitrary functor(itself/*This/boss) in place.
        template<class P>
        auto invoke(P functor)
        {
            auto backup = This();
            functor(*backup);
            return backup;
        }
        // form: Attach homeless branch and return itself.
        template<class ...Args>
        auto branch(Args&&... args)
        {
            auto backup = This();
            backup->T::attach(std::forward<Args>(args)...);
            return backup;
        }
        // form: UI-control will be detached upon destruction of the master.
        auto depend(sptr master_ptr)
        {
            auto& master_inst = *master_ptr;
            master_inst.LISTEN(tier::release, e2::dtor, master_id, memomap[master_inst.id])
            {
                auto backup = This();
                memomap.erase(master_inst.id);
                if (memomap.empty()) base::detach();
            };
            return This();
        }
        // form: UI-control will be detached when the last item of collection is detached.
        template<class S>
        auto depend_on_collection(S data_collection_src) //todo too heavy, don't use
        {
            auto backup = This();
            for (auto& data_src : data_collection_src)
            {
                depend(data_src);
            }
            return backup;
        }
        // form: Create and attach a new item using a template and dynamic datasource.
        template<class Property, class Sptr, class P>
        auto attach_element(Property, Sptr data_src_sptr, P item_template)
        {
            auto arg_value = typename Property::type{};

            auto backup = This();
            data_src_sptr->bell::signal(tier::request, Property{}, arg_value);
            auto new_item = item_template(data_src_sptr, arg_value)
                                 ->depend(data_src_sptr);
            auto item_shadow = ptr::shadow(new_item);
            auto data_shadow = ptr::shadow(data_src_sptr);
            auto boss_shadow = ptr::shadow(backup);
            data_src_sptr->LISTEN(tier::release, Property{}, arg_new_value, memomap[data_src_sptr->id], (boss_shadow, data_shadow, item_shadow, item_template))
            {
                if (auto boss_ptr = boss_shadow.lock())
                if (auto data_src = data_shadow.lock())
                if (auto old_item = item_shadow.lock())
                {
                    auto new_item = item_template(data_src, arg_new_value)
                                         ->depend(data_src);
                    item_shadow = ptr::shadow(new_item); // Update current item shadow.
                    boss_ptr->replace(old_item, new_item);
                }
            };
            branch(new_item);
            return new_item;
        }
        // form: Create and attach a new item using a template and dynamic datasource.
        template<class Property, class S, class P, class F = noop>
        auto attach_collection(Property, S& data_collection_src, P item_template, F proc = {})
        {
            auto backup = This();
            for (auto& data_src_sptr : data_collection_src)
            {
                auto item = attach_element(Property{}, data_src_sptr, item_template);
                proc(data_src_sptr);
            }
            return backup;
        }
        template<class BackendProp, class P>
        void publish_property(BackendProp, P setter)
        {
            LISTEN(tier::request, BackendProp{}, property_value, -, (setter))
            {
                setter(property_value);
            };
        }
        template<class BackendProp, class FrontendProp>
        auto attach_property(BackendProp, FrontendProp)
        {
            auto property_value = typename BackendProp::type{};

            auto backup = This();
            bell::signal(tier::request, BackendProp{},  property_value);
            bell::signal(tier::anycast, FrontendProp{}, property_value);

            LISTEN(tier::release, BackendProp{}, property_value)
            {
                this->bell::signal(tier::anycast, FrontendProp{}, property_value);
            };
            return backup;
        }
        auto limits(twod new_min_sz = -dot_11, twod new_max_sz = -dot_11)
        {
            base::limits(new_min_sz, new_max_sz);
            return This();
        }
        auto alignment(bind new_atgrow, bind new_atcrop = { snap::none, snap::none })
        {
            base::alignment(new_atgrow, new_atcrop);
            return This();
        }
        auto setpad(dent new_intpad, dent new_extpad = {})
        {
            base::setpad(new_intpad, new_extpad);
            return This();
        }
        auto nested_context(auto& parent_canvas)
        {
            auto basis = rect{ dot_00, base::region.size } - base::intpad;
            auto context = parent_canvas.change_basis(basis, true);
            return context;
        }
    };

    // controls: Splitter.
    class fork
        : public form<fork>
    {
        sptr& object_1; // fork: 1st object.
        sptr& object_2; // fork: 2nd object.
        sptr& splitter; // fork: Resizing grip object.
        rect  griparea; // fork: Resizing grip region.
        axis  rotation; // fork: Fork orientation.
        si32  fraction; // fork: Ratio between objects.
        bool  adaptive; // fork: Fixed ratio.

        auto xpose(twod p)
        {
            return rotation == axis::X ? p : twod{ p.y, p.x };
        }
        void _config(axis orientation, si32 grip_width, si32 s1 = 1, si32 s2 = 1)
        {
            rotation = orientation;
            griparea.size = xpose({ std::max(0, grip_width), 0 });
            _config_ratio(s1, s2);
        }
        void _config_ratio(si32 s1, si32 s2)
        {
            if (s1 < 0) s1 = 0;
            if (s2 < 0) s2 = 0;
            auto sum = s1 + s2;
            fraction = sum ? netxs::divround(s1 * max_ratio, sum)
                           : max_ratio >> 1;
        }

    protected:
        // fork: .
        void deform(rect& new_area) override
        {
            auto region_1 = rect{};
            auto region_2 = rect{};
            auto region_3 = griparea;
            auto meter = [&](auto& newsz_x, auto& newsz_y,
                             auto& size1_x, auto& size1_y,
                             auto& coor2_x, auto& coor2_y,
                             auto& size2_x, auto& size2_y,
                             auto& coor3_x, auto& coor3_y,
                             auto& size3_x, auto& size3_y)
            {
                auto limit_x = std::max(newsz_x - size3_x, 0);
                auto split_x = netxs::divround(limit_x * fraction, max_ratio);
                auto test = [&]
                {
                    size1_x = split_x;
                    size1_y = newsz_y;
                    if (object_1)
                    {
                        object_1->base::recalc(region_1);
                        split_x = size1_x;
                        newsz_y = size1_y;
                    }
                    size2_x = limit_x - split_x;
                    size2_y = newsz_y;
                    coor2_x = split_x + size3_x;
                    coor2_y = 0;
                    auto test_size2 = region_2.size;
                    if (object_2)
                    {
                        object_2->base::recalc(region_2);
                        newsz_y = size2_y;
                    }
                    return test_size2 == region_2.size;
                };
                auto ok = test();
                split_x = newsz_x - size3_x - size2_x;
                if (!ok) test(); // Repeat if object_2 doesn't fit.
                coor3_x = split_x;
                coor3_y = 0;
                size3_y = newsz_y;
                if (adaptive) _config_ratio(split_x, size2_x);
                newsz_x = split_x + size3_x + size2_x;
            };
            auto& new_size = new_area.size;
            rotation == axis::X ? meter(new_size.x, new_size.y, region_1.size.x, region_1.size.y, region_2.coor.x, region_2.coor.y, region_2.size.x, region_2.size.y, region_3.coor.x, region_3.coor.y, region_3.size.x, region_3.size.y)
                                : meter(new_size.y, new_size.x, region_1.size.y, region_1.size.x, region_2.coor.y, region_2.coor.x, region_2.size.y, region_2.size.x, region_3.coor.y, region_3.coor.x, region_3.size.y, region_3.size.x);
            if (splitter) splitter->base::recalc(region_3);
            griparea = region_3;
        }
        // fork: .
        void inform(rect new_area) override
        {
            auto corner_2 = twod{ griparea.coor.x + griparea.size.x, 0 };
            auto region_1 = rect{ dot_00, xpose({ griparea.coor.x, griparea.size.y })};
            auto region_2 = rect{ xpose(corner_2), xpose({ new_area.size.x - corner_2.x, griparea.size.y })};
            auto region_3 = griparea;
            region_1.coor += new_area.coor;
            region_2.coor += new_area.coor;
            region_3.coor += new_area.coor;
            if (object_1) object_1->base::notify(region_1);
            if (object_2) object_2->base::notify(region_2);
            if (splitter) splitter->base::notify(region_3);
            adaptive = faux;
        }

    public:
        fork(axis orientation = axis::X, si32 grip_width = 0, si32 s1 = 1, si32 s2 = 1)
            : form{ 3 },
              object_1{ base::subset[0] },
              object_2{ base::subset[1] },
              splitter{ base::subset[2] },
              rotation{ },
              fraction{ },
              adaptive{ }
        {
            _config(orientation, grip_width, s1, s2);
            LISTEN(tier::preview, e2::form::layout::swarp, warp)
            {
                adaptive = true; // Adjust the grip ratio on coming resize.
                this->bell::expire(tier::preview, true);
            };
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (auto context = form::nested_context(parent_canvas))
                {
                    if (splitter) splitter->render(parent_canvas);
                    if (object_1) object_1->render(parent_canvas);
                    if (object_2) object_2->render(parent_canvas);
                }
            };
        }

        static constexpr auto min_ratio = si32{ 0           };
        static constexpr auto max_ratio = si32{ 0xFFFF      };
        static constexpr auto mid_ratio = si32{ 0xFFFF >> 1 };

        // fork: .
        auto get_ratio()
        {
            return fraction;
        }
        // fork: .
        auto set_ratio(si32 new_ratio = max_ratio)
        {
            fraction = new_ratio;
        }
        // fork: .
        void config(si32 s1, si32 s2 = 1)
        {
            _config_ratio(s1, s2);
            base::reflow();
        }
        // fork: .
        auto config(axis orientation, si32 grip_width, si32 s1, si32 s2)
        {
            _config(orientation, grip_width, s1, s2);
            return This();
        }
        // fork: .
        void rotate()
        {
            auto width = xpose(griparea.size).x;
            rotation = (axis)!rotation;
                 if (rotation == axis::Y && width == 2) width = 1;
            else if (rotation == axis::X && width == 1) width = 2;
            (rotation == axis::X ? griparea.size.x : griparea.size.y) = width;
            base::reflow();
        }
        // fork: .
        void swap()
        {
            std::swap(object_1, object_2);
            base::reflow();
        }
        // fork: .
        void move_slider(si32 step)
        {
            if (splitter)
            {
                auto delta = griparea.size * xpose({ step, 0 });
                splitter->bell::signal(tier::preview, e2::form::upon::changed, delta);
            }
        }
        // fork: .
        auto attach(slot Slot, auto item_ptr)
        {
            if (Slot == slot::_1)
            {
                if (object_1) remove(object_1);
                object_1 = item_ptr;
            }
            else if (Slot == slot::_2)
            {
                if (object_2) remove(object_2);
                object_2 = item_ptr;
            }
            else if (Slot == slot::_I)
            {
                if (splitter) remove(splitter);
                splitter = item_ptr;
                splitter->LISTEN(tier::preview, e2::form::upon::changed, delta)
                {
                    auto split = xpose(griparea.coor + delta).x;
                    auto limit = xpose(base::size() - griparea.size).x;
                    fraction = netxs::divround(max_ratio * split, limit);
                    this->base::reflow();
                };
            }
            item_ptr->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            return item_ptr;
        }
        // fork: Remove nested object by it's ptr.
        void remove(sptr item_ptr) override
        {
            if (object_1 == item_ptr ? ((void)object_1.reset(), true) :
                object_2 == item_ptr ? ((void)object_2.reset(), true) :
                splitter == item_ptr ? ((void)splitter.reset(), true) : faux)
            {
                auto backup = This();
                item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
            }
        }
    };

    // controls: Vertical/horizontal list.
    class list
        : public form<list>
    {
        bool updown; // list: List orientation, true: vertical(default), faux: horizontal.
        sort lineup; // list: Attachment order.

    protected:
        // list: .
        void deform(rect& new_area) override
        {
            auto& object_area = new_area;
            auto& new_size = object_area.size;
            auto& height = object_area.coor[updown];
            auto& y_size = new_size[updown];
            auto& x_size = new_size[1 - updown];
            auto  x_temp = x_size;
            auto start = height;
            auto meter = [&]
            {
                height = start;
                for (auto& object : subset)
                {
                    if (!object || object->base::hidden) continue;
                    auto& entry = *object;
                    y_size = 0;
                    entry.base::recalc(object_area);
                    if (x_size > x_temp) x_temp = x_size;
                    else                 x_size = x_temp;
                    height += entry.base::socket.size[updown];
                }
            };
            meter(); if (subset.size() > 1 && x_temp != x_size) meter();
            y_size = height;
        }
        // list: .
        void inform(rect new_area) override
        {
            auto object_area = new_area;
            auto& size_y = object_area.size[updown];
            auto& coor_y = object_area.coor[updown];
            auto& lock_y = base::anchor[updown];
            auto found = faux;
            for (auto& object : subset)
            {
                if (!object || object->base::hidden) continue;
                auto& entry = *object;
                if (!found) // Looking for anchored list entry.
                {
                    auto anker = entry.base::area() + entry.base::extpad; // Use old entry position.
                    auto anker_coor_y = anker.coor[updown];
                    auto anker_size_y = anker.size[updown];
                    if (lock_y < anker_coor_y + anker_size_y || lock_y < anker_coor_y)
                    {
                        base::anchor += object_area.coor - anker.coor;
                        found = true;
                    }
                }
                size_y = entry.base::socket.size[updown];
                entry.base::notify(object_area);
                coor_y += size_y;
            }
        }

    public:
        list(axis orientation = axis::Y, sort attach_order = sort::forward)
            : updown{ orientation == axis::Y },
              lineup{ attach_order }
        {
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (auto context = form::nested_context(parent_canvas))
                {
                    auto basis = parent_canvas.full();
                    auto frame = parent_canvas.clip();
                    auto min_y = frame.coor[updown] - basis.coor[updown];
                    auto max_y = frame.size[updown] + min_y;
                    auto bound = [xy = updown](auto& o){ return o ? o->base::region.coor[xy] + o->base::region.size[xy] : -dot_mx.y; };
                    auto start = std::ranges::lower_bound(base::subset, min_y, {}, bound);
                    while (start != base::subset.end())
                    {
                        if (auto& object = *start++)
                        {
                            object->render(parent_canvas);
                            if (!object->base::hidden && bound(object) >= max_y) break;
                        }
                    }
                }
            };
        }
        // list: .
        void clear()
        {
            auto backup = This();
            while (subset.size())
            {
                auto item_ptr = subset.back();
                subset.pop_back();
                item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
            }
        }
        // list: Remove the last nested object. Return the object refrence.
        auto pop_back()
        {
            if (subset.size())
            {
                auto object = subset.back();
                auto backup = This();
                subset.pop_back();
                object->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
                return object;
            }
            return sptr{};
        }
        // list: Attach specified item.
        template<sort Order = sort::forward>
        auto attach(auto object)
        {
            auto order = Order == sort::forward ? lineup : lineup == sort::reverse ? sort::forward : sort::reverse;
            if (order == sort::reverse) subset.insert(subset.begin(), object);
            else                        subset.push_back(object);
            object->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            return object;
        }
    };

    // controls: 2D grid.
    class grid
        : public form<grid>
    {
        struct elem
        {
            twod coor; // elem: Grid cell coordinates for placing the object.
            twod span; // elem: The number of adjacent grid cells occupied by the object.
            rect area; // elem: Object slot.
            bool done; // elem: Object resized.
        };
        struct cell
        {
            twod size; // Cell size.
            si32 span; // Cell span.
        };
        template<bool SetInner = faux>
        auto cellsz(twod coor, twod span)
        {
            auto size = coor + span;
            auto x = std::accumulate(widths.begin() + coor.x, widths.begin() + size.x, 0, [](auto x, auto& w){ return x += w.size.x; });
            auto y = std::accumulate(widths.begin() + coor.y, widths.begin() + size.y, 0, [](auto y, auto& w){ return y += w.size.y; });
            return twod{ x, y };
        }
        std::vector<cell> widths;  // grid: Grid cell metrics.
        std::vector<elem> blocks;  // grid: Geometry of stored objects.

    protected:
        // grid: .
        void deform(rect& new_area) override
        {
            widths.clear();
            auto m = dot_00;
            auto first_run = true;
            auto recalc = [&](auto object_iter, auto tail2, auto elem_iter)
            {
                auto changed = faux;
                while (object_iter != tail2)
                {
                    auto& object = *(*object_iter++);
                    auto& elem = *elem_iter++;
                    if (elem.span.x < 1 || elem.span.y < 1) continue;
                    auto dimension = elem.coor + elem.span;
                    auto max_len = std::max(dimension.x, dimension.y);
                    if (max_len > (si32)widths.size())
                    {
                        m = std::max(m, dimension);
                        widths.resize(max_len);
                    }
                    auto area_size = cellsz<true>(elem.coor, elem.span);
                    if (first_run)
                    {
                        widths[elem.coor.x].span = std::max(elem.span.x, widths[elem.coor.x].span);
                        widths[elem.coor.y].span = std::max(elem.span.y, widths[elem.coor.y].span);
                    }
                    if (elem.done && elem.area.size == area_size) continue;
                    elem.area.size = area_size;
                    elem.done = true;
                    object.base::recalc(elem.area);
                    auto delta = elem.area.size - area_size;
                    if (delta.x > 0)
                    {
                        changed = true;
                        auto head = widths.begin() + elem.coor.x;
                        auto tail = head + widths[elem.coor.x].span;
                        auto iter = head + elem.span.x - 1;
                        (*iter++).size.x += delta.x;
                        while (delta.x && iter != tail)
                        {
                            auto& w = (*iter++).size.x;
                            auto dx = std::min(w, delta.x);
                            w -= dx;
                            delta.x -= dx;
                        }
                    }
                    if (delta.y > 0)
                    {
                        changed = true;
                        auto head = widths.begin() + elem.coor.y;
                        auto tail = head + widths[elem.coor.y].span;
                        auto iter = head + elem.span.y - 1;
                        (*iter++).size.y += delta.y;
                        while (delta.y && iter != tail)
                        {
                            auto& h = (*iter++).size.y;
                            auto dy = std::min(h, delta.y);
                            h -= dy;
                            delta.y -= dy;
                        }
                    }
                }
                return changed;
            };
            auto recoor = [&](auto object_iter, auto tail, auto elem_iter)
            {
                while (object_iter != tail)
                {
                    auto& object = *(*object_iter++);
                    auto& elem = *elem_iter++;
                    elem.area.coor = cellsz(dot_00, elem.coor);
                    elem.done = {};
                    object.base::recalc(elem.area);
                }
            };
            while (recalc(subset.rbegin(), subset.rend(), blocks.rbegin()))
            { }
            recoor(subset.begin(), subset.end(), blocks.begin());
            new_area.size = std::max(new_area.size, cellsz(dot_00, m));
        }
        // grid: .
        void inform(rect /*new_area*/) override
        {
            auto elem_ptr = blocks.begin();
            for (auto& object : subset)
            {
                auto& elem = *elem_ptr++;
                object->base::notify(elem.area);
            }
        }

    public:
        grid()
        {
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (auto context = form::nested_context(parent_canvas))
                {
                    for (auto& object : subset)
                    {
                        object->render(parent_canvas);
                    }
                }
            };
        }
        // grid: .
        void clear()
        {
            auto backup = This();
            while (subset.size())
            {
                auto item_ptr = subset.back();
                subset.pop_back();
                item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
            }
            blocks.clear();
        }
        // grid: Remove specified object by index.
        //auto remove_by_index(si32 index)
        //{
        //    if (index < subset.size())
        //    {
        //        auto object = subset[index];
        //        auto backup = This();
        //        subset.erase(subset.begin() + index);
        //        blocks.erase(blocks.begin() + index);
        //        object->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
        //        return object;
        //    }
        //    return sptr{};
        //}
        // grid: Return specified object with its geometry by index.
        auto get_item(si32 index)
        {
            if (index < (si32)subset.size())
            {
                return std::pair{ subset[index], blocks[index] };
            }
            return std::pair{ sptr{}, elem{} };
        }
        // grid: Attach specified item.
        auto attach(auto object, elem conf = { .span = dot_11 })
        {
            blocks.push_back(conf);
            subset.push_back(object);
            object->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            return object;
        }
        // grid: Attach item grid.
        void attach_cells(twod size, std::vector<sptr> object_list)
        {
            auto conf = elem{ .span = dot_11, .done = faux };
            auto temp = std::exchange(base::hidden, true); // Suppress reflowing during attaching.
            for (auto object : object_list)
            {
                if (object) attach(object, conf);
                if (++conf.coor.x == size.x)
                {
                    conf.coor.x = 0;
                    if (++conf.coor.y == size.y) break;
                }
            }
            base::hidden = temp;
        }
        // grid: Remove nested object.
        void remove(sptr item_ptr) override
        {
            auto head = subset.begin();
            auto tail = subset.end();
            auto iter = std::find_if(head, tail, [&](auto& c){ return c == item_ptr; });
            if (iter != tail)
            {
                auto backup = This();
                blocks.erase(blocks.begin() + std::distance(subset.begin(), iter));
                subset.erase(iter);
                item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
            }
        }
    };

    // controls: Layered cake of objects on top of each other.
    class cake
        : public form<cake>
    {
    protected: 
        // cake: .
        void deform(rect& new_area) override
        {
            auto new_coor = new_area.coor;
            auto new_size = new_area.size;
            auto meter = [&]
            {
                for (auto& object : subset)
                {
                    object->base::recalc(new_area);
                    new_area.coor = new_coor;
                }
            };
            meter();
            if (subset.size() > 1 && new_size != new_area.size)
            {
                meter();
            }
        }
        // cake: .
        void inform(rect new_area) override
        {
            for (auto& object : subset)
            {
                object->base::notify(new_area);
            }
        }

    public:
        cake()
        {
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (auto context = form::nested_context(parent_canvas))
                {
                    for (auto& object : subset)
                    {
                        object->render(parent_canvas);
                    }
                }
            };
        }
        // cake: Remove the last nested object. Return the object refrence.
        auto pop_back()
        {
            if (subset.size())
            {
                auto object = subset.back();
                auto backup = This();
                subset.pop_back();
                object->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
                return object;
            }
            return sptr{};
        }
        // cake: Create a new item of the specified subtype and attach it.
        auto attach(auto object)
        {
            if (object)
            {
                subset.push_back(object);
                object->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            }
            return object;
        }
    };

    // controls: Container for multiple objects, but only the last one is shown.
    class veer
        : public form<veer>
    {
    protected:
        // veer: .
        void deform(rect& new_area) override
        {
            if (subset.size())
            if (auto object = subset.back())
            {
                object->base::recalc(new_area);
            }
        }
        // veer: .
        void inform(rect new_area) override
        {
            if (subset.size())
            if (auto object = subset.back())
            {
                object->base::notify(new_area);
            }
        }

    public:
        veer()
        {
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (auto context = form::nested_context(parent_canvas))
                {
                    if (subset.size())
                    if (auto object = subset.back())
                    {
                        object->render(parent_canvas);
                    }
                }
            };
        }
        // veer: Return the last object or empty sptr.
        auto back()
        {
            return subset.size() ? subset.back()
                                 : sptr{};
        }
        // veer: Return the first object or empty sptr.
        auto front()
        {
            return subset.size() ? subset.front()
                                 : sptr{};
        }
        // veer: Return nested objects count.
        auto count()
        {
            return subset.size();
        }
        // veer: Return true if empty.
        auto empty()
        {
            return subset.empty();
        }
        // veer: Remove the last object. Return the object refrence.
        auto pop_back()
        {
            if (subset.size())
            {
                auto object = subset.back();
                auto backup = This();
                subset.pop_back();
                object->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
                return object;
            }
            return sptr{};
        }
        // veer: Roll objects.
        void roll(si32 dt = 1)
        {
            if (dt && subset.size() > 1)
            {
                if (dt > 0) while (dt--)
                {
                    subset.insert(subset.begin(), subset.back());
                    subset.pop_back();
                }
                else while (dt++)
                {
                    subset.push_back(subset.front());
                    subset.erase(subset.begin());
                }
            }
        }
        // veer: Create a new item of the specified subtype and attach it.
        auto attach(auto object)
        {
            subset.push_back(object);
            object->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            return object;
        }
    };

    // controls: Text page.
    template<auto fx>
    class postfx
        : public flow, public form<postfx<fx>>
    {
        twod square; // post: Page area.
        text source; // post: Text source.
        bool beyond; // post: Allow vertical scrolling beyond the last line.
        bool recent; // post: Paragraphs are not aligned.

    protected:
        // post: .
        void deform(rect& new_area) override
        {
            square = new_area.size;
            flow::reset();
            if (recent) // Update new paragraph's coords before resize.
            {
                auto publish = [&](auto& combo)
                {
                    combo.coord = flow::print(combo);
                };
                topic.stream(publish);
                recent = faux;
            }
            else // Sync anchor.
            {
                auto entry = topic.lookup(base::anchor);
                auto publish = [&](auto& combo)
                {
                    combo.coord = flow::print(combo);
                    if (combo.id() == entry.id) entry.coor.y -= combo.coord.y;
                };
                topic.stream(publish);
                // Apply only vertical anchoring for this type of control.
                base::anchor.y -= entry.coor.y; // Move the central point accordingly to the anchored object
            }
            auto cover = flow::minmax();
            base::oversz = { -std::min(0, cover.coor.x),
                              std::max(0, cover.coor.x + cover.size.x - square.x),
                             -std::min(0, cover.coor.y),
                              0 };
            auto height = cover.size.y;
            if (beyond) square.y += height - 1;
            else        square.y  = height;
            new_area.size.y = square.y;
        }
        // post: .
        void inform(rect new_area) override
        {
            if (square.x != new_area.size.x)
            {
                deform(new_area);
            }
        }

    public:
        page topic; // post: Text content.

        postfx(bool scroll_beyond = faux)
            :   flow{ square        },
              beyond{ scroll_beyond },
              recent{               }
        {
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                output(parent_canvas);
                //auto mark = rect{ base::anchor + base::coor(), {10,5} };
                //mark.coor += parent_canvas.clip().coor; // Set client's basis
                //parent_canvas.fill(mark, [](cell& c){ c.alpha(0x80).bgc().chan.r = 0xff; });
            };
        }
        // post: .
        auto& lyric(si32 paraid) { return *topic[paraid].lyric; }
        // post: .
        auto& content(si32 paraid) { return topic[paraid]; }
        // post: .
        auto upload(view utf8, si32 initial_width = 0) // Don't use cell link id here. Apply it to the parent (with a whole rect coverage).
        {
            recent = true;
            source = utf8;
            topic = utf8;
            if (initial_width < 0)
            {
                initial_width = topic.limits().x + base::intpad.l + base::intpad.r;
                base::limits({ initial_width, -1 });
            }
            base::resize(twod{ initial_width, 0 });
            base::reflow();
            return this->This();
        }
        // post: .
        auto& get_source() const
        {
            return source;
        }
        // post: .
        void output(face& canvas)
        {
            flow::reset(canvas, base::intpad.corner());
            auto publish = [&](auto const& combo)
            {
                flow::print2(combo, canvas, fx);
            };
            topic.stream(publish);
        }
    };

    using post = postfx<noop{}>;

    // controls: Scroller.
    class rail
        : public form<rail>
    {
        pro::robot robot{*this }; // rail: Animation controller.

        using upon = e2::form::upon;

        twod permit; // rail: Allowed axes to scroll.
        twod siezed; // rail: Allowed axes to capture.
        twod oversc; // rail: Allow overscroll with auto correct.
        twod strict; // rail: Don't allow overscroll.
        twod manual; // rail: Manual scrolling (no auto align).
        bool animat; // rail: Smooth scrolling.
        subs fasten; // rail: Subscriptions on masters to follow they state.
        rack scinfo; // rail: Scroll info.
        fp2d drag_origin; // rail: Drag origin.

        si32 spd       = skin::globals().spd;
        si32 pls       = skin::globals().pls;
        si32 ccl       = skin::globals().ccl;
        si32 spd_accel = skin::globals().spd_accel;
        si32 ccl_accel = skin::globals().ccl_accel;
        si32 spd_max   = skin::globals().spd_max;
        si32 ccl_max   = skin::globals().ccl_max;
        si32 switching = datetime::round<si32>(skin::globals().switching);

        si32 speed{ spd  }; // rail: Text auto-scroll initial speed component ΔR.
        si32 pulse{ pls  }; // rail: Text auto-scroll initial speed component ΔT.
        si32 cycle{ ccl  }; // rail: Text auto-scroll duration in ms.
        bool steer{ faux }; // rail: Text scroll vertical direction.

        // rail: .
        static constexpr auto xy(axes Axes)
        {
            return twod{ !!(Axes & axes::X_only), !!(Axes & axes::Y_only) };
        }
        // rail: .
        bool empty() //todo VS2019 requires bool
        {
            return base::subset.empty() || !base::subset.back();
        }

    protected:
        // rail: Resize nested object with scroll bounds checking.
        void inform(rect new_area) override
        {
            if (empty()) return;
            auto& item = *base::subset.back();
            item.base::anchor = base::anchor - item.base::region.coor;
            auto block = item.base::resize(new_area.size - item.base::extpad, faux);
            auto frame = new_area.size;
            auto delta = dot_00;
            revise(item, block, frame, delta);
            block += item.base::extpad;
            item.base::socket = block;
            item.base::accept(block);
        }

    public:
        rail(axes allow_to_scroll = axes::all, axes allow_to_capture = axes::all, axes allow_overscroll = axes::all, bool smooth_scrolling = faux)
            : permit{ xy(allow_to_scroll)  },
              siezed{ xy(allow_to_capture) },
              oversc{ xy(allow_overscroll) },
              strict{ xy(axes::all) },
              manual{ xy(axes::all) },
              animat{ smooth_scrolling }
        {
            LISTEN(tier::preview, e2::form::upon::scroll::any, info) // Receive scroll parameters from external sources.
            {
                auto delta = dot_00;
                switch (this->bell::protos(tier::preview))
                {
                    case upon::scroll::bycoor::v.id: delta = { scinfo.window.coor - info.window.coor };        break;
                    case upon::scroll::bycoor::x.id: delta = { scinfo.window.coor.x - info.window.coor.x, 0 }; break;
                    case upon::scroll::bycoor::y.id: delta = { 0, scinfo.window.coor.y - info.window.coor.y }; break;
                    case upon::scroll::to_top::v.id: delta = { dot_mx };                                       break;
                    case upon::scroll::to_top::x.id: delta = { dot_mx.x, 0 };                                  break;
                    case upon::scroll::to_top::y.id: delta = { 0, dot_mx.y };                                  break;
                    case upon::scroll::to_end::v.id: delta = { -dot_mx };                                      break;
                    case upon::scroll::to_end::x.id: delta = { -dot_mx.x, 0 };                                 break;
                    case upon::scroll::to_end::y.id: delta = { 0, -dot_mx.y };                                 break;
                    case upon::scroll::bystep::v.id: delta = { info.vector };                                  break;
                    case upon::scroll::bystep::x.id: delta = { info.vector.x, 0 };                             break;
                    case upon::scroll::bystep::y.id: delta = { 0, info.vector.y };                             break;
                    case upon::scroll::bypage::v.id: delta = { info.vector * scinfo.window.size };             break;
                    case upon::scroll::bypage::x.id: delta = { info.vector.x * scinfo.window.size.x, 0 };      break;
                    case upon::scroll::bypage::y.id: delta = { 0, info.vector.y * scinfo.window.size.y };      break;
                    case upon::scroll::cancel::v.id: cancel<X, true>(); cancel<Y, true>();                     break;
                    case upon::scroll::cancel::x.id: cancel<X, true>();                                        break;
                    case upon::scroll::cancel::y.id: cancel<Y, true>();                                        break;
                    default:                         break;
                }
                if (delta) scroll(delta);
            };
            LISTEN(tier::request, e2::form::upon::scroll::any, req_scinfo)
            {
                req_scinfo = scinfo;
            };

            using button = hids::events::mouse::button;
            LISTEN(tier::release, hids::events::mouse::scroll::act, gear)
            {
                if (gear.meta(hids::anyCtrl)) return; // Ctrl+Wheel is reserved for zooming.
                if (gear.whlsi)
                {
                    auto hz = (permit[X] && (gear.hzwhl || gear.meta(hids::anyAlt | hids::anyShift)))
                           || (permit == xy(axes::X_only));
                    if (hz) wheels<X>(gear.whlsi);
                    else    wheels<Y>(gear.whlsi);
                }
                gear.dismiss();
            };
            LISTEN(tier::release, button::drag::start::right, gear)
            {
                auto ds = gear.delta.get();
                auto dx = ds.x;
                auto dy = ds.y * 2;
                auto vt = std::abs(dx) < std::abs(dy);

                if ((siezed[X] && !vt)
                 || (siezed[Y] &&  vt))
                {
                    if (gear.capture(bell::id))
                    {
                        drag_origin = gear.coord;
                        manual = xy(axes::all);
                        strict = xy(axes::all) - oversc; // !oversc = dot_11 - oversc
                        gear.dismiss();
                    }
                }
            };
            LISTEN(tier::release, button::drag::pull::right, gear)
            {
                if (gear.captured(bell::id))
                {
                    if (auto delta = twod{ gear.coord } - twod{ drag_origin })
                    {
                        drag_origin = gear.coord;
                        auto value = permit * delta;
                        if (value) scroll(value);
                    }
                    gear.dismiss();
                }
            };
            LISTEN(tier::release, button::drag::cancel::right, gear)
            {
                if (gear.captured(bell::id))
                {
                    giveup(gear);
                }
            };
            LISTEN(tier::general, hids::events::halt, gear)
            {
                if (gear.captured(bell::id))
                {
                    giveup(gear);
                }
            };
            LISTEN(tier::release, button::drag::stop::right, gear)
            {
                if (gear.captured(bell::id))
                {
                    auto  v0 = gear.delta.avg();
                    auto& speed = v0.dS;
                    auto  cycle = datetime::round<si32>(v0.dT);
                    auto  limit = datetime::round<si32>(skin::globals().deceleration);
                    auto  start = 0;

                    if (permit[X]) actify<X>(quadratic{ speed.x, cycle, limit, start });
                    if (permit[Y]) actify<Y>(quadratic{ speed.y, cycle, limit, start });
                    //todo if (permit == xy(axes::all)) actify(quadratic{ speed, cycle, limit, start });

                    base::deface();
                    gear.setfree();
                    gear.dismiss();
                }
            };
            LISTEN(tier::release, button::click::right, gear)
            {
                if (!gear.captured(bell::id))
                {
                    if (manual[X]) cancel<X, true>();
                    if (manual[Y]) cancel<Y, true>();
                }
            };
            LISTEN(tier::release, button::down::any, gear)
            {
                cutoff();
            };
            LISTEN(tier::release, e2::form::animate::reset, task_id)
            {
                cutoff();
            };
            LISTEN(tier::release, e2::form::animate::stop, task_id)
            {
                switch (task_id)
                {
                    case Y: manual[Y] = true; /*scroll<Y>();*/ break;
                    case X: manual[X] = true; /*scroll<X>();*/ break;
                    default: break;
                }
                base::deface();
            };
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (empty()) return;
                if (auto context = form::nested_context(parent_canvas))
                {
                    auto& item = *base::subset.back();
                    item.render(parent_canvas, faux);
                }
            };
        }

        // rail: .
        auto smooth(bool smooth_scroll = true)
        {
            animat = smooth_scroll;
            return This();
        }
        // rail: .
        template<axis Axis>
        auto follow(sptr master = {})
        {
            if (master)
            {
                master->LISTEN(tier::release, upon::scroll::bycoor::any, master_scinfo, fasten)
                {
                    auto backup_scinfo = master_scinfo;
                    this->bell::signal(tier::preview, e2::form::upon::scroll::bycoor::_<Axis>, backup_scinfo);
                };
            }
            else fasten.clear();
            return This();
        }
        // rail: .
        void cutoff()
        {
            if (manual[X]) robot.pacify(X);
            if (manual[Y]) robot.pacify(Y);
        }
        // rail: .
        void giveup(hids& gear)
        {
            cancel<X>();
            cancel<Y>();
            base::deface();
            gear.setfree();
            gear.dismiss();
        }
        // rail: .
        template<axis Axis>
        void wheels(si32 step)
        {
            auto dir = step > 0;
            if (animat)
            {
                if (robot.active(Axis) && (steer == dir))
                {
                    speed += spd_accel;
                    cycle += ccl_accel;
                    speed = std::min(speed, spd_max);
                    cycle = std::min(cycle, ccl_max);
                }
                else
                {
                    steer = dir;
                    speed = spd;
                    cycle = ccl;
                    //todo at least one line should be
                    //move<Axis>(dir ? 1 : -1);
                }
                auto start = 0;
                auto boost = dir ? speed : -speed;
                if constexpr (Axis == X) boost *= 2;
                keepon<Axis>(quadratic<si32>(boost, pulse, cycle, start));
            }
            else
            {
                auto delta = Axis == X ? twod{ step, 0 }
                                       : twod{ 0, step };
                scroll(delta);
            }
        }
        // rail: .
        template<axis Axis, class Fx>
        void keepon(Fx&& func)
        {
            strict[Axis] = true;
            robot.actify(Axis, std::forward<Fx>(func), [&](auto& p)
            {
                //todo revise
                //if (auto step = netxs::saturate_cast<twod::type>(p))
                if (auto step = twod::cast(p))
                {
                    auto delta = Axis == X ? twod{ step, 0 }
                                           : twod{ 0, step };
                    scroll(delta);
                }
            });
        }
        // rail: Check overscroll if no auto correction.
        template<axis Axis>
        auto inside()
        {
            if (empty() || !manual[Axis]) return true;
            auto& item = *base::subset.back();
            auto frame = (base::size() - base::intpad)[Axis];
            auto block = item.base::area() + item.base::oversz;
            auto coord = block.coor[Axis];
            auto bound = std::min(frame - block.size[Axis], 0);
            auto clamp = std::clamp(coord, bound, 0);
            return clamp == coord;
        }
        // rail: .
        template<axis Axis, class Fx>
        void actify(Fx&& func)
        {
            if (inside<Axis>()) keepon<Axis>(std::forward<Fx>(func));
            else                lineup<Axis>();
        }
        // rail: .
        template<axis Axis, bool Forced = faux>
        void cancel()
        {
            auto correct = Forced || !inside<Axis>();
            if (correct) lineup<Axis>();
        }
        // rail: .
        template<axis Axis>
        void lineup()
        {
            if (empty()) return;
            manual[Axis] = faux;
            auto& item = *base::subset.back();
            auto block = item.base::area();
            auto coord = block.coor[Axis];
            auto width = block.size[Axis];
            auto frame = (base::size() - base::intpad)[Axis];
            auto bound = std::min(frame - width, 0);
            auto newxy = std::clamp(coord, bound, 0);
            auto route = newxy - coord;
            auto tempo = switching;
            auto start = 0;
            auto fader = constlinearAtoB<si32>(route, tempo, start);
            keepon<Axis>(fader);
        }
        void revise(base& item, rect& block, twod frame, twod& delta)
        {
            auto coord = block.coor;
            auto width = block.size;
            auto basis = base::intpad.corner() + item.base::oversz.corner();
            frame -= base::intpad;
            coord -= basis; // Scroll origin basis.
            coord += delta;
            width += item.base::oversz;
            auto bound = std::min(frame - width, dot_00);
            auto clamp = std::clamp(coord, bound, dot_00);
            for (auto xy : { axis::X, axis::Y }) // Check overscroll if no auto correction.
            {
                if (coord[xy] != clamp[xy] && manual[xy] && strict[xy]) // Clamp if it is outside the scroll limits and no overscroll.
                {
                    delta[xy] = clamp[xy] - coord[xy];
                    coord[xy] = clamp[xy];
                }
            }
            coord += basis; // Object origin basis.
            block.coor = coord;
        }
        // rail: .
        void scroll(twod& delta)
        {
            if (empty()) return;
            auto& item = *base::subset.back();
            auto frame = base::size();
            auto block = item.base::area();
            revise(item, block, frame, delta);
            item.base::moveto(block.coor);
            base::deface();
        }
        // rail: Attach specified item.
        auto attach(auto object)
        {
            if (!empty()) remove(base::subset.back());
            base::subset.push_back(object);
            object->bell::signal(tier::release, e2::form::upon::vtree::attached, This());
            object->LISTEN(tier::release, e2::area, new_area, object->relyon) // Sync scroll info.
            {
                if (empty()) return;
                auto& item = *base::subset.back();
                auto frame = base::socket.size - base::extpad - base::intpad;
                auto coord = new_area.coor;
                auto block = new_area.size + item.base::oversz;
                auto basis = base::intpad.corner() + item.base::oversz.corner();
                coord -= basis; // Scroll origin basis.
                scinfo.beyond = item.base::oversz;
                scinfo.region = block;
                scinfo.window.coor =-coord; // Viewport.
                scinfo.window.size = frame; //
                this->bell::signal(tier::release, upon::scroll::bycoor::any, scinfo);
            };
            return object;
        }
        // rail: Detach specified object.
        void remove(sptr object) override
        {
            if (!empty() && base::subset.back() == object)
            {
                auto backup = This();
                base::subset.pop_back();
                object->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
                scinfo.region = {};
                scinfo.window.coor = {};
                this->bell::signal(tier::release, upon::scroll::bycoor::any, scinfo); // Reset dependent scrollbars.
                fasten.clear();
            }
            else base::subset.clear();
        }
        // rail: Update nested object.
        void replace(sptr old_object, sptr new_object) override
        {
            auto object_coor = dot_00;
            if (!empty())
            {
                auto object = base::subset.back();
                object_coor = object->base::coor();
                remove(old_object);
            }
            if (new_object)
            {
                new_object->base::moveto(object_coor);
                attach(new_object);
            }
        }
    };

    // controls: Scrollbar.
    template<axis Axis, auto drawfx>
    class gripfx
        : public flow, public form<gripfx<Axis, drawfx>>
    {
        pro::timer timer{*this }; // grip: Minimize by timeout.

        using form = ui::form<gripfx<Axis, drawfx>>;
        using upon = e2::form::upon;

        enum activity
        {
            mouse_leave = 0, // faux
            mouse_hover = 1, // true
            pager_first = 10,
            pager_next  = 11,
        };

        struct math
        {
            rack  master_inf = {};                           // math: Master scroll info.
            si32& master_dir = master_inf.vector     [Axis]; // math: Master scroll direction.
            si32& master_len = master_inf.region     [Axis]; // math: Master len.
            si32& master_pos = master_inf.window.coor[Axis]; // math: Master viewport pos.
            si32& master_box = master_inf.window.size[Axis]; // math: Master viewport len.
            si32  scroll_len = 0; // math: Scrollbar len.
            si32  scroll_pos = 0; // math: Scrollbar grip pos.
            si32  scroll_box = 0; // math: Scrollbar grip len.
            si32  m          = 0; // math: Master max pos.
            si32  s          = 0; // math: Scroll max pos.
            fp64  r          = 1; // math: Scroll/master len ratio.

            si32  cursor_pos = 0; // math: Mouse cursor position.

            // math: Calc scroll to master metrics.
            void s_to_m()
            {
                auto scroll_center = scroll_pos + scroll_box / 2.0;
                auto master_center = scroll_len ? scroll_center / r
                                                : 0;
                master_pos = (si32)std::round(master_center - master_box / 2.0);

                // Reset to extreme positions.
                if (scroll_pos == 0 && master_pos > 0) master_pos = 0;
                if (scroll_pos == s && master_pos < m) master_pos = m;
            }
            // math: Calc master to scroll metrics.
            void m_to_s()
            {
                if (master_box == 0) return;
                if (master_len == 0) master_len = master_box;
                r = (fp64)scroll_len / master_len;
                auto master_middle = master_pos + master_box / 2.0;
                auto scroll_middle = master_middle * r;
                scroll_box = std::max(1, (si32)(master_box * r));
                scroll_pos = (si32)std::round(scroll_middle - scroll_box / 2.0);

                // Don't place the grip behind the scrollbar.
                if (scroll_pos >= scroll_len) scroll_pos = scroll_len - 1;

                // Extreme positions are always closed last.
                s = scroll_len - scroll_box;
                m = master_len - master_box;

                if (scroll_len > 2) // Two-row hight is not suitable for this type of aligning.
                {
                    if (scroll_pos == 0 && master_pos > 0) scroll_pos = 1;
                    if (scroll_pos == s && master_pos < m) scroll_pos = s - 1;
                }
            }
            void update(rack const& scinfo)
            {
                master_inf = scinfo;
                m_to_s();
            }
            void resize(twod new_size)
            {
                scroll_len = new_size[Axis];
                m_to_s();
            }
            void stepby(si32 delta)
            {
                scroll_pos = std::clamp(scroll_pos + delta, 0, s);
                s_to_m();
            }
            void commit(rect& handle)
            {
                handle.coor[Axis]+= scroll_pos;
                handle.size[Axis] = scroll_box;
            }
            auto inside(si32 coor)
            {
                if (coor >= scroll_pos + scroll_box)
                {
                    return -1; // Below the grip.
                }
                if (coor >= scroll_pos)
                {
                    return 0; // Inside the grip.
                }
                else
                {
                    return 1; // Above the grip.
                }
            }
            auto follow()
            {
                auto dir = scroll_len > 2 ? inside(cursor_pos)
                                          : cursor_pos > 0 ?-1 // Don't stop to follow over
                                                           : 1;//    box on small scrollbar.
                return dir;
            }
            void setdir(si32 dir)
            {
                master_dir = dir;
            }
        };

        wptr boss; // grip: .
        bool wide; // grip: Is the scrollbar active.
        si32 thin; // grip: Scrollbar thickness.
        si32 init; // grip: Handle base width.
        si32 mult; // grip: Vertical bar width multiplier.
        hook memo; // grip: .
        math calc; // grip: Scrollbar calculator.
        bool on_pager = faux; // grip: .
        fp2d drag_origin; // grip: Drag origin.

        template<class Event>
        void send()
        {
            if (auto master = this->boss.lock())
            {
                master->bell::signal(tier::preview, Event::template _<Axis>, calc.master_inf);
            }
        }
        void config(si32 width)
        {
            thin = width;
            auto lims = Axis == axis::X ? twod{ -1, width }
                                        : twod{ width, -1 };
            base::limits(lims, lims);
        }
        void giveup(hids& gear)
        {
            if (on_pager)
            {
                gear.dismiss();
            }
            else
            {
                if (gear.captured(bell::id))
                {
                    if (this->form::protos(tier::release, hids::events::mouse::button::drag::cancel::right))
                    {
                        send<upon::scroll::cancel>();
                    }
                    base::deface();
                    gear.setfree();
                    gear.dismiss();
                }
            }
        }
        void pager(si32 dir)
        {
            calc.setdir(dir);
            send<upon::scroll::bypage>();
        }
        auto pager_repeat()
        {
            if (on_pager)
            {
                auto dir = calc.follow();
                pager(dir);
            }
            return on_pager;
        }

    protected:
        // gripfx: .
        void inform(rect new_area) override
        {
            calc.resize(new_area.size);
        }

    public:
        gripfx(sptr boss, si32 thickness = 1, si32 multiplier = 2)
            : boss{ boss       },
              wide{ faux       },
              thin{ thickness  },
              init{ thickness  },
              mult{ multiplier }
        {
            config(thin);

            boss->LISTEN(tier::release, upon::scroll::bycoor::any, scinfo, memo)
            {
                calc.update(scinfo);
                base::deface();
            };

            using bttn = hids::events::mouse::button;
            LISTEN(tier::release, hids::events::mouse::scroll::act, gear)
            {
                if (gear.meta(hids::anyCtrl)) return; // Ctrl+Wheel is reserved for zooming.
                if (gear.whlsi) pager(gear.whlsi > 0 ? 1 : -1);
                gear.dismiss();
            };
            LISTEN(tier::release, hids::events::mouse::move, gear)
            {
                calc.cursor_pos = twod{ gear.coord }[Axis];
            };
            LISTEN(tier::release, hids::events::mouse::button::dblclick::left, gear)
            {
                gear.dismiss(); // Do not pass double clicks outside.
            };
            LISTEN(tier::release, hids::events::mouse::button::down::any, gear)
            {
                if (!on_pager)
                if (this->form::protos(tier::release, bttn::down::left)
                 || this->form::protos(tier::release, bttn::down::right))
                if (auto dir = calc.inside(twod{ gear.coord }[Axis]))
                {
                    if (gear.capture(bell::id))
                    {
                        on_pager = true;
                        pager_repeat();
                        gear.dismiss();

                        timer.actify(activity::pager_first, skin::globals().repeat_delay, [&](auto)
                        {
                            if (pager_repeat())
                            {
                                timer.actify(activity::pager_next, skin::globals().repeat_rate, [&](auto)
                                {
                                    return pager_repeat(); // Repeat until on_pager.
                                });
                            }
                            return faux; // One shot call (first).
                        });
                    }
                }
            };
            LISTEN(tier::release, hids::events::mouse::button::up::any, gear)
            {
                if (on_pager && gear.captured(bell::id))
                {
                    if (this->form::protos(tier::release, bttn::up::left)
                     || this->form::protos(tier::release, bttn::up::right))
                    {
                        gear.setfree();
                        gear.dismiss();
                        on_pager = faux;
                        timer.pacify(activity::pager_first);
                        timer.pacify(activity::pager_next);
                    }
                }
            };
            LISTEN(tier::release, hids::events::mouse::button::up::right, gear)
            {
                //if (!gear.captured(bell::id)) //todo why?
                {
                    send<upon::scroll::cancel>();
                    gear.dismiss();
                }
            };

            LISTEN(tier::release, hids::events::mouse::button::drag::start::any, gear)
            {
                if (on_pager)
                {
                    gear.dismiss();
                }
                else
                {
                    if (gear.capture(bell::id))
                    {
                        drag_origin = gear.coord;
                        gear.dismiss();
                    }
                }
            };
            LISTEN(tier::release, hids::events::mouse::button::drag::pull::any, gear)
            {
                if (on_pager)
                {
                    gear.dismiss();
                }
                else
                {
                    if (gear.captured(bell::id))
                    {
                        if (auto delta = (twod{ gear.coord } - twod{ drag_origin })[Axis])
                        {
                            drag_origin = gear.coord;
                            calc.stepby(delta);
                            send<upon::scroll::bycoor>();
                            gear.dismiss();
                        }
                    }
                }
            };
            LISTEN(tier::release, hids::events::mouse::button::drag::cancel::any, gear)
            {
                giveup(gear);
            };
            LISTEN(tier::general, hids::events::halt, gear)
            {
                giveup(gear);
            };
            LISTEN(tier::release, hids::events::mouse::button::drag::stop::any, gear)
            {
                if (on_pager)
                {
                    gear.dismiss();
                }
                else
                {
                    if (gear.captured(bell::id))
                    {
                        if (this->form::protos(tier::release, bttn::drag::stop::right))
                        {
                            send<upon::scroll::cancel>();
                        }
                        base::deface();
                        gear.setfree();
                        gear.dismiss();
                    }
                }
            };
            LISTEN(tier::release, e2::form::state::mouse, active)
            {
                auto apply = [&](auto active)
                {
                    wide = active;
                    auto resize = Axis == axis::Y && mult;
                    if (resize) config(active ? init * mult // Make vertical scrollbar
                                              : init);      // wider on hover.
                    base::reflow();
                    return faux; // One-shot call.
                };

                timer.pacify(activity::mouse_leave);

                if (active) apply(activity::mouse_hover);
                else timer.actify(activity::mouse_leave, skin::globals().leave_timeout, apply);
            };
            //LISTEN(tier::release, hids::events::mouse::move, gear)
            //{
            //    auto apply = [&](auto active)
            //    {
            //        wide = active;
            //        if (Axis == axis::Y) config(active ? init * 2 // Make vertical scrollbar
            //                                           : init);   //  wider on hover
            //        base::reflow();
            //        return faux; // One shot call
            //    };
            //
            //    timer.pacify(activity::mouse_leave);
            //    apply(activity::mouse_hover);
            //    timer.template actify<activity::mouse_leave>(skin::globals().leave_timeout, apply);
            //};
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                auto region = parent_canvas.clip();
                auto object = parent_canvas.full();
                auto handle = region;

                calc.commit(handle);

                auto& handle_len = handle.size[Axis];
                auto& region_len = region.size[Axis];
                auto& object_len = object.size[Axis];

                handle.trimby(region);
                handle_len = std::max(1, handle_len);

                drawfx(*this, parent_canvas, handle, object_len, handle_len, region_len, wide);
            };
        }
    };

    namespace drawfx
    {
        static constexpr auto xlight = [](auto& boss, auto& canvas, auto handle, auto object_len, auto handle_len, auto region_len, auto wide)
        {
            if (object_len && handle_len != region_len) // Show only if it is oversized.
            {
                // Brightener isn't suitable for white backgrounds.
                //auto bright = skin::color(tone::brighter);
                //bright.bga(bright.bga() / 2).fga(0);
                //bright.link(bell::id);

                if (wide) // Draw full scrollbar on mouse hover
                {
                    canvas.fill([&](cell& c){ c.link(boss.bell::id).xlight(); });
                }
                //canvas.fill(handle, [&](cell& c){ c.fusefull(bright); });
                canvas.fill(handle, [&](cell& c){ c.link(boss.bell::id).xlight(); });
            }
        };
        static constexpr auto underline = [](auto& boss, auto& canvas, auto handle, auto object_len, auto handle_len, auto region_len, auto /*wide*/)
        {
            if (object_len && handle_len != region_len) // Show only if it is oversized.
            {
                auto fgc = boss.color().fgc();
                auto src = boss.base::This();
                while (!fgc && (src = src->parent())) // Detect scrollbar/underline color.
                {
                    fgc = src->color().fgc();
                }
                canvas.fill(handle, [&](cell& c){ c.unc(fgc).und(true); });
            }
        };
    }

    template<axis Axis>
    using grip = gripfx<Axis, drawfx::xlight>;

    // controls: Pluggable dummy object.
    class mock
        : public form<mock>
    { };

    // controls: Text label.
    class item
        : public form<item>
    {
        static constexpr auto dots = "…"sv;
        para data{}; // item: Label content.
        text utf8{}; // item: Text source.
        bool flex{}; // item: Violate or not the label size.
        bool test{}; // item: Place or not(default) the Two Dot Leader when there is not enough space.
        bool ulin{}; // item: Draw full-width underline.

        // item: .
        void _set(view new_utf8)
        {
            data.parser::style.wrp(wrap::off);
            data = new_utf8;
            utf8 = new_utf8;
        }

    protected:
        // item: .
        void deform(rect& new_area) override
        {
            new_area.size.x = flex ? new_area.size.x : data.size().x;
            new_area.size.y = std::max(data.size().y, new_area.size.y);
        }

    public:
        // item: .
        template<bool Reflow = true>
        auto set(view new_utf8)
        {
            _set(new_utf8);
            if constexpr (Reflow) base::reflow();
            return This();
        }
        // item: .
        auto& get_source()
        {
            return utf8;
        }

        item(view label = {})
        {
            _set(label);
            LISTEN(tier::release, e2::data::utf8, utf8)
            {
                set(utf8);
            };
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                auto full = parent_canvas.full();
                auto context = parent_canvas.bump(-base::intpad, faux);
                parent_canvas.cup(dot_00);
                parent_canvas.output(data);
                if (test)
                {
                    auto area = parent_canvas.clip();
                    auto size = data.size();
                    if (area.size > 0 && size.x > 0)
                    {
                        if (full.coor.x < area.coor.x)
                        {
                            auto coor = area.coor - parent_canvas.coor();
                            coor.y += std::min(area.size.y - 1, base::intpad.t);
                            parent_canvas.core::begin(coor)->txt(dots);
                        }
                        if (full.coor.x + base::intpad.l + size.x + base::intpad.r > area.coor.x + area.size.x)
                        {
                            auto coor = area.coor - parent_canvas.coor();
                            coor.x += area.size.x - 1;
                            coor.y += std::min(area.size.y - 1, base::intpad.t);
                            parent_canvas.core::begin(coor)->txt(dots);
                        }
                    }
                }
                if (ulin)
                {
                    auto area = parent_canvas.full();
                    parent_canvas.fill(area, [](cell& c)
                    {
                        auto u = c.und();
                        if (u == unln::line) c.und(unln::biline);
                        else                 c.und(unln::line);
                    });
                }
                parent_canvas.bump(context);
            };
        }
        // item: .
        auto flexible(bool b = true) { flex = b; return This(); }
        // item: .
        auto drawdots(bool b = true) { test = b; return This(); }
        // item: .
        auto accented(bool b = true) { ulin = b; return This(); }
        // item: .
        void brush(cell c)
        {
            data.parser::brush.reset(c);
        }
    };

    // controls: Textedit box.
    class edit
        : public form<edit>
    {
        page data;

    public:
        edit()
        {
        }
    };

    // DEPRECATED STUFF

    class stem_rate_grip
        : public form<stem_rate_grip>
    {
        //todo cache specific
        netxs::sptr<face> coreface;
        face& canvas;

    public:
        page topic; // stem_rate_grip: Text content.

        bool enabled;
        twod box_len;
        text pin_str;
        text sfx_str;
        si32 sfx_len;
        si32 cur_val;

        enum
        {
            txt_id,
            pin_id,
        };

        void set_pen(byte hilight)
        {
            canvas.mark().bga(hilight);
        }
        void recalc()
        {
            auto cur_str = std::to_string(cur_val);
            auto cur_len = utf::length(cur_str);
            auto pin_pos = std::max(cur_len, sfx_len) + 1;
            box_len.x = 1 + 2 * pin_pos;
            box_len.y = 4;

            topic[txt_id] = cur_str + " " + sfx_str;
            topic[pin_id] = pin_str;
            topic[txt_id].locus.kill().chx(pin_pos - cur_len).cud(1);
            topic[pin_id].locus.kill().chx(pin_pos);
            topic.reindex();

            base::resize(box_len);
            deface();
        }
        auto set_val(si32 new_val, view pin_chr)
        {
            cur_val = new_val;
            pin_str = pin_chr;
            recalc();
            return box_len;
        }

        stem_rate_grip(view sfx_string)
            : coreface{ ptr::shared<face>() },
                canvas{ *coreface           },
               sfx_str{ sfx_string          }
        {
            //todo cache specific
            canvas.link(bell::id);
            LISTEN(tier::release, e2::area, new_area)
            {
                if (canvas.size() != new_area.size)
                {
                    canvas.size(new_area.size);
                }
            };
            LISTEN(tier::request, e2::form::canvas, canvas) { canvas = coreface; };

            sfx_len = utf::length(sfx_str);

            canvas.mark().bgc(whitelt);
            topic = ansi::idx(txt_id).nop().eol()
                         .idx(pin_id).nop();

            set_pen(0);

            LISTEN(tier::release, e2::form::state::mouse, active)
            {
                set_pen(active ? 80 : 0);
                recalc();
            };
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (base::ruined())
                {
                    canvas.wipe();
                    canvas.output(topic);
                    base::ruined(faux);
                }
                parent_canvas.fill(canvas, cell::shaders::fusefull);
            };
        }

    protected:
        // stem_rate_grip: .
        void deform(rect& new_area) override
        {
            new_area.size = box_len; // Suppress resize.
        }
    };

    template<auto Tier, class Event>
    class stem_rate
        : public form<stem_rate<Tier, Event>>
    {
        pro::robot robot{*this }; // stem_rate: Animation controller.

        //todo cache specific
        netxs::sptr<face> coreface;
        face& canvas;

        using tail = netxs::datetime::tail<si32>;

    public:
        page topic; // stem_rate: Text content.

        netxs::sptr<stem_rate_grip> grip_ctl;

        enum
        {
            //   cur_id + "sfx_str"
            //  ────█─────────bar_id──
            // min_id            max_id

            bar_id,
            min_id,
            max_id,
        };

        twod pin_len;
        si32 pin_pos = 0;
        si32 bar_len = 0;
        si32 cur_val = 0;
        si32 min_val = 0;
        si32 max_val = 0;
        si32 origin = 0;
        si32 deltas = 0;
        fp2d drag_origin;

        //todo unify mint = 1/fps60 = 16ms
        //it seems that 4ms is enough, there is no need to be tied to fps (an open question)
        tail bygone{ 75ms, 4ms };

        text grip_suffix;
        text label_text;
        si32 pad = 5;
        si32 bgclr = 4;

        void recalc()
        {
            bar_len = std::max(0, base::size().x - (pad + 1) * 2);
            auto pin_abs = netxs::divround((bar_len + 1) * (cur_val - min_val),
                (max_val - min_val));
            auto pin_str = text{};
                 if (pin_abs == 0)           pin_str = "├";
            else if (pin_abs == bar_len + 1) pin_str = "┤";
            else                             pin_str = "┼";

            pin_len = grip_ctl->set_val(cur_val, pin_str);
            pin_pos = pad + pin_abs - pin_len.x / 2;
            topic[bar_id] = "└" + utf::repeat("─", bar_len) + "┘";
            topic[bar_id].locus.kill().chx(pad);
            topic.reindex();
        }
        si32 next_val(si32 delta)
        {
            auto dm = max_val - min_val;
            auto p = divround((bar_len + 1) * (origin - min_val), dm);
            auto c = divround((p + delta) * dm, (bar_len + 1)) + min_val;
            bygone.set(c - cur_val);
            return c;
        }
        bool _move_grip(si32 new_val)
        {
            new_val = std::clamp(new_val, min_val, max_val);
            if (new_val != cur_val)
            {
                cur_val = new_val;
                recalc();
                base::deface();

                return true;
            }
            return faux;
        }
        void move_grip(si32 new_val)
        {
            if (_move_grip(new_val))
            {
                base::riseup(Tier, Event{}, cur_val);
            }
        }
        void giveup(hids& gear)
        {
            if (gear.captured(grip_ctl->id))
            {
                deltas = 0;
                move_grip(origin);
                gear.setfree();
                gear.dismiss();
            }
        }

        stem_rate(text const& caption, si32 min_value, si32 max_value, view suffix)
            : coreface{ ptr::shared<face>() },
              canvas{ *coreface },
              min_val{ min_value },
              max_val{ max_value },
              grip_suffix{ suffix }
        {
            //todo cache specific
            canvas.link(bell::id);
            LISTEN(tier::release, e2::area, new_area)
            {
                if (canvas.size() != new_area.size)
                {
                    canvas.size(new_area.size);
                }
                recalc();
            };
            LISTEN(tier::request, e2::form::canvas, canvas) { canvas = coreface; };

            cur_val = -1;
            base::riseup(Tier, Event{}, cur_val);

            base::limits(twod{ utf::length(caption) + (pad + 2) * 2, 10 });

            topic = ansi::wrp(wrap::off).jet(bias::left)
                .cpy(50).chx(pad + 2).cuu(3).add(caption).cud(3)
                .idx(bar_id).nop().eol()
                .idx(min_id).nop().idx(max_id).nop();

            topic[min_id] = std::to_string(min_val);
            topic[max_id] = std::to_string(max_val);
            topic[max_id].style.jet(bias::right);
            topic[max_id].locus.chx(pad);
            topic[min_id].locus.chx(pad);

            LISTEN(tier::general, Event{}, cur_val)
            {
                if (cur_val >= min_val)
                {
                    _move_grip(cur_val);
                }
            };
            LISTEN(tier::release, e2::form::upon::vtree::attached, parent)
            {
                grip_ctl = stem_rate_grip::ctor(grip_suffix);
                grip_ctl->bell::signal(tier::release, e2::form::upon::vtree::attached, base::This());

                grip_ctl->LISTEN(tier::release, hids::events::mouse::button::drag::start::left, gear)
                {
                    if (gear.capture(grip_ctl->id))
                    {
                        drag_origin = gear.coord;
                        origin = cur_val;
                        gear.dismiss();
                    }
                };
                grip_ctl->LISTEN(tier::release, hids::events::mouse::button::drag::pull::left, gear)
                {
                    if (gear.captured(grip_ctl->id))
                    {
                        if (auto delta = (twod{ gear.coord } - twod{ drag_origin }).x)
                        {
                            drag_origin = gear.coord;
                            deltas += delta;
                            move_grip(next_val(deltas));
                            gear.dismiss();
                        }
                    }
                };
                grip_ctl->LISTEN(tier::release, hids::events::mouse::button::drag::cancel::left, gear)
                {
                    giveup(gear);
                };
                grip_ctl->LISTEN(tier::general, hids::events::halt, gear)
                {
                    giveup(gear);
                };
                grip_ctl->LISTEN(tier::release, hids::events::mouse::button::drag::stop::left, gear)
                {
                    if (gear.captured(grip_ctl->id))
                    {
                        deltas = 0;
                        gear.setfree();
                        base::deface();
                        robot.actify(bygone.fader<quadratic<si32>>(750ms), [&](auto& delta)
                        {
                            move_grip(cur_val + delta);
                        });
                        gear.dismiss();
                    }
                };
                grip_ctl->LISTEN(tier::release, hids::events::mouse::scroll::act, gear)
                {
                    if (gear.meta(hids::anyCtrl)) return; // Ctrl+Wheel is reserved for zooming.
                    if (gear.whlsi) move_grip(cur_val - gear.whlsi);
                    gear.dismiss();
                };
                recalc();
            };
            LISTEN(tier::release, hids::events::mouse::button::click::right, gear)
            {
                base::color(canvas.mark().fgc(), (tint)((++bgclr) % 16));
                base::deface();
                gear.dismiss();
            };
            LISTEN(tier::release, hids::events::mouse::scroll::act, gear)
            {
                if (gear.meta(hids::anyCtrl)) return; // Ctrl+Wheel is reserved for zooming.
                if (gear.whlsi) move_grip(cur_val - 10 * gear.whlsi);
                gear.dismiss();
            };
            LISTEN(tier::release, e2::render::any, parent_canvas)
            {
                if (base::ruined())
                {
                    base::ruined(faux); // The object may be invalidated again during rendering. (see pro::d_n_d)
                    canvas.wipe(base::color());
                    canvas.output(topic);
                    auto cp = canvas.cp();
                    cp.x = pin_pos;
                    cp.y -= 3;
                    grip_ctl->base::moveto(cp);
                    grip_ctl->render(canvas);
                }
                parent_canvas.fill(canvas, cell::shaders::fusefull);
            };
        }
    };
}