// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

#include "richtext.hpp"
#include "events.hpp"
#include "xml.hpp"

#include <typeindex>
#include <future>

namespace netxs::input
{
    struct hids;
    struct foci;
    using sysmouse = directvt::binary::sysmouse_t;
    using syskeybd = directvt::binary::syskeybd_t;
    using sysfocus = directvt::binary::sysfocus_t;
    using syswinsz = directvt::binary::syswinsz_t;
    using sysclose = directvt::binary::sysclose_t;
    using sysboard = directvt::binary::sysboard_t;
    using clipdata = directvt::binary::clipdata_t;
    using auth = netxs::events::auth;
}
namespace netxs::ui
{
    struct base;
    struct input_fields_t;

    using namespace netxs::input;
    using sptr = netxs::sptr<base>;
    using wptr = netxs::wptr<base>;
    using focus_test_t = std::pair<id_t, si32>;
    using gear_id_list_t = std::list<id_t>;
    using functor = std::function<void(sptr)>;
    using proc = std::function<void(hids&)>;
    using s11n = directvt::binary::s11n;
    using escx = ansi::escx;
    using book = std::vector<sptr>;
}

namespace netxs::events::userland
{
    struct e2
    {
        static constexpr auto dtor = netxs::events::userland::root::dtor;
        static constexpr auto cascade = netxs::events::userland::root::cascade;
        static constexpr auto cleanup = netxs::events::userland::root::cleanup;

        EVENTPACK( e2, netxs::events::userland::root::base )
        {
            EVENT_XS( postrender, ui::face       ), // release: UI-tree post-rendering. Draw debug overlay, maker, titles, etc.
            EVENT_XS( nextframe , bool           ), // general: Signal for rendering the world, the parameter indicates whether the world has been modified since the last rendering.
            EVENT_XS( shutdown  , const text     ), // general: Server shutdown.
            EVENT_XS( area      , rect           ), // release: Object rectangle.
            GROUP_XS( extra     , si32           ), // Event extension slot.
            GROUP_XS( timer     , time           ), // Timer tick, arg: current moment (now).
            GROUP_XS( render    , ui::face       ), // release: UI-tree rendering.
            GROUP_XS( conio     , si32           ),
            GROUP_XS( form      , bool           ),
            GROUP_XS( data      , si32           ),
            GROUP_XS( config    , si32           ), // Set/notify/get/global_set configuration data.
            GROUP_XS( command   , si32           ), // Exec UI command.

            SUBSET_XS( extra )
            {
                EVENT_XS( slot1, si32 ),
                EVENT_XS( slot2, si32 ),
                EVENT_XS( slot3, si32 ),
                EVENT_XS( slot4, si32 ),
                EVENT_XS( slot5, si32 ),
                EVENT_XS( slot6, si32 ),
                EVENT_XS( slot7, si32 ),
                EVENT_XS( slot8, si32 ),
                EVENT_XS( slot9, si32 ),
                EVENT_XS( slotA, si32 ),
                EVENT_XS( slotB, si32 ),
                EVENT_XS( slotC, si32 ),
                EVENT_XS( slotD, si32 ),
                EVENT_XS( slotE, si32 ),
                EVENT_XS( slotF, si32 ),
            };
            SUBSET_XS( timer )
            {
                EVENT_XS( tick, time ), // relaese: Execute before e2::timer::any (rendering).
            };
            SUBSET_XS( render ) // release any: UI-tree default rendering submission.
            {
                GROUP_XS( background, ui::face ), // release: UI-tree background rendering. Used by form::shader.

                SUBSET_XS( background )
                {
                    EVENT_XS( prerender, ui::face ), // release: UI-tree pre-rendering, used by pro::cache (can interrupt bell::signal) and any kind of highlighters.
                };
            };
            SUBSET_XS( config )
            {
                EVENT_XS( creator, ui::sptr ), // request: Pointer to world object.
                EVENT_XS( fps    , si32     ), // Request to set new fps, arg: new fps (si32); the value == -1 is used to request current fps.
                GROUP_XS( cursor , span     ), // Any kind of intervals property.
                GROUP_XS( plugins, si32     ),

                SUBSET_XS( cursor )
                {
                    EVENT_XS( blink, span ), // Cursor blinking interval.
                    EVENT_XS( style, si32 ), // netxs::text_cursor.
                };
                SUBSET_XS( plugins )
                {
                    EVENT_XS( align, bool     ), // release: Enable/disable align plugin.
                    GROUP_XS( focus, ui::sptr ), // request: pro::focus owner.
                    GROUP_XS( sizer, dent     ), // Configure sizer.

                    SUBSET_XS( focus )
                    {
                        EVENT_XS( owner, ui::sptr ), // request: pro::focus owner.
                    };
                    SUBSET_XS( sizer )
                    {
                        EVENT_XS( inner, dent ), // release: Set inner size; request: request unner size.
                        EVENT_XS( outer, dent ), // release: Set outer size; request: request outer size.
                        EVENT_XS( inert, bool ), // release: Set read only mode (no active actions, follow only).
                        EVENT_XS( alive, bool ), // release: Shutdown pro::sizer.
                    };
                };
            };
            SUBSET_XS( conio )
            {
                EVENT_XS( mouse   , input::sysmouse ), // release: Mouse activity.
                EVENT_XS( keybd   , input::syskeybd ), // release: Keybd activity.
                EVENT_XS( focus   , input::sysfocus ), // release: Focus activity.
                EVENT_XS( board   , input::sysboard ), // release: Clipboard preview.
                EVENT_XS( error   , const si32      ), // release: Return error code.
                EVENT_XS( winsz   , const twod      ), // release: Order to update terminal primary overlay.
                EVENT_XS( preclose, const bool      ), // release: Signal to quit after idle timeout, arg: bool - ready to shutdown.
                EVENT_XS( quit    , const si32      ), // release: Quit, arg: si32 - quit reason.
                EVENT_XS( pointer , const bool      ), // release: Mouse pointer visibility.
                EVENT_XS( logs    , const text      ), // Log output.
            };
            SUBSET_XS( data )
            {
                //todo revise (see app::desk)
                EVENT_XS( changed, text       ), // release/preview/request: Current menu item id(text).
                EVENT_XS( request, si32       ),
                EVENT_XS( disable, si32       ),
                EVENT_XS( flush  , si32       ),
                EVENT_XS( utf8   , const text ), // Signaling with a text string, release only.
            };
            SUBSET_XS( command )
            {
                EVENT_XS( cout       , const text  ), // Append extra data to output.
                EVENT_XS( custom     , si32        ), // Custom command, arg: cmd_id.
                EVENT_XS( printscreen, input::hids ), // Copy screen area to clipboard.
                GROUP_XS( request    , input::hids ), // general: Request input field list.

                SUBSET_XS( request )
                {
                    EVENT_XS( inputfields, ui::input_fields_t ), // general: Request input field list.
                };
            };
            SUBSET_XS( form )
            {
                EVENT_XS( canvas   , sptr<core>  ), // Request global canvas.
                GROUP_XS( size     , input::hids ), // Window size manipulation.
                GROUP_XS( layout   , const twod  ),
                GROUP_XS( draggable, bool        ), // Signal to the form to enable draggablity for specified mouse button.
                GROUP_XS( upon     , bool        ),
                GROUP_XS( proceed  , bool        ),
                GROUP_XS( cursor   , bool        ),
                GROUP_XS( drag     , input::hids ),
                GROUP_XS( prop     , text        ),
                GROUP_XS( global   , twod        ),
                GROUP_XS( state    , const twod  ),
                GROUP_XS( animate  , id_t        ),

                SUBSET_XS( size )
                {
                    EVENT_XS( restore    , ui::sptr    ),
                    EVENT_XS( minimize   , input::hids ),
                    GROUP_XS( enlarge    , input::hids ),

                    SUBSET_XS( enlarge )
                    {
                        EVENT_XS( fullscreen , input::hids ),
                        EVENT_XS( maximize   , input::hids ),
                    };
                };
                SUBSET_XS( draggable )
                {
                    EVENT_XS( left     , bool ),
                    EVENT_XS( right    , bool ),
                    EVENT_XS( middle   , bool ),
                    EVENT_XS( xbutton1 , bool ),
                    EVENT_XS( xbutton2 , bool ),
                    EVENT_XS( leftright, bool ),

                    INDEX_XS( left, right, middle, xbutton1, xbutton2, leftright ),
                };
                SUBSET_XS( layout )
                {
                    EVENT_XS( unselect, input::hids ), // Inform if unselected.
                    EVENT_XS( selected, input::hids ), // Inform if selected.
                    EVENT_XS( shift   , const twod  ), // Request a global shifting with delta.
                    EVENT_XS( jumpto  , ui::base    ), // Fly to the specified object.
                    EVENT_XS( convey  , cube        ), // Request a global conveying with delta (Inform all children to be conveyed).
                    EVENT_XS( bubble  , rect        ), // Order to popup the requested item through the visual tree.
                    EVENT_XS( expose  , rect        ), // Order to bring the requested item on top of the visual tree.
                    EVENT_XS( appear  , twod        ), // Fly to the specified coords.
                    EVENT_XS( swarp   , const dent  ), // preview: Do form swarping.
                    GROUP_XS( go      , ui::sptr    ), // preview: Do form swarping.
                    GROUP_XS( focus   , id_t        ),

                    SUBSET_XS( go )
                    {
                        EVENT_XS( next , ui::sptr ), // request: Proceed request for available objects (next).
                        EVENT_XS( prev , ui::sptr ), // request: Proceed request for available objects (prev).
                        EVENT_XS( item , ui::sptr ), // request: Proceed request for available objects (current).
                    };
                    SUBSET_XS( focus )
                    {
                        EVENT_XS( next , id_t ), // request: Ask to switch focus to the next window.
                        EVENT_XS( prev , id_t ), // request: Ask to switch focus to the prev window.
                    };
                };
                SUBSET_XS( upon )
                {
                    EVENT_XS( created, input::hids ), // release: Notify the instance of who created it.
                    EVENT_XS( started, ui::sptr    ), // release: Notify the instance is commissioned. arg: visual root.
                    EVENT_XS( resized, const rect  ), // anycast: Notify about the actual window area.
                    EVENT_XS( changed, twod        ), // Event after resize, arg: diff bw old and new size.
                    EVENT_XS( dragged, input::hids ), // Event after drag.
                    EVENT_XS( stopped, bool        ), // release: Notify that the main reading loop has exited. arg bool: fast or not.
                    GROUP_XS( vtree  , ui::sptr    ), // Visual tree events, arg: parent base_sptr.
                    GROUP_XS( scroll , rack        ), // Event after scroll.

                    SUBSET_XS( vtree )
                    {
                        EVENT_XS( attached, ui::sptr ), // Child has been attached, arg: parent ui::sptr.
                        EVENT_XS( detached, ui::sptr ), // Child has been detached, arg: parent ui::sptr.
                    };
                    SUBSET_XS( scroll )
                    {
                        GROUP_XS( to_top, rack ), // Scroll to top.
                        GROUP_XS( to_end, rack ), // Scroll to end.
                        GROUP_XS( bycoor, rack ), // Scroll absolute.
                        GROUP_XS( bystep, rack ), // Scroll by delta.
                        GROUP_XS( bypage, rack ), // Scroll by page.
                        GROUP_XS( cancel, rack ), // Reset scrolling.

                        SUBSET_XS( to_top )
                        {
                            EVENT_XS( x, rack ), // Scroll to_top along X.
                            EVENT_XS( y, rack ), // Scroll to_top along Y.
                            EVENT_XS( v, rack ), // Scroll to_top along XY.

                            INDEX_XS( x, y, v ),
                        };
                        SUBSET_XS( to_end )
                        {
                            EVENT_XS( x, rack ), // Scroll to_end along X.
                            EVENT_XS( y, rack ), // Scroll to_end along Y.
                            EVENT_XS( v, rack ), // Scroll to_end along XY.

                            INDEX_XS( x, y, v ),
                        };
                        SUBSET_XS( bycoor )
                        {
                            EVENT_XS( x, rack ), // Scroll absolute along X.
                            EVENT_XS( y, rack ), // Scroll absolute along Y.
                            EVENT_XS( v, rack ), // Scroll absolute along XY.

                            INDEX_XS( x, y, v ),
                        };
                        SUBSET_XS( bystep )
                        {
                            EVENT_XS( x, rack ), // Scroll by delta along X.
                            EVENT_XS( y, rack ), // Scroll by delta along Y.
                            EVENT_XS( v, rack ), // Scroll by delta along XY.

                            INDEX_XS( x, y, v ),
                        };
                        SUBSET_XS( bypage )
                        {
                            EVENT_XS( x, rack ), // Scroll by page along X.
                            EVENT_XS( y, rack ), // Scroll by page along Y.
                            EVENT_XS( v, rack ), // Scroll by page along XY.

                            INDEX_XS( x, y, v ),
                        };
                        SUBSET_XS( cancel )
                        {
                            EVENT_XS( x, rack ), // Cancel scrolling along X.
                            EVENT_XS( y, rack ), // Cancel scrolling along Y.
                            EVENT_XS( v, rack ), // Cancel scrolling along XY.

                            INDEX_XS( x, y, v ),
                        };
                    };
                };
                SUBSET_XS( proceed )
                {
                    EVENT_XS( create    , rect        ), // Return coordinates of the new object placeholder.
                    EVENT_XS( createby  , input::hids ), // Return gear with coordinates of the new object placeholder gear::slot.
                    EVENT_XS( render    , bool        ), // Ask children to render itself to the parent canvas, arg is the world is damaged or not.
                    EVENT_XS( attach    , ui::sptr    ), // Order to attach a child, arg is a parent base_sptr.
                    EVENT_XS( swap      , ui::sptr    ), // Order to replace existing object. See tiling manager empty slot.
                    EVENT_XS( functor   , ui::functor ), // Exec functor (see pro::focus).
                    EVENT_XS( onbehalf  , ui::proc    ), // Exec functor on behalf (see gate).
                    GROUP_XS( quit      , bool        ), // Request to quit/detach (arg: fast or not).

                    SUBSET_XS( quit )
                    {
                        EVENT_XS( one, bool ), // Signal to close (fast or not).
                    };
                };
                SUBSET_XS( cursor )
                {
                    EVENT_XS(blink, bool),
                };
                SUBSET_XS( animate )
                {
                    EVENT_XS( start, id_t ),
                    EVENT_XS( stop , id_t ),
                    EVENT_XS( reset, id_t ),
                };
                SUBSET_XS( drag )
                {
                    GROUP_XS( start , input::hids ), // Notify about mouse drag start by pro::mouse.
                    GROUP_XS( pull  , input::hids ), // Notify about mouse drag pull by pro::mouse.
                    GROUP_XS( cancel, input::hids ), // Notify about mouse drag cancel by pro::mouse.
                    GROUP_XS( stop  , input::hids ), // Notify about mouse drag stop by pro::mouse.

                    SUBSET_XS( start )
                    {
                        EVENT_XS( left     , input::hids ),
                        EVENT_XS( right    , input::hids ),
                        EVENT_XS( middle   , input::hids ),
                        EVENT_XS( xbutton1 , input::hids ),
                        EVENT_XS( xbutton2 , input::hids ),
                        EVENT_XS( leftright, input::hids ),

                        INDEX_XS( left, right, middle, xbutton1, xbutton2, leftright ),
                    };
                    SUBSET_XS( pull )
                    {
                        EVENT_XS( left     , input::hids ),
                        EVENT_XS( right    , input::hids ),
                        EVENT_XS( middle   , input::hids ),
                        EVENT_XS( xbutton1 , input::hids ),
                        EVENT_XS( xbutton2 , input::hids ),
                        EVENT_XS( leftright, input::hids ),

                        INDEX_XS( left, right, middle, xbutton1, xbutton2, leftright ),
                    };
                    SUBSET_XS( cancel )
                    {
                        EVENT_XS( left     , input::hids ),
                        EVENT_XS( right    , input::hids ),
                        EVENT_XS( middle   , input::hids ),
                        EVENT_XS( xbutton1 , input::hids ),
                        EVENT_XS( xbutton2 , input::hids ),
                        EVENT_XS( leftright, input::hids ),

                        INDEX_XS( left, right, middle, xbutton1, xbutton2, leftright ),
                    };
                    SUBSET_XS( stop )
                    {
                        EVENT_XS( left     , input::hids ),
                        EVENT_XS( right    , input::hids ),
                        EVENT_XS( middle   , input::hids ),
                        EVENT_XS( xbutton1 , input::hids ),
                        EVENT_XS( xbutton2 , input::hids ),
                        EVENT_XS( leftright, input::hids ),

                        INDEX_XS( left, right, middle, xbutton1, xbutton2, leftright ),
                    };
                };
                SUBSET_XS( prop )
                {
                    EVENT_XS( name      , text       ), // user name.
                    EVENT_XS( zorder    , zpos       ), // Set form z-order, si32: -1 backmost, 0 plain, 1 topmost.
                    EVENT_XS( filler    , const cell ), // Set form brush/color.
                    EVENT_XS( fullscreen, ui::sptr   ), // Set fullscreen app.
                    EVENT_XS( viewport  , rect       ), // request: Return form actual viewport.
                    EVENT_XS( lucidity  , si32       ), // set or request window transparency, si32: 0-255, -1 to request.
                    EVENT_XS( cwd       , text       ), // Riseup preview->anycast current working directory.
                    GROUP_XS( window    , twod       ), // Set or request window properties.
                    GROUP_XS( ui        , text       ), // Set or request textual properties.
                    GROUP_XS( colors    , argb       ), // Set or request bg/fg colors.

                    SUBSET_XS( window )
                    {
                        EVENT_XS( size    , twod     ), // Set window size.
                        EVENT_XS( fullsize, rect     ), // Request window size with titles and borders.
                        EVENT_XS( instance, ui::sptr ), // Request window instance.
                    };
                    SUBSET_XS( ui )
                    {
                        EVENT_XS( title   , text ), // Form title + foci status.
                        EVENT_XS( header  , text ), // Set/get form caption header.
                        EVENT_XS( footer  , text ), // Set/get form caption footer.
                        EVENT_XS( tooltip , text ), // Set/get tooltip text.
                        EVENT_XS( slimmenu, bool ), // Set/get window menu size.
                        EVENT_XS( acryl   , bool ), // Set/get window acrylic effect.
                        EVENT_XS( cache   , bool ), // Set/get render cache usage.
                    };
                    SUBSET_XS( colors )
                    {
                        EVENT_XS( bg, argb ), // Set/get argb color.
                        EVENT_XS( fg, argb ), // Set/get argb color.
                    };
                };
                SUBSET_XS( global )
                {
                    EVENT_XS( sysstart, si32 ), // release: Notify dtvt-application started: 1 - started, 0 - exited.
                };
                SUBSET_XS( state )
                {
                    EVENT_XS( mouse    , si32       ), // Notify if mouse is active or not. The form is active when the number of clients (form::eventa::mouse::enter - mouse::leave) is not zero, only release.
                    EVENT_XS( hover    , si32       ), // Notify how many mouse cursors are hovering, si32 - number of cursors.
                    EVENT_XS( color    , ui::tone   ), // Notify has changed tone, preview to set.
                    EVENT_XS( highlight, bool       ),
                    EVENT_XS( visible  , bool       ),
                    EVENT_XS( maximized, id_t       ),
                    EVENT_XS( disabled , bool       ),
                    GROUP_XS( focus    , const id_t ),
                    GROUP_XS( keybd    , bool       ),

                    SUBSET_XS( focus )
                    {
                        EVENT_XS( on   , const id_t ),
                        EVENT_XS( off  , const id_t ),
                        EVENT_XS( count, si32       ), // Notify the object has changed keyboard foci count.
                    };
                    SUBSET_XS( keybd )
                    {
                        EVENT_XS( enlist   , ui::gear_id_list_t ), // anycast: Enumerate all available foci.
                        EVENT_XS( find     , ui::focus_test_t   ), // request: Check the focus.
                        EVENT_XS( next     , ui::focus_test_t   ), // request: Next hop count.
                        EVENT_XS( check    , bool               ), // anycast: Check any focus.
                        GROUP_XS( command  , si32               ), // release: Hotkey command preview.

                        SUBSET_XS( command )
                        {
                            EVENT_XS( close, si32 ), // release: Hotkey close command preview.
                        };
                    };
                };
            };
        };
    };
}

namespace netxs::ui
{
    using e2 = netxs::events::userland::e2;

    //todo reimplement
    struct skin
    {
        poly winfocus;
        poly brighter;
        poly shadower;

        cell warning;
        cell danger;
        cell action;
        cell active;
        cell inactive;
        cell selected;
        cell focused;
        cell window_clr;

        bool tracking = faux;
        bool menuwide = faux;
        bool macstyle = faux;

        si32 spd;
        si32 pls;
        si32 ccl;
        si32 spd_accel;
        si32 ccl_accel;
        si32 spd_max;
        si32 ccl_max;
        span switching;
        span deceleration;
        span blink_period;
        span menu_timeout;
        span leave_timeout;
        span repeat_delay;
        span repeat_rate;

        bool shadow_enabled = true;
        si32 shadow_blur = 3;
        fp32 shadow_bias = 0.37f;
        fp32 shadow_opacity = 105.5f;
        twod shadow_offset = dot_21;

        twod min_value = dot_00;
        twod max_value = twod{ 3000, 2000 }; //todo unify

        static auto& globals()
        {
            static skin _globals;
            return _globals;
        }
        // skin:: Return global brighter/shadower color (cell).
        static cell const& color(si32 property)
        {
            auto& g = globals();
            switch (property)
            {
                case tone::prop::winfocus:   return g.winfocus;
                case tone::prop::window_clr: return g.window_clr;
                case tone::prop::brighter:   return g.brighter;
                case tone::prop::shadower:   return g.shadower;
                case tone::prop::selected:   return g.selected;
                case tone::prop::active:     return g.active;
                case tone::prop::focused:    return g.focused;
                case tone::prop::warning:    return g.warning;
                case tone::prop::danger:     return g.danger;
                case tone::prop::action:     return g.action;
                case tone::prop::inactive:   return g.inactive;
                default:                     return g.brighter;
            }
        }
        // skin:: Return global gradient for brighter/shadower.
        static poly const& grade(si32 property)
        {
            auto& g = globals();
            switch (property)
            {
                case tone::prop::winfocus: return g.winfocus;
                case tone::prop::brighter: return g.brighter;
                case tone::prop::shadower: return g.shadower;
                default:                   return g.brighter;
            }
        }
    };

    // console: Base visual.
    struct base
        : public bell, public std::enable_shared_from_this<base>
    {
        enum type
        {
            reflow_root = -1,
            client = 0,
            node = 1,
            placeholder = 2,
        };

        book subset; // base: List of nested objects.
        wptr father; // base: Reference to parent.
        subs relyon; // base: Subscription on parent events.
        rect region; // base: The region occupied by the object.
        rect socket; // base: The region provided for the object.
        cell filler; // base: Object color.
        twod min_sz; // base: Minimal size.
        twod max_sz; // base: Maximal size.
        twod anchor; // base: Object balance point. Center point for any transform (on preview).
        dent oversz; // base: Oversize, for scrolling.
        dent extpad; // base: Pads around object.
        dent intpad; // base: Pads inside object.
        bind atgrow; // base: Bindings on enlarging.
        bind atcrop; // base: Bindings on shrinking.
        bool wasted; // base: Should the object be redrawn.
        bool hidden; // base: Ignore rendering and resizing.
        bool locked; // base: Object has fixed size.
        bool master; // base: Anycast root.
        si32 family; // base: Object type.

        template<class T = base>
        auto   This()       { return std::static_pointer_cast<std::remove_reference_t<T>>(shared_from_this()); }
        auto&  coor() const { return region.coor;          }
        auto&  size() const { return region.size;          }
        auto&  area() const { return region;               }
        void   root(bool b) { master = b;                  }
        bool   root()       { return master;               }
        si32   kind()       { return family;               }
        void   kind(si32 k) { family = k;                  }
        auto center() const { return region.center();      }
        auto parent()       { return father.lock();        }
        void ruined(bool s) { wasted = s;                  }
        auto ruined() const { return wasted;               }
        template<bool Absolute = true>
        auto actual_area() const
        {
            auto area = rect{ -oversz.corner(), region.size + oversz };
            if constexpr (Absolute) area.coor += region.coor;
            return area;
        }
        auto color() const { return base::filler; }
        void color(argb fg_color, argb bg_color)
        {
            base::filler.bgc(bg_color)
                        .fgc(fg_color)
                        .txt(whitespace);
            bell::signal(tier::release, e2::form::prop::filler, filler);
        }
        void color(cell const& new_filler) // Set id=0 to make the object transparent to mouse events.
        {
            base::filler = new_filler;
            bell::signal(tier::release, e2::form::prop::filler, filler);
        }
        // base: Align object.
        static void xform(snap atcrop, snap atgrow, si32& coor, si32& size, si32& width)
        {
            switch (size > width ? atcrop : atgrow)
            {
                case snap::head:   coor = 0;                  break;
                case snap::tail:   coor = width - size;       break;
                case snap::center: coor = (width - size) / 2; break;
                case snap::both:
                case snap::none: break;
            }
        }
        // base: Recalc actual area (ext rect) for the object.
        void recalc(rect& new_area)
        {
            if (base::hidden) return;
            auto required = new_area;
            new_area -= base::extpad;
            new_area.size = base::locked ? base::region.size
                                         : std::clamp(new_area.size, base::min_sz, base::max_sz);
            auto nested_area = rect{ dot_00, new_area.size } - base::intpad;
            deform(nested_area);
            new_area.size = nested_area.size + base::intpad;
            new_area += base::extpad;
            if ((required.size.x < new_area.size.x && base::atcrop.x == snap::both)
             || (required.size.x > new_area.size.x && base::atgrow.x == snap::both))
            {
                required.size.x = new_area.size.x;
            }
            if ((required.size.y < new_area.size.y && base::atcrop.y == snap::both)
             || (required.size.y > new_area.size.y && base::atgrow.y == snap::both))
            {
                required.size.y = new_area.size.y;
            }
            base::socket = new_area;
            new_area = required;
        }
        // base: Apply new area (ext rect) and notify subscribers.
        void accept(rect new_area)
        {
            xform(atcrop.x, atgrow.x, socket.coor.x, socket.size.x, new_area.size.x);
            xform(atcrop.y, atgrow.y, socket.coor.y, socket.size.y, new_area.size.y);
            std::swap(new_area, base::socket);
            new_area -= base::extpad;
            bell::signal(tier::release, e2::area, new_area);
            base::region = new_area;
        }
        // base: Notify about appoved area (ext rect) for the object.
        void notify(rect new_area, bool apply = true)
        {
            if (base::hidden) return;
            auto nested_area = rect{ dot_00, base::socket.size };
            nested_area -= base::extpad;
            nested_area -= base::intpad;
            inform(nested_area);
            if (apply) accept(new_area);
        }
        // base: Change object area (ext rect), and return delta.
        void change(rect new_area)
        {
            recalc(new_area);
            notify(new_area);
        }
        // base: Resize relative anchor point. The object is responsible for correcting the anchor point during deforming. Return new area of object.
        auto resize(twod new_size, bool apply = true)
        {
            auto anchored = base::anchor;
            auto new_area = base::region;
            new_area.size = new_size;
            new_area += base::extpad;
            recalc(new_area);
            notify(new_area, faux);
            new_area.coor += anchored - base::anchor;
            base::socket = new_area;
            if (apply) accept(new_area);
            return new_area;
        }
        // base: Move and return delta.
        auto moveto(twod new_coor)
        {
            base::socket.coor = new_coor;
            base::socket.size = base::region.size;
            auto new_area = base::socket;
            auto old_coor = base::region.coor;
            bell::signal(tier::release, e2::area, new_area);
            base::region.coor = new_area.coor;
            return base::region.coor - old_coor;
        }
        // base: Dry run. Recheck current position.
        auto moveto()
        {
            return moveto(base::region.coor);
        }
        // base: Move by the specified step and return the coor delta.
        auto moveby(twod step)
        {
            return moveto(base::region.coor + step);
        }
        // base: Dry run current area size value.
        auto resize()
        {
            return resize(base::region.size);
        }
        // base: Resize by step, and return size delta.
        auto sizeby(twod step)
        {
            auto old_size = base::region.size;
            auto new_size = old_size + step;
            return resize(new_size).size - old_size;
        }
        // base: Resize and move, and return delta.
        auto extend(rect new_area)
        {
            auto old_area = base::region;
            if (new_area.size == base::region.size) moveto(new_area.coor);
            else                                    change(new_area + base::extpad);
            auto delta = base::region;
            delta -= old_area;
            return delta;
        }
        // base: Mark the visual subtree as requiring redrawing.
        void strike(rect area)
        {
            if (auto parent_ptr = parent())
            {
                area.coor += base::region.coor;
                parent_ptr->deface(area);
            }
        }
        // base: Mark the visual subtree as requiring redrawing.
        void strike()
        {
            strike(base::region);
        }
        // base: Mark the form and its subtree as requiring redrawing.
        virtual void deface(rect area)
        {
            base::wasted = true;
            strike(area);
        }
        // base: Mark the form and its subtree as requiring redrawing.
        void deface()
        {
            deface(base::region);
        }
        // base: Going to rebuild visual tree. Retest current size, ask parent if it is linked.
        template<bool Forced = faux>
        void reflow()
        {
            auto parent_ptr = parent();
            if (parent_ptr && (!base::master || (Forced && (base::family != base::reflow_root)))) //todo unify -- See basewindow in vtm.cpp
            {
                parent_ptr->reflow<Forced>();
            }
            else change(base::region + base::extpad);
        }
        // base: Remove the form from the visual tree.
        void detach()
        {
            if (auto parent_ptr = parent())
            {
                strike();
                parent_ptr->remove(This());
            }
        }
        // base: Remove visual tree branch.
        void destroy()
        {
            auto lock = bell::sync();
            auto shadow = This();
            if (auto parent_ptr = parent())
            {
                parent_ptr->destroy();
            }
            detach();
        }
        // base: Recursively calculate global coordinate.
        void global(auto& coor)
        {
            coor -= base::region.coor;
            if (auto parent_ptr = parent())
            {
                parent_ptr->global(coor);
            }
        }
        // base: Recursively find the root of the visual tree.
        netxs::sptr<bell> gettop() override
        {
            auto parent_ptr = parent();
            if (!base::master && parent_ptr) return parent_ptr->gettop();
            else                             return This();
        }
        // base: Invoke a lambda with parent as a parameter.
        // Usage example:
        //     toboss([&](auto& parent_ptr) { c.fuse(parent.filler); });
        template<class P>
        void toboss(P proc)
        {
            if (auto parent_ptr = parent())
            {
                proc(*parent_ptr);
            }
        }
        // base: Execute the proc along the entire visual tree.
        template<class P, bool Plain = std::is_same_v<void, std::invoke_result_t<P>>>
        void diveup(P proc)
        {
            if constexpr (Plain) proc();
            else                 if (!proc()) return;
            if (auto parent_ptr = parent())
            {
                parent_ptr->diveup(proc);
            }
        }
        // base: Fire an event on yourself and pass it parent if not handled.
        // Warning: The parameter type is not checked/casted.
        // Usage example:
        //          base::raw_riseup(tier::preview, e2::form::prop::ui::header, txt);
        void raw_riseup(si32 Tier, hint event_id, auto& param, bool forced = faux)
        {
            //todo make it flat
            auto lock = bell::sync();
            bell::signal(Tier, event_id, param);
            if (forced)
            {
                base::toboss([&](auto& boss)
                {
                    boss.base::raw_riseup(Tier, event_id, param, forced);
                });
            }
            else
            {
                if (!bell::accomplished(Tier))
                {
                    base::toboss([&](auto& boss)
                    {
                        boss.base::raw_riseup(Tier, event_id, param, forced);
                    });
                }
            }
        }
        // base: Fire an event on yourself and pass it parent if not handled.
        // Usage example:
        //          base::riseup(tier::preview, e2::form::prop::ui::header, txt);
        template<class Event>
        auto riseup(si32 Tier, Event, Event::type&& param = {}, bool forced = faux)
        {
            raw_riseup(Tier, Event::id, param, forced);
            return param;
        }
        template<class Event>
        void riseup(si32 Tier, Event, Event::type& param, bool forced = faux)
        {
            raw_riseup(Tier, Event::id, param, forced);
        }
        template<class Event>
        void riseup(si32 Tier, Event, Event::type const& param, bool forced = faux)
        {
            raw_riseup(Tier, Event::id, param, forced);
        }
        void limits(twod new_min_sz = -dot_11, twod new_max_sz = -dot_11)
        {
            base::min_sz = new_min_sz.less(dot_00, skin::globals().min_value, new_min_sz);
            base::max_sz = new_max_sz.less(dot_00, skin::globals().max_value, new_max_sz);
        }
        void alignment(bind new_atgrow, bind new_atcrop = { snap::none, snap::none })
        {
            base::atgrow = new_atgrow;
            base::atcrop.x = new_atcrop.x == snap::none ? new_atgrow.x : new_atcrop.x;
            base::atcrop.y = new_atcrop.y == snap::none ? new_atgrow.y : new_atcrop.y;
        }
        void setpad(dent new_intpad, dent new_extpad = {})
        {
            base::intpad = new_intpad;
            base::extpad = new_extpad;
        }
        // base: Render to the canvas. Trim = trim viewport to the nested object region.
        template<bool Forced = faux>
        void render(face& canvas, bool trim = true, bool pred = true, bool post = true)
        {
            if (hidden) return;
            if (auto context = canvas.change_basis<Forced>(base::region, trim)) // Basis = base::region.coor.
            {
                if (pred) bell::signal(tier::release, e2::render::background::prerender, canvas);
                if (post) bell::signal(tier::release, e2::postrender, canvas);
            }
        }

    protected:
        virtual void deform(rect& /*new_area*/) {}
        virtual void inform(rect /*new_area*/) {}
        // base: Remove nested object.
        virtual void remove(sptr item_ptr)
        {
            auto head = subset.begin();
            auto tail = subset.end();
            auto iter = std::find_if(head, tail, [&](auto& c){ return c == item_ptr; });
            if (iter != tail)
            {
                auto backup = This();
                subset.erase(iter);
                item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
            }
        }
        // base: Update nested object.
        virtual void replace(sptr old_item_ptr, sptr new_item_ptr)
        {
            auto head = subset.begin();
            auto tail = subset.end();
            auto iter = std::find_if(head, tail, [&](auto& c){ return c == old_item_ptr; });
            if (iter != tail)
            {
                auto backup = This();
                auto pos = subset.erase(iter);
                old_item_ptr->bell::signal(tier::release, e2::form::upon::vtree::detached, backup);
                subset.insert(pos, new_item_ptr);
                new_item_ptr->bell::signal(tier::release, e2::form::upon::vtree::attached, backup);
            }
        }
        virtual ~base() = default;

    public:
        base(auth& indexer, size_t nested_count = 0)
            : bell{ indexer },
              subset{ nested_count },
              min_sz{ skin::globals().min_value },
              max_sz{ skin::globals().max_value },
              wasted{ true },
              hidden{ faux },
              locked{ faux },
              master{ faux },
              family{ type::client }
        {
            LISTEN(tier::release, e2::cascade, proc)
            {
                auto backup = This();
                auto keepon = proc(backup);
                if (!keepon) this->bell::expire(tier::release);
            };
            LISTEN(tier::release, e2::form::upon::vtree::attached, parent_ptr)
            {
                if (!master)
                {
                    parent_ptr->LISTEN(tier::release, e2::cascade, proc, relyon)
                    {
                        auto backup = This(); // Object can be deleted inside proc.
                        backup->bell::signal(tier::release, e2::cascade, proc);
                    };
                }
                father = parent_ptr;
                // Propagate form events up to the visual branch ends (children).
                // Exec after all subscriptions.
                //todo implement via e2::cascade
            };
            LISTEN(tier::release, e2::form::upon::vtree::any, parent_ptr) // any: Run after all.
            {
                if (this->bell::protos(tier::release, e2::form::upon::vtree::detached))
                {
                    relyon.reset();
                }
                if (parent_ptr && !hidden) parent_ptr->base::reflow();
            };
            LISTEN(tier::release, e2::render::background::any, parent_canvas)
            {
                     if (base::filler.xy())   parent_canvas.fill(cell::shaders::fusefull(base::filler));
                else if (base::filler.link()) parent_canvas.fill(cell::shaders::onlyid(bell::id));
            };
        }
    };

    struct input_fields_t
    {
        std::list<std::future<regs>> futures;
        regs fields;
        id_t gear_id = {};
        si32 acpStart = {};
        si32 acpEnd = {};

        void promise(auto& tasks)
        {
            auto& new_promise = tasks.emplace_back();
            futures.emplace_back(new_promise.get_future());
        }
        void set_value(rect r)
        {
            fields.push_back(r);
        }
        void set_value(auto&& rects)
        {
            fields.insert(fields.end(), rects.begin(), rects.end());
        }
        auto wait_for(span t = 400ms)
        {
            auto timeout = datetime::now() + t;
            for (auto& f : futures)
            {
                if (std::future_status::ready == f.wait_until(timeout))
                {
                    set_value(f.get());
                }
            }
            return std::move(fields);
        }
    };
    struct input_fields_handler
    {
        base&                         owner; // input_fields_handler: .
        std::list<std::promise<regs>> tasks; // input_fields_handler: .

        void send_input_fields_request(auto& boss, auto& inputfield_request) // Send request without ui sync.
        {
            inputfield_request.promise(tasks);
            boss.stream.s11n::req_input_fields.send(boss, inputfield_request);
        }

        input_fields_handler(auto& boss)
            : owner{ boss }
        {
            boss.LISTEN(tier::release, ui::e2::command::request::inputfields, inputfield_request)
            {
                send_input_fields_request(boss, inputfield_request);
            };
        }
        void handle(s11n::xs::ack_input_fields lock)
        {
            if (tasks.size())
            {
                auto& list = lock.thing.field_list;
                auto offset = dot_00;
                owner.global(offset);
                for (auto& r : list) r.coor -= offset;
                tasks.front().set_value(std::move(list));
                tasks.pop_front();
            }
        }
    };
}