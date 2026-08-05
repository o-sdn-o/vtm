// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

namespace netxs::x11
{
    using fd_t = os::fd_t;

    auto serialize_list(text& yield, auto& packet, auto const& payload)
    {
        using field_t = std::optional<ui32>;
        auto vmask = 0u;
        auto count = sizeof(payload) / sizeof(field_t);
        auto field = reinterpret_cast<field_t const*>(&payload);
        auto start = yield.size();
        yield += view{ (char*)&packet, sizeof(packet) };
        for (auto i = 0u; i < count; ++i)
        {
            if (field[i].has_value())
            {
                auto v = field[i].value();
                yield += view{ (char*)&v, sizeof(v) };
                vmask |= 1u << i;
            }
        }
        auto& ref = *reinterpret_cast<std::decay_t<decltype(packet)>*>(yield.data() + start);
        ref.value_mask = vmask;
        ref.length     = sizeof(packet) / 4 + std::popcount(vmask);
    }
    auto serialize_str(text& yield, auto& packet, auto const& data)
    {
        static constexpr auto is_string = requires{ data.size(); };
        auto data_bytes = 0ul;
        if constexpr (is_string) // String.
        {
            data_bytes = data.size();
            packet.data_len = data.size();
        }
        else // POD.
        {
            data_bytes = sizeof(data);
            packet.data_len = (ui32)(sizeof(data) * 8 / packet.format);
        }
        packet.length = (ui16)(sizeof(packet) / 4 + (data_bytes + 3) / 4);
        yield += view{ (char*)&packet, sizeof(packet) };
        if constexpr (is_string)
        {
            yield += view{ data.data(), data_bytes };
            yield += "\0\0\0"s.substr(0, 4 - (data_bytes % 4));
        }
        else
        {
            yield += view{ (char*)&data, data_bytes };
        }
    }

    #pragma pack(push, 1)
    namespace motif
    {
        static constexpr auto decorations = 1 << 1;
        struct hints // Motif WM Hints to disable decorations.
        {
            ui32 flags       = 2; // MWM_HINTS_DECORATIONS
            ui32 functions   = 0;
            ui32 decorations = 0; // 0: No decors.
            ui32 input_mode  = 0;
            ui32 status      = 0;
        };
    }
    namespace req
    {
        struct create_window // Opcode 1 (create window). This request generates a CreateNotify event.
        {
            byte opcode = 1;
            byte depth = 32;          // Color depth.
            ui16 length;              // Packet size in 4-byte words.
            ui32 window_id;           // New Window ID.
            ui32 parent_id;           // root_window_id.
            si16 x;                   // Initial coords in px.
            si16 y;                   //
            ui16 width;               // Initial size in px (not including the border).
            ui16 height;              //
            ui16 border_width = 0;
            ui16 window_class = 1;    // 0: CopyFromParent, 1: InputOutput, 2: InputOnly (input events and painting).
            ui32 visual_id;           // 0: CopyFromParent.
            ui32 value_mask;          // Payload bits.

            struct payload
            {
                std::optional<ui32> background_pixmap;     // 4 0: none, 1: ParentRelative or PIXMAP
                std::optional<ui32> background_pixel;      // 4 argb
                std::optional<ui32> border_pixmap;         // 4 0: CopyFromParent or PIXMAP
                std::optional<ui32> border_pixel;          // 4 argb
                std::optional<ui32> bit_gravity;           // 1 BITGRAVITY (Forget) defines which region of the window should be retained if the window is resized
                std::optional<ui32> win_gravity;           // 1 WINGRAVITY (NorthWest) defines how the window should be repositioned if the parent is resized
                std::optional<ui32> backing_store;         // 1 0: NotUseful, 1: WhenMapped, 2: Always
                std::optional<ui32> backing_planes;        // 4 (default: all ones)
                std::optional<ui32> backing_pixel;         // 4 argb (0x00000000)
                std::optional<ui32> override_redirect;     // 1 bool  specifies whether map and configure requests on this window should override a SubstructureRedirect on the parent, typically to inform a window manager not to tamper with the window
                std::optional<ui32> save_under;            // 1 bool  If true, the server is advised that when this window is mapped, saving the contents of windows it obscures would be beneficial
                std::optional<ui32> event_mask;            // 4 SETofEVENT
                std::optional<ui32> do_not_propagate_mask; // 4 SETofDEVICEEVENT
                std::optional<ui32> colormap_id;           // 4 0: CopyFromParent or COLORMAP
                std::optional<ui32> cursor_id;             // 4 0: None or CURSOR
            };
            auto serialize(text& yield, payload p) { x11::serialize_list(yield, *this, p); }
        };
        struct change_window_attrs // Opcode 2 (change window attributes).
        {
            byte opcode = 2;
            byte pad    = 0;
            ui16 length;
            ui32 window_id;
            ui32 value_mask; // Bitfield specifies attr list.

            using payload = create_window::payload;
            auto serialize(text& yield, payload p) { x11::serialize_list(yield, *this, p); }
        };
        struct destroy_window // Opcode 4 (destroy window).
        {
            byte opcode = 4;
            byte pad    = 0;
            ui16 length = 2;
            ui32 window_id;
        };
        struct map_window // Opcode 8 (show window).
        {
            byte opcode = 8;
            byte pad    = 0;
            ui16 length = 2;
            ui32 window_id;
        };
        struct unmap_window // Opcode 10 (hide window).
        {
            byte opcode = 10;
            byte pad    = 0;
            ui16 length = 2;
            ui32 window_id;
        };
        struct configure_window // Opcode 12 (configure window).
        {
            byte opcode = 12;
            byte pad1   = 0;
            ui16 length;
            ui32 window_id;
            ui16 value_mask; // value bit list
            ui16 pad2   = 0;
            // payload ... value-list
            //             2 si16 x
            //             2 si16 y
            //             2 ui16 width
            //             2 ui16 height
            //             2 ui16 border-width
            //             4 ui32 sibling (window id)
            //             1 byte stack-mode
            //                    0 Above
            //                    1 Below
            //                    2 TopIf
            //                    3 BottomIf
            //                    4 Opposite
        };
        struct intern_atom // Opcode 16 (InternAtom)
        {
            struct reply
            {
                byte type;    // Always 1 (Reply).
                byte pad0;
                ui16 sequence;
                ui32 length;  // Always 0.
                ui32 atom_id; // Requested Atom ID.
                ui32 pad2[5];
            };
            byte opcode = 16;
            byte only_if_exists; // 0: Create if absent, 1: Only if exists.
            ui16 length;         // (sizeof(intern_atom) + name_len + padding) / 4
            ui16 data_len;       // Name string length in bytes.
            ui16 pad = 0;

            auto serialize(text& yield, view name) { x11::serialize_str(yield, *this, name); }
        };
        struct change_property // Opcode 18 (change property).
        {
            byte opcode    = 18;
            byte mode      = 0;  // 0: Replace
            ui16 length;
            ui32 window_id;
            ui32 property;       // Atom (e.g., WM_NAME)
            ui32 type;           // Atom (e.g., STRING)
            byte format    = 32; // Payload unit format (e.g., 8: 8-bit chars (string), 32: 32-bit words)
            byte pad[3]    = {};
            ui32 data_len;       // Payload size in format units.

            auto serialize(text& yield, auto const& data) { x11::serialize_str(yield, *this, data); }
        };
        struct get_property // Opcode 20 (get property)
        {
            struct reply
            {
                byte type = 1;    // Always 1 (Reply).
                byte format;      // Bits in word.
                ui16 sequence;
                ui32 length;      // Payload length in 32-bit words.
                ui32 prop_type;   // Property type.
                ui32 bytes_after; // Bytes remains.
                ui32 value_len;   // Item size (array step).
                ui32 pad[3];
                // payload...
            };
            byte opcode = 20;
            byte remove = 0;      // 1: Delete property after read, 0: Do nothing.
            ui16 length = 6;
            ui32 window_id;
            ui32 property;        // Property atom.
            ui32 prop_type;       // Property type, 0: for any. (e.g., XA_WINDOW=33)
            ui32 long_offset = 0; // Offset from beginning in 32-bit words.
            ui32 long_length = 1; // Read ammount in 32-bit words.
        };
        struct get_input_focus // Opcode 43 (get_input focus).
        {
            struct reply
            {
                byte type;
                byte revertTo; // Focus destination after focused window deletion: 0: ToNone, 1: ToRoot, 2: ToParent.
                ui16 sequence;
                ui32 length;
                ui32 focused_window_id;
                ui32 pad[5];
            };
            byte opcode = 43;
            byte pad    = 0;
            ui16 length = 1;
        };
        struct create_gc // Opcode 55 (create graphical context).
        {
            byte opcode = 55;
            byte pad    = 0;
            ui16 length = 4;     // 16 bytes / 4 = 4 words
            ui32 gc_id;          // Generating id
            ui32 drawable;       // Our window ID (or root window)
            ui32 value_mask = 0; // No additional attributes
        };
        struct create_colormap // Opcode 78 (create colormap)
        {
            byte opcode = 78;
            byte alloc  = 0;  // 0: None, 1: All
            ui16 length = 4;
            ui32 colormap_id; // Our side XID.
            ui32 window_id;   // Window ID (root_window_id).
            ui32 visual_id;   // Some argb Visual ID.
        };
        struct query_extension // Opcode 98 (query extension, query MIT-SHM).
        {
            struct reply
            {
                byte status; // 1
                byte pad;
                ui16 sequence;
                ui32 length;
                byte present; // 1 if supported.
                byte major_opcode; // opcode for extension.
                byte first_event;
                byte first_error;
                byte pad2[20];
            };
            byte opcode = 98;
            byte pad1   = 0;
            ui16 length;
            ui16 data_len; // Extension name length in bytes.
            ui16 pad2   = 0;

            auto serialize(text& yield, view name) { x11::serialize_str(yield, *this, name); }
        };
        namespace shm // SHM Minor Opcodes: 0:QueryVersion, 1:Attach, 2:Detach, 3:PutImage, 4:GetImage, 5:CreatePixmap, 6:AttachFd, 7:CreateSegment
        {
            struct query_version // ShmQueryVersion (Minor Opcode 0)
            {
                struct reply // Always 32 bytes.
                {
                    byte status;         // 1: Success.
                    byte pad1;
                    ui16 sequence;       // X11 request sequence number.
                    ui32 length;         // Attached payload length (0)
                    ui16 major_version;  // (required 1)
                    ui16 minor_version;  // (required >= 2)
                    ui16 uid;
                    ui16 gid;
                    byte pixmap_format;
                    byte pad2[15];
                };
                byte major_opcode;     // MIT-SHM major_opcode.
                byte minor_opcode = 0; // 0: QueryVersion.
                ui16 length       = 1; // 4 bytes / 4 = 1 word.
            };
            struct attach_fd // ShmAttachFd (Minor Opcode 6) - bind SHM file descriptor with x-serveer (via ::sendmsg()).
            {
                byte major_opcode;     // MIT-SHM major_opcode.
                byte minor_opcode = 6; // 6: AttachFd.
                ui16 length       = 3;
                ui32 shm_seg_id;       // Our side generated unique resource ID (XID).
                byte read_only    = 0;
                byte pad[3]       = {};
            };
            struct detach // ShmDetach (Minor Opcode 2)
            {
                byte major_opcode;     // MIT-SHM major_opcode.
                byte minor_opcode = 2; // 2: Detach.
                ui16 length       = 2;
                ui32 shm_seg_id;       // Detached segment ID (XID).
            };
            struct put_image // ShmPutImage (Minor Opcode 3) - immediately output from SHM to screen.
            {
                struct reply // ShmCompletionEvent
                {
                    byte type;          // shm_completion_event
                    byte pad0;
                    ui16 sequence;
                    ui32 drawable;      // Dest Window ID
                    ui16 minor_opcode;  // 3 (ShmPutImage).
                    byte major_opcode;  // shm_major_opcode.
                    byte pad1;
                    ui32 shmseg;        // Linked segment ID.
                    ui32 offset;        // Segment offset.
                    ui32 pad2;
                    ui32 pad3;
                    ui32 pad4;
                };
                byte major_opcode;      // shm_major_opcode.
                byte minor_opcode = 3;  // 3: PutImage.
                ui16 length       = 10; // 40 bytes / 4 = 10 words.
                ui32 drawable;          // Our window ID (fg_w).
                ui32 gc_id;             // Graphical context.
                ui16 total_width;       // Buffer width/height in SHM
                ui16 total_height;      //
                ui16 src_x;             // Clip coor.
                ui16 src_y;             //
                ui16 src_width;         // Clip size.
                ui16 src_height;        //
                si16 dst_x;             // Dest coor.
                si16 dst_y;             //
                byte depth      = 32;   // 32-bit ARGB
                byte format     = 2;    // 2: ZPixmap
                byte send_event = 1;    // 0: Don't notify on output end; 1: Send event on output end (reply).
                byte pad        = 0;
                ui32 shm_seg_id;        // Linked segment ID.
                ui32 offset;            // Segment offset.
            };
        }
        namespace xfixes
        {
            struct query_version 
            {
                struct reply
                {
                    byte status;         // 1: Reply
                    byte pad1;
                    ui16 sequence;
                    ui32 length;
                    ui32 major_version;
                    ui32 minor_version;
                    ui32 pad2[4];
                };
                byte major_opcode;       // xfixes_major_opcode.
                byte minor_opcode = 0;   // 0: XFixesQueryVersion.
                ui16 length = 3;
                ui32 client_major_version = 2; // Required 2.0+ (Window Shape(Input) Region).
                ui32 client_minor_version = 0;
            };
            struct set_window_shape_region // XFixesSetWindowShapeRegion (Minor opcode 21).
            {
                byte major_opcode;      // xfixes_major_opcode.
                byte minor_opcode = 21; // 21: XFixesSetWindowShapeRegion
                ui16 length       = 5;
                ui32 window_id;         // Dest window ID.
                byte shape_kind   = 1;  // 1: ShapeInput (input region).
                byte pad[3]       = {};
                si16 x_offset     = 0;  // Region offset.
                si16 y_offset     = 0;  //
                ui32 region       = 0;  // 0 (None): empty region (make window transparent for mouse).
            };
        }
        struct noop // Opcode 127 (NoOperation).
        {
            byte opcode = 127;
            byte pad    = 0;
            ui16 length = 1; // + n for payload
            // arbitrary payload ...
        };
    }
    namespace event
    {
        namespace mask
        {
            #define eventmasks         \
                X(KeyPress            )\
                X(KeyRelease          )\
                X(ButtonPress         )\
                X(ButtonRelease       )\
                X(EnterWindow         )\
                X(LeaveWindow         )\
                X(PointerMotion       )\
                X(PointerMotionHint   )\
                X(Button1Motion       )\
                X(Button2Motion       )\
                X(Button3Motion       )\
                X(Button4Motion       )\
                X(Button5Motion       )\
                X(ButtonMotion        )\
                X(KeymapState         )\
                X(Exposure            )\
                X(VisibilityChange    )\
                X(StructureNotify     )\
                X(ResizeRedirect      )\
                X(SubstructureNotify  )\
                X(SubstructureRedirect)\
                X(FocusChange         )\
                X(PropertyChange      )\
                X(ColormapChange      )\
                X(OwnerGrabButton     )
            static constexpr auto _counter = __COUNTER__ + 1;
            #define X(a) static constexpr auto a = 1 << (__COUNTER__ - _counter);
                eventmasks
            #undef X
            #undef eventmasks
        }
        #define eventlist      \
            X(Error           )\
            X(Reply           )\
            X(KeyPress        )\
            X(KeyRelease      )\
            X(ButtonPress     )\
            X(ButtonRelease   )\
            X(MotionNotify    )\
            X(EnterNotify     )\
            X(LeaveNotify     )\
            X(FocusIn         )\
            X(FocusOut        )\
            X(KeymapNotify    )\
            X(Expose          )\
            X(GraphicsExpose  )\
            X(NoExpose        )\
            X(VisibilityNotify)\
            X(CreateNotify    )\
            X(DestroyNotify   )\
            X(UnmapNotify     )\
            X(MapNotify       )\
            X(MapRequest      )\
            X(ReparentNotify  )\
            X(ConfigureNotify )\
            X(ConfigureRequest)\
            X(GravityNotify   )\
            X(ResizeRequest   )\
            X(CirculateNotify )\
            X(CirculateRequest)\
            X(PropertyNotify  )\
            X(SelectionClear  )\
            X(SelectionRequest)\
            X(SelectionNotify )\
            X(ColormapNotify  )\
            X(ClientMessage   )\
            X(MappingNotify   )\
            X(GenericEvent    )\
            X(_last           )
        static constexpr auto _counter = __COUNTER__ + 1;
        #define X(a) static constexpr auto a = __COUNTER__ - _counter;
            eventlist
        #undef X
        auto str(si32 e)
        {
            static constexpr auto el = std::to_array(
            {
                #define X(a) #a##sv, // "EventName"sv
                    eventlist
                #undef X
            });
            return e >= 0 && e < _last ? el[e] : "undef"sv;
        }
        #undef eventlist
        struct any
        {
            byte type; // Bit 7 may be set if the event is artificially generated (SendEvent).
            byte detail;
            ui16 sequence;
            ui32 length; // Reply's payload length in 32-bit words.
            ui32 pad[6];
        };
        struct error // Type 0
        {
            byte type;
            byte error_code;   // Error code (1: BadRequest, 2: BadValue, 3: BadWindow, 128+: Extensions...).
            ui16 sequence;     // Request stamp.
            ui32 bad_value;    // Invalid XID.
            ui16 minor_opcode; // Request's minor opcode.
            byte major_opcode; // Request's major opcode.
            byte pad[21];
        };
        struct mouse_click // Type 4 (button press) and 5 (button release).
        {
            byte type;
            byte button; // 1: Left, 2: Middle, 3: Right, 4: WheelUp, 5: WheelDown, 6: WheelLeft, 7: WheelRight
            ui16 sequence;
            ui32 time;
            ui32 root_window;
            ui32 event_window;
            ui32 child_window;
            si16 root_x;
            si16 root_y;
            si16 event_x; // Relative X
            si16 event_y; // Relative Y
            ui16 state;   // Keybd modifiers (Shift, Ctrl, Alt...)
            byte same_screen;
            byte pad;
        };
        struct motion // Type 6 (motion notify).
        {
            byte type;
            byte detail; // 0: Normal, 1: Hint
            ui16 sequence;
            ui32 time;
            ui32 root_window;
            ui32 event_window;
            ui32 child_window;
            si16 root_x;
            si16 root_y;
            si16 event_x; // Relative (to window) mouse Х.
            si16 event_y; // Relative (to window) mouse Y.
            ui16 state;   // Mouse buttons bitfield + keybd mods (Ctrl, Shift, etc.)
            byte same_screen;
            byte pad;
        };
        struct crossing // Type 7 (enter notify) and 8 (leave notify), a-la TrackMouseEvent/WM_MOUSELEAVE.
        {
            byte type;
            byte detail;
            ui16 sequence;
            ui32 time;
            ui32 root_window;
            ui32 event_window;
            ui32 child_window;
            si16 root_x;
            si16 root_y;
            si16 event_x;
            si16 event_y;
            ui16 state;
            byte mode;
            byte flags; // Bits: same_screen, focus
        };
        struct focus // Type 9 (focus on) and 10 (focus off).
        {
            byte type;
            byte mode; // 0: Normal, 1: Grab, 2: Ungrab
            ui16 sequence;
            ui32 window;
            byte detail;
            byte pad[23];
        };
        struct configure // Type 22 (configure notify) a-la WM_SIZE/WM_MOVE.
        {
            byte type;
            byte pad;
            ui16 sequence;
            ui32 event_window;
            ui32 window;
            ui32 above_sibling;
            si16 x;
            si16 y;
            ui16 width;  // New width.
            ui16 height; // New heigth.
            ui16 border_width;
            byte override_redirect;
            byte pad2;
        };
        struct property_notify // Type 28 (property notify)
        {
            byte type;
            byte pad0;
            ui16 sequence;
            ui32 window_id; // Source window ID.
            ui32 atom;      // Changed property atom.
            ui32 time;      // Timestamp in ms.
            byte state;     // 0: PropertyNewValue, 1: PropertyDelete.
            byte pad1[15];
        };
        struct client_message // Type 33 (client message).
        {
            byte type;
            byte format; // ? 32 bits
            ui16 sequence;
            ui32 window;
            ui32 message_type; // Procotol atom (e.g., WM_PROTOCOLS)
            ui32 data32[5];    // Payload. If data32[0]==atom_WM_DELETE_WINDOW -> Close window
        };
    }
    template<class T>
    struct data_n_size
    {
        auto data() { return (void*)this; }
        auto size() { return sizeof(T::s); }
    };
    struct session_t : data_n_size<session_t>
    {
        struct auth
        {
            struct reply
            {
                byte status;            // 0: Failed, 1: Success.
                byte pad1;
                ui16 major_version;
                ui16 minor_version;
                ui16 additional_length; // Payload length in 4-byte chunks.
                // payload ...
            };
            byte byte_order;       // 0x6c ('l') or 0x42 ('B')
            byte pad1;             //
            ui16 major_version;    // X_PROTOCOL
            ui16 minor_version;    // X_PROTOCOL_REVISION
            ui16 auth_proto_len;   //
            ui16 auth_data_len;    //
            ui16 pad2;             //
        };
        struct format : data_n_size<format>
        {
            struct
            {
                byte depth;          // 1 byte depth
                byte bits_per_pixel; // 1 byte bits_per_pixel
                byte scanline_pad;   // 1 byte scanline_pad
                byte pad[5];         // 5 pad  unused
            } s;
        };
        struct screen : data_n_size<screen>
        {
            struct depth : data_n_size<depth>
            {
                struct visual_type : data_n_size<visual_type>
                {
                    struct vclass
                    {
                        static constexpr auto StaticGray  = (byte)1;
                        static constexpr auto GrayScale   = (byte)2;
                        static constexpr auto StaticColor = (byte)3;
                        static constexpr auto PseudoColor = (byte)4;
                        static constexpr auto TrueColor   = (byte)5;
                        static constexpr auto DirectColor = (byte)6;
                    };
                    struct
                    {
                        ui32 visual_id;          // 4 ui32 visual_id
                        byte visual_class;       // 1 byte vclass
                        byte bits_per_rgb_value; // 1 byte bits_per_rgb_value
                        ui16 colormap_entries;   // 2 ui16 colormap_entries
                        ui32 red_mask;           // 4 ui32 red_mask
                        ui32 green_mask;         // 4 ui32 green_mask
                        ui32 blue_mask;          // 4 ui32 blue_mask
                        ui32 pad;                // 4 pad  unused
                    } s;
                };
                struct
                {
                    byte depth;               // 1 byte depth
                    byte pad1;                // 1 pad  unused
                    ui16 num_of_visual_types; // 2 n    number of visual_types in visuals
                    ui32 pad2;                // 4 pad  unused
                } s;
                std::vector<visual_type> list_of_visual_types; // 24*n  list_of_visual_types  visuals
            };
            struct
            {
                ui32 root_window_id;        // 4 ui32 WINDOW      root_window_id
                ui32 default_colormap;      // 4 ui32 COLORMAP    default_colormap
                ui32 white_pixel;           // 4 ui32             white_pixel
                ui32 black_pixel;           // 4 ui32             black_pixel
                ui32 current_input_masks;   // 4 ui32 SETofEVENT  current_input_masks
                ui16 width_in_pixels;       // 2 ui16             width_in_pixels
                ui16 height_in_pixels;      // 2 ui16             height_in_pixels
                ui16 width_in_millimeters;  // 2 ui16             width_in_millimeters
                ui16 height_in_millimeters; // 2 ui16             height_in_millimeters
                ui16 min_installed_maps;    // 2 ui16             min_installed_maps
                ui16 max_installed_maps;    // 2 ui16             max_installed_maps
                ui32 root_visual;           // 4 ui32 VisualId    root_visual
                byte backing_stores;        // 1 byte             backing_stores 0: Never, 1: WhenMapped, 2: Always
                byte save_unders;           // 1 byte BOOL        save_unders 0/1
                byte root_depth;            // 1 byte             root_depth
                byte number_of_depths;      // 1 byte             number of depths (list_of_depths) in allowed_depths
            } s;
            std::vector<depth> list_of_depths; // List of allowed_depths (n is always a multiple of 4)
        };
        struct
        {
            ui32 release_number;              // 4 ui32 buffer[0..3]   = release_number
            ui32 resource_id_base;            // 4 ui32 buffer[4..7]   = resource_id_base
            ui32 resource_id_mask;            // 4 ui32 buffer[8..11]  = resource_id_mask
            ui32 motion_buffer_size;          // 4 ui32 buffer[12..15] = motion_buffer_size
            ui16 vendor_length;               // 2 ui16 buffer[16..17] = vendor_length
            ui16 maximum_request_length;      // 2 ui16 buffer[18..19] = maximum_request_length
            byte number_of_screens;           // 1 byte buffer[20]     = number_of_screens in roots
            byte number_of_formats;           // 1 byte buffer[21]     = number_of_formats in pixmap_formats
            byte image_byte_order;            // 1 byte buffer[22]     = 0: LSBFirst, 1: MSBFirst
            byte bitmap_format_bit_order;     // 1 byte buffer[23]     = 0: LeastSignificant, 1: MostSignificant
            byte bitmap_format_scanline_unit; // 1 byte buffer[24]     = bitmap_format_scanline_unit
            byte bitmap_format_scanline_pad;  // 1 byte buffer[25]     = bitmap_format_scanline_pad
            byte min_keycode;                 // 1 byte buffer[26]     = min_keycode
            byte max_keycode;                 // 1 byte buffer[27]     = max_keycode
            byte pad[4];                      // 4 ui32 buffer[28..31] = unused
        } s;

        text                                  vendor_str;     // buffer[32..32+vendor_length] = vendor_str
        std::vector<format>                   pixmap_formats; // format * number_of_formats = pixmap_formats
        std::vector<screen>                   roots;          // screen * number_of_screens = roots (always a multiple of 4)

        ui32                                  argb_visual32_id = 0;
        ui32                                  argb_colormap_id = 0;

        ui32                                  atom_motif_wm_hints = 0; // Disable decoractions.
        ui32                                  atom_net_wm_state_skip_taskbar = 0; // Hide from the taskbar.
        ui32                                  atom_net_wm_state = 0;              //
        //ui32                                  atom_net_wm_window_type = 0;
        //ui32                                  atom_net_wm_window_type_combo = 0;
        //ui32                                  atom_compton_shadow = 0;
        ui32                                  atom_active_window = 0;
        ui32                                  atom_number_of_desktops = 0;
        ui32                                  atom_current_desktop = 0;
        ui32                                  atom_workarea = 0;

        byte                                  xfixes_major_opcode = 0;
        byte                                  xfixes_first_event = 0;

        byte                                  shm_major_opcode = 0;
        byte                                  shm_completion_event = 0;
        fd_t                                  shm_buffer_fd = os::invalid_fd;
        byte*                                 shm_buffer_ptr = {};
        size_t                                shm_buffer_len = {};
        ui32                                  shm_segmen_xid = {};
        size_t                                shm_buffer_offset = {};

        size_t                                current_frame_index = {};
        bool shm_ready_flag[2]   = { true, true }; // Buffer ready flags.

        sptr<os::ipc::stdcon>                 x11connection;  // Active X11 socket connection.
        generics::indexer_growing<ui32, 256>  resource_indexer; // Use growing indexer to avoid reusing indexes.

        struct seq_handler
        {
            using fx_t = std::function<void(x11::event::any const& ev, view payload)>;
            ui16 sequence;
            fx_t callback;
        };
        std::deque<seq_handler> reply_callbacks;
        std::mutex              mutex;
        ui16                    sequence_counter = 0; // Sent request counter.

        template<class R, class V = qiew, class P = noop>
        void sendrq(R request, V payload = {}, P callback = {})
        {
            auto lock = std::lock_guard{ mutex };
            sequence_counter++;
                auto packet = text{};
            if constexpr (requires{ request.serialize(packet, payload); })
            {
                request.serialize(packet, payload);
                x11connection->send(packet);
            }
            else
            {
                assert(!payload);
                x11connection->send(view{ (char*)&request, sizeof(request) });
            }
            if constexpr (!std::is_same_v<P, noop>)
            {
                reply_callbacks.push_back({ sequence_counter, std::move(callback) });
            }
        }
        auto parse_reply(x11::event::any& ev)
        {
            auto r = std::decay_t<decltype(reply_callbacks.front())>{};
            {
                auto lock = std::lock_guard{ mutex };
                if (reply_callbacks.size())
                {
                    r = std::move(reply_callbacks.front());
                    reply_callbacks.pop_front();
                }
            }
            if (!r.callback)
            {
                if constexpr (debugmode) log("%%unexpected Reply with empty queue", prompt::x11);
            }
            else
            {
                auto extra_data = text{};
                if (ev.length > 0)
                {
                    extra_data.resize(ev.length * sizeof(ui32));
                    x11connection->recv(extra_data.data(), extra_data.size()); // Blocking call.
                }
                if constexpr (debugmode) log("%%seq=%%", prompt::x11, r.sequence);
                r.callback(ev, extra_data);
            }
        }
        auto event_str(si32 e)
        {
            return e == shm_completion_event ? "ShmCompletionEvent" : x11::event::str(e);
        }
        template<bool B = true>
        auto str() const
        {
            if (roots.empty()) return "no screen roots"s;
            auto str = utf::fprint("%%Connected: id_base/mask=%%/%% root_window_id=0x%% screens=%% vendor='%%'\n", prompt::x11,
                utf::to_hex(s.resource_id_base),
                utf::to_hex(s.resource_id_mask),
                utf::to_hex(roots.front().s.root_window_id),
                (si32)s.number_of_screens,
                utf::debase<faux, faux>(vendor_str));
            str += pixmap_formats.size() ? utf::fprint("    pixmap_formats(%%):\n", pixmap_formats.size()) : "    no pixmap_formats\n";
            for (auto& format : pixmap_formats)
            {
                auto& pf = format.s;
                str += utf::fprint("\tdepth=%% bpp=%% scanline_pad=%%\n", (si32)pf.depth, (si32)pf.bits_per_pixel, (si32)pf.scanline_pad);
            }
            str += roots.size() ? utf::fprint("    root screens(%%):\n", roots.size()) : "    no screen roots\n";
            for (auto& root : roots)
            {
                auto& sc = root.s;
                str += utf::fprint("     root_window_id=0x"      , utf::to_hex(sc.root_window_id),
                                    "\n\t default_colormap="     , sc.default_colormap,
                                    "\n\t white_pixel=0x"        , utf::to_hex(sc.white_pixel),
                                    "\n\t black_pixel=0x"        , utf::to_hex(sc.black_pixel),
                                    "\n\t current_input_masks=0x", utf::to_hex(sc.current_input_masks),
                                    "\n\t width_in_pixels="      , sc.width_in_pixels,
                                    "\n\t height_in_pixels="     , sc.height_in_pixels,
                                    "\n\t width_in_millimeters=" , sc.width_in_millimeters,
                                    "\n\t height_in_millimeters=", sc.height_in_millimeters,
                                    "\n\t min_installed_maps="   , sc.min_installed_maps,
                                    "\n\t max_installed_maps="   , sc.max_installed_maps,
                                    "\n\t root_visual=0x"        , utf::to_hex(sc.root_visual),
                                    "\n\t backing_stores="       , (si32)sc.backing_stores,
                                    "\n\t save_unders="          , (si32)sc.save_unders,
                                    "\n\t root_depth="           , (si32)sc.root_depth,
                                    "\n\t number_of_depths="     , (si32)sc.number_of_depths,
                                    "\n");
                str += root.list_of_depths.size() ? utf::fprint("\t   depths(%%):\n", root.list_of_depths.size()) : "        no depths\n";
                for (auto& depth : root.list_of_depths)
                {
                    auto& d = depth.s;
                    str += utf::fprint("\t\t depth=%% num_of_visual_types=%%\n", (si32)d.depth, d.num_of_visual_types);
                    //str += depth.list_of_visual_types.size() ? utf::fprint("          visual_types(%%):\n", depth.list_of_visual_types.size()) : "          no visual_types\n";
                    //for (auto& vt : depth.list_of_visual_types)
                    //{
                    //    auto& v = vt.s;
                    //    str += utf::fprint("\tvisual_id=0x",              utf::to_hex(v.visual_id),
                    //                        "\n\t\t visual_class=",       (si32)v.visual_class,
                    //                        "\n\t\t bits_per_rgb_value=", (si32)v.bits_per_rgb_value,
                    //                        "\n\t\t colormap_entries=",   v.colormap_entries,
                    //                        "\n\t\t red_mask=0x",         utf::to_hex(v.red_mask),
                    //                        "\n\t\t green_mask=0x",       utf::to_hex(v.green_mask),
                    //                        "\n\t\t blue_mask=0x",        utf::to_hex(v.blue_mask),
                    //                        "\n");
                    //}
                }
            }
            if (str.back() == '\n') str.pop_back();
            return str;
        }
        auto new_resource_id()
        {
            auto current_idx = resource_indexer.get_new();
            auto resource_id = s.resource_id_base | (current_idx & s.resource_id_mask);
            return resource_id;
        }
        auto free_resource_id(ui32& resource_id)
        {
            resource_indexer.release(resource_id & s.resource_id_mask);
            resource_id = {};
        }
        auto detect_argb_32bit()
        {
            auto argb_supported = faux;
            for (auto& format : pixmap_formats) // Check if argb supported.
            {
                auto& pf = format.s;
                if (pf.depth == 32 && pf.bits_per_pixel == 32)
                {
                    argb_supported = true;
                    break;
                }
            }
            if (argb_supported && roots.size()) // Find visual_id with depth=32.
            for (auto& depth : roots.front().list_of_depths)
            {
                if (depth.s.depth == 32)
                if (depth.list_of_visual_types.size()) // Take first available visual_type.
                {
                    auto& v = depth.list_of_visual_types.front().s;
                    argb_visual32_id = v.visual_id;
                    argb_colormap_id = new_resource_id();
                    sendrq<x11::req::create_colormap>({ .colormap_id = argb_colormap_id,
                                                        .window_id   = roots.front().s.root_window_id,
                                                        .visual_id   = argb_visual32_id });
                    if constexpr (debugmode) log("%%ARGB visual id found: argb_visual32_id=0x%%", prompt::x11, utf::to_hex(argb_visual32_id));
                    return true;
                }
            }
            auto errmsg = utf::fprint("%%32-bit ARGB pixel format is not supported on X11 server\n", prompt::x11);
            errmsg += pixmap_formats.size() ? utf::fprint("    Supported pixmap formats(%%):\n", pixmap_formats.size()) : "    There are no pixmap formats\n";
            for (auto& format : pixmap_formats)
            {
                auto& pf = format.s;
                errmsg += utf::fprint("\tdepth=%% bpp=%% scanline_pad=%%\n", (si32)pf.depth, (si32)pf.bits_per_pixel, (si32)pf.scanline_pad);
            }
            log<faux>(errmsg);
            return faux;
        }
        template<class ExtensionQueryVersion>
        auto detect_extension(qiew extension_name, byte& major_opcode, byte& first_event, ui16 required_major_version, ui16 required_minor_version)
        {
            auto errdetails = text{};
            sendrq<x11::req::query_extension>({}, extension_name);
            auto reply = x11::req::query_extension::reply{};
            if (x11connection->recv((char*)&reply, sizeof(reply)).size() == sizeof(reply))
            if (reply.present)
            {
                major_opcode = reply.major_opcode;
                first_event  = reply.first_event;
                sendrq<ExtensionQueryVersion>({ .major_opcode = reply.major_opcode });
                auto v_reply = typename ExtensionQueryVersion::reply{};
                if (x11connection->recv((char*)&v_reply, sizeof(v_reply)).size() == sizeof(v_reply))
                if (v_reply.status == 1)
                if (v_reply.major_version > required_major_version || (v_reply.major_version == required_major_version && v_reply.minor_version >= required_minor_version)) // Check min version major.minor.
                {
                    if constexpr (debugmode) log("%%%% version %%.%% detected (ext_major_opcode=%% ext_completion_event=%%)", prompt::x11, extension_name, (si32)v_reply.major_version, (si32)v_reply.minor_version, (si32)major_opcode, (si32)first_event);
                    return true;
                }
                if (v_reply.status == 1)
                {
                    errdetails = utf::fprint("\n\t%% version %%.%% detected", extension_name, (si32)v_reply.major_version, (si32)v_reply.minor_version);
                }
                else
                {
                    errdetails = utf::fprint("\n\tFailed to receive %% version details", extension_name);
                }
            }
            auto errmsg = utf::fprint("%%The required %% extension (or required min version %%.%%) is missing", prompt::x11, extension_name, required_major_version, required_minor_version);
            log(errmsg + errdetails);
            return faux;
        }
        void send_shm_attach_fd(ui32 client_shmseg_xid)
        {
            auto request = x11::req::shm::attach_fd{ .major_opcode = shm_major_opcode,
                                                     .shm_seg_id   = client_shmseg_xid };
            auto iov = ::iovec{ .iov_base = &request,
                                .iov_len  = sizeof(request) };
            union // Ancillary Data
            {
                ::cmsghdr cm;
                char control[CMSG_SPACE(sizeof(int))];
            }
            control_buffer{};
            auto msg = ::msghdr{ .msg_name       = nullptr,
                                 .msg_namelen    = 0,
                                 .msg_iov        = &iov,
                                 .msg_iovlen     = 1,
                                 .msg_control    = control_buffer.control,
                                 .msg_controllen = sizeof(control_buffer.control) };
            auto cmsg = CMSG_FIRSTHDR(&msg);
            cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type  = SCM_RIGHTS;
            *reinterpret_cast<fd_t*>(CMSG_DATA(cmsg)) = shm_buffer_fd;
            auto lock = std::lock_guard{ mutex };
            sequence_counter++;
            auto bytes_sent = ::sendmsg(x11connection->handle.w, &msg, 0);
            if (bytes_sent == -1)
            {
                log("%%sendmsg failed during shm_attach_fd invocation", prompt::x11);
            }
        }
        void send_shm_detach_fd(ui32 client_shmseg_xid)
        {
            sendrq<x11::req::shm::detach>({ .major_opcode = shm_major_opcode,
                                            .shm_seg_id   = client_shmseg_xid });
            if constexpr (debugmode) log("%%Shared buffer segment XID %% is detached", prompt::x11, client_shmseg_xid);
        }
        auto create_window(ui32 master_window_id, ui32 new_window_id, ui32 new_gc_id, rect area)
        {
            if (!area) area = rect_11;
            sendrq<x11::req::create_window>({ .window_id = new_window_id,
                                              .parent_id = roots.front().s.root_window_id,
                                              .x         = (si16)area.coor.x,
                                              .y         = (si16)area.coor.y,
                                              .width     = (ui16)area.size.x,
                                              .height    = (ui16)area.size.y,
                                              .visual_id = argb_visual32_id },
                                            x11::req::create_window::payload{ .border_pixel = 0x00'000000u, // Own 32-bit ARGB border pixel value.
                                                                              .override_redirect = 0,       // 1: On.
                                                                              .event_mask        = x11::event::mask::KeymapState | x11::event::mask::KeyPress | x11::event::mask::KeyRelease
                                                                                                 | x11::event::mask::ButtonPress | x11::event::mask::ButtonRelease
                                                                                                 | x11::event::mask::EnterWindow | x11::event::mask::LeaveWindow
                                                                                                 | x11::event::mask::PointerMotion
                                                                                                 | x11::event::mask::Exposure
                                                                                                 | x11::event::mask::FocusChange
                                                                                                 | x11::event::mask::StructureNotify,
                                                                              .colormap_id       = argb_colormap_id });
            // Disable decorations.
            sendrq<x11::req::change_property>({ .window_id = new_window_id,
                                                .property  = atom_motif_wm_hints,   // Atom "_MOTIF_WM_HINTS".
                                                .type      = atom_motif_wm_hints }, // Atom "_MOTIF_WM_HINTS" (same).
                                            x11::motif::hints{ .flags = x11::motif::decorations, .decorations = 0 });

            if (area.size == dot_11)
            {
                // Remove sub-layers from taskbar.
                sendrq<x11::req::change_property>({ .window_id = new_window_id,
                                                    .property  = 68,   // Atom WM_TRANSIENT_FOR=68.
                                                    .type      = 33 }, // Atom XA_WINDOW=33.
                                                master_window_id);
                sendrq<x11::req::change_property>({ .window_id = new_window_id,
                                                    .property  = atom_net_wm_state, // Atom "_NET_WM_STATE".
                                                    .type      = 4 },               // Atom XA_ATOM=4.
                                                atom_net_wm_state_skip_taskbar);
                // Make sub-layer transparent for mouse.
                sendrq<x11::req::xfixes::set_window_shape_region>({ .major_opcode = xfixes_major_opcode,
                                                                    .window_id    = new_window_id });
            }

            // Disable shadows.
            //sendrq<x11::req::change_property>({ .window_id = new_window_id,
            //                                    .property  = atom_net_wm_window_type, // Atom "_NET_WM_WINDOW_TYPE".
            //                                    .type      = 4 },                     // Atom XA_ATOM=4.
            //                                atom_net_wm_window_type_combo);
            //sendrq<x11::req::change_property>({ .window_id = new_window_id,
            //                                    .property  = atom_compton_shadow,      // Atom "_COMPTON_SHADOW".
            //                                    .type      = 6 },                      // Atom XA_CARDINAL=6.
            //                                0u); // 0: off, 1: on.

            sendrq<x11::req::create_gc>({ .gc_id = new_gc_id, .drawable = new_window_id });

            if constexpr (debugmode) log("create window: window_id=%% parent_id=%% depth=%% visual_id=0x%% area=%% colormap_id=0x%%",
                utf::to_hex(new_window_id), utf::to_hex(roots.front().s.root_window_id), 32, utf::to_hex(argb_visual32_id), area, utf::to_hex(argb_colormap_id));
        }
        void window_set_title(ui32 window_id, qiew title)
        {
            sendrq<x11::req::change_property>({ .window_id = window_id,
                                                .property  = 39,   // Atom WM_NAME (predefined id = 39).
                                                .type      = 31,   // Atom STRING (predefined id = 31).
                                                .format    = 8 },  // Format (8: 8-bit chars (string)).
                                            title);
        }
        bool resize_shared_buffer(size_t size)
        {
            if (shm_buffer_len)
            {
                //todo implement delayed detach+copy
                send_shm_detach_fd(shm_segmen_xid);
                reset_shared_buffer();
                free_resource_id(shm_segmen_xid);
            }
            shm_buffer_fd =
                #if defined(__linux__)
                    ::memfd_create("x11_shm_buffer", MFD_CLOEXEC);
                #else
                    ::shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0600); // SHM_ANON - native anonymous descriptor in BSD.
                #endif
            if (shm_buffer_fd == os::invalid_fd)
            {
                log("%%Failed to create anonymous shared memory fd (errno=%%)", prompt::gui, os::error());
            }
            else
            {
                if (::ftruncate(shm_buffer_fd, size) == -1) // Set shm size.
                {
                    ::close(shm_buffer_fd);
                    shm_buffer_fd = os::invalid_fd;
                    log("%%Failed to truncate shared memory file to required size (errno=%%)", prompt::gui, os::error());
                }
                else
                {
                    auto mapped_ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_buffer_fd, 0);
                    if (mapped_ptr == MAP_FAILED)
                    {
                        ::close(shm_buffer_fd);
                        shm_buffer_fd = os::invalid_fd;
                        log("%%Failed to mmap shared memory descriptor (errno=%%)", prompt::gui, os::error());
                    }
                    else
                    {
                        shm_buffer_ptr = (byte*)mapped_ptr;
                        shm_buffer_len = size;
                        shm_segmen_xid = new_resource_id();
                        send_shm_attach_fd(shm_segmen_xid);
                        if constexpr (debugmode) log("%%Shared buffer successfuly created at 0x%%, %% bytes", prompt::x11, utf::to_hex(shm_buffer_ptr), shm_buffer_len);
                    }
                }
            }
            return shm_buffer_len > 0;
        }
        void reset_shared_buffer()
        {
            if (shm_buffer_ptr && shm_buffer_ptr != MAP_FAILED)
            {
                ::munmap(shm_buffer_ptr, shm_buffer_len);
                shm_buffer_len = {};
                shm_buffer_ptr = {};
            }
            if (shm_buffer_fd != os::invalid_fd)
            {
                ::close(shm_buffer_fd);
                shm_buffer_fd = os::invalid_fd;
            }
        }
        auto get_atoms()
        {
            auto get_atom_id = [&](qiew name, bool create)
            {
                sendrq<x11::req::intern_atom>({ .only_if_exists = !create }, name); // 0: Create if absent. 1: Don't create.
                auto reply = x11::req::intern_atom::reply{};
                if (x11connection->recv((char*)&reply, sizeof(reply)).size() == 32 && reply.type == 1)
                {
                    if constexpr (debugmode) log("%%Received atom for '%%'=0x%%", prompt::x11, name, reply.atom_id);
                    return reply.atom_id;
                }
                log("%%Failed to intern atom: %%", prompt::x11, name);
                return 0u;
            };
            // Window related.
            atom_motif_wm_hints            = get_atom_id("_MOTIF_WM_HINTS", true);
            atom_net_wm_state              = get_atom_id("_NET_WM_STATE", true);
            atom_net_wm_state_skip_taskbar = get_atom_id("_NET_WM_STATE_SKIP_TASKBAR", true);
            //atom_net_wm_window_type        = get_atom_id("_NET_WM_WINDOW_TYPE", true);
            //atom_net_wm_window_type_combo  = get_atom_id("_NET_WM_WINDOW_TYPE_COMBO", true);
            //atom_compton_shadow            = get_atom_id("_COMPTON_SHADOW", true); // Picom/Compton
            // Server related.
            atom_active_window      = get_atom_id("_NET_ACTIVE_WINDOW", faux);
            //atom_number_of_desktops = get_atom_id("_NET_NUMBER_OF_DESKTOPS", faux);
            //atom_current_desktop    = get_atom_id("_NET_CURRENT_DESKTOP", faux);
            //atom_workarea           = get_atom_id("_NET_WORKAREA", faux);
            return true;//atom_motif_wm_hints > 0;
        }
        auto get_error(x11::event::error& err)
        {
            auto err_str = text{};
            switch (err.error_code)
            {
                case  1: err_str = "Bad Request";        break;
                case  2: err_str = "Bad Value";          break;
                case  3: err_str = "Bad Window";         break;
                case  4: err_str = "Bad Pixmap";         break;
                case  5: err_str = "Bad Atom";           break;
                case  6: err_str = "Bad Cursor";         break;
                case  7: err_str = "Bad Font";           break;
                case  8: err_str = "Bad Match";          break;
                case  9: err_str = "Bad Drawable";       break;
                case 10: err_str = "Bad Access";         break;
                case 11: err_str = "Bad Alloc";          break;
                case 12: err_str = "Bad Color";          break;
                case 13: err_str = "Bad GC";             break;
                case 14: err_str = "Bad IDChoice";       break;
                case 15: err_str = "Bad Name";           break;
                case 16: err_str = "Bad Length";         break;
                case 17: err_str = "Bad Implementation"; break;
            }
            if (err.major_opcode == shm_major_opcode) // MIT-SHM related error.
            {
                err_str += " (MIT-SHM Extension Error)";
            }
            else if (err.major_opcode == xfixes_major_opcode) // XFIXES related error.
            {
                err_str += " (XFIXES Extension Error)";
            }
            return utf::fprint("%%Error: code=%%, seq=%%, bad_resource_id=0x%%, major=%%, minor=%% desc: %%", prompt::x11,
                            (ui32)err.error_code, (ui32)err.sequence, utf::to_hex(err.bad_value),
                            (ui32)err.major_opcode, (ui32)err.minor_opcode, err_str);
        }
        auto parse_error(x11::event::error& err)
        {
            log(get_error(err));
            auto lock = std::lock_guard{ mutex };
            if (reply_callbacks.size())
            if (reply_callbacks.front().sequence == err.sequence) // Pop broken request handler.
            {
                reply_callbacks.pop_front();
            }
        }
        auto listen_root_events() // Subscribe on root's property change (to track some desktop window has received focus).
        {
            sendrq<x11::req::change_window_attrs>({ .window_id = roots.front().s.root_window_id },
                x11::req::change_window_attrs::payload{ .event_mask = x11::event::mask::PropertyChange });
            return true;
        }
        auto get_root_window_prop(ui32 /*atom_id*/)
        {
            //todo
            return 0;
        }
    };
    #pragma pack(pop)

    auto get_cookie(view target_display_num)
    {
        struct x11cookie_t
        {
            text auth_name;
            text auth_data;
        };
        auto x11cookie = x11cookie_t{};
        auto auth_path = text{}; // Path to .Xauthority file.
        if (auto xauth_env = os::env::get("XAUTHORITY"); xauth_env.size())
        {
            auth_path = xauth_env;
        }
        else if (auto home_env = os::env::get("HOME"); home_env.size())
        {
            auth_path = home_env + "/.Xauthority";
        }
        if (auth_path.size())
        if (auto fs = std::ifstream{ auth_path, std::ios::binary }; fs.is_open())
        {
            auto read_ui16be = [&]
            {
                auto bytes = text(2, '\0');
                return fs.read(bytes.data(), bytes.size()) ? ((ui16)(byte)bytes[0] << 8) | (byte)bytes[1] : ui16{};
            };
            auto read_string = [&](ui16 length)
            {
                auto string = text(length, '\0');
                if (length > 0) fs.read(string.data(), length);
                return string;
            };
            while (fs.peek() != EOF)
            {
[[maybe_unused]]auto family   = read_ui16be(); // family = 256 (FamilyLocal).
                auto addr_len = read_ui16be();
[[maybe_unused]]auto addr_str = read_string(addr_len);
                auto disp_len = read_ui16be();
                auto disp_str = read_string(disp_len);
                auto name_len = read_ui16be();
                auto name_str = read_string(name_len);
                auto data_len = read_ui16be();
                auto data_str = read_string(data_len);
                if (!fs) break; // Unexpected errors.
                if constexpr (debugmode) log("XAuth entry: family=%%, disp='%%', proto='%%', data_size=%%", family, disp_str, name_str, data_len);
                if (name_str == "MIT-MAGIC-COOKIE-1" && (disp_str == target_display_num || disp_str.empty()))
                {
                    if constexpr (debugmode) log("Cookie found. Name: %%, Data size: %%", name_str, data_len);
                    x11cookie.auth_name = std::move(name_str);
                    x11cookie.auth_data = std::move(data_str);
                    break;
                }
            }
        }
        return x11cookie;
    }
    auto build_auth_packet(auto& cookie_data)
    {
        auto req = x11::session_t::auth{ .byte_order     = netxs::endian_LE ? 'l' : 'B',
                                         .major_version  = 11,
                                         .minor_version  = 0,
                                         .auth_proto_len = (ui16)cookie_data.auth_name.size(),
                                         .auth_data_len  = (ui16)cookie_data.auth_data.size() };
        auto auth_name_padded_len = (req.auth_proto_len + 3) & ~3; // Rounding up to a multiple of 4.
        auto auth_data_padded_len = (req.auth_data_len  + 3) & ~3; //
        auto packet = text(sizeof(req) + auth_name_padded_len + auth_data_padded_len, '\0');
        std::memcpy(packet.data(), &req, sizeof(req));
        std::memcpy(packet.data() + sizeof(req), cookie_data.auth_name.data(), cookie_data.auth_name.size());
        std::memcpy(packet.data() + sizeof(req) + auth_name_padded_len, cookie_data.auth_data.data(), cookie_data.auth_data.size());
        return packet;
    }
    auto parse_auth_reply(auto x11connection)
    {
        auto session_ptr = ptr::shared<x11::session_t>();
        auto& session = *session_ptr;
        auto reply = x11::session_t::auth::reply{};
        if (auto l1 = x11connection->recv((char*)&reply, sizeof(reply)); l1.size() == sizeof(reply))
        {
            auto remaining_bytes = (size_t)reply.additional_length * 4;
            auto buffer = text(remaining_bytes, '\0');
            if (reply.status == 0) // Failed.
            {
                log("%%Connection rejected: '%%'", prompt::x11, utf::debase<faux, faux>(x11connection->recv(buffer.data(), buffer.size())));
            }
            else if (reply.status != 1)
            {
                log("%%Unknown response status", prompt::x11);
            }
            else if (auto l3 = x11connection->recv(buffer.data(), buffer.size()); l3.size() != buffer.size())
            {
                log("%%Error reading response payload", prompt::x11);
            }
            else
            {
                auto q = qiew{ buffer };
                auto failed = faux;
                auto load = [&](auto& object)
                {
                    auto ptr = object.data();
                    auto len = object.size();
                    auto len_padded = (size_t)((len + 3) & ~3);
                    if (!failed && q.size() >= len_padded)
                    {
                        std::memcpy(ptr, q.data(), len);
                        q.remove_prefix(len_padded);
                    }
                    else failed = true;
                };
                load(session);
                session.vendor_str.resize(session.s.vendor_length);
                load(session.vendor_str);
                session.pixmap_formats.resize(session.s.number_of_formats);
                for (auto& pixmap_format : session.pixmap_formats)
                {
                    load(pixmap_format);
                }
                session.roots.resize(session.s.number_of_screens);
                for (auto& screen : session.roots)
                {
                    load(screen);
                    screen.list_of_depths.resize(screen.s.number_of_depths);
                    for (auto& depth : screen.list_of_depths)
                    {
                        load(depth);
                        depth.list_of_visual_types.resize(depth.s.num_of_visual_types);
                        for (auto& visual_type : depth.list_of_visual_types)
                        {
                            load(visual_type);
                        }
                    }
                }
                if (failed) session_ptr.reset();
                else        session_ptr->x11connection = x11connection;
            }
        }
        else
        {
            log("%%Error reading connection reply", prompt::x11);
        }
        return session_ptr;
    }
    static auto session_ptr = sptr<x11::session_t>{}; // x11: Active X11 session.
    auto connect()
    {
        if (auto display_env = os::env::get("DISPLAY"); display_env.size())
        if (auto colon_start = display_env.find(':'); colon_start != text::npos)
        if (auto display_num = utf::to_int(display_env.substr(colon_start + 1)))
        if (auto x11unixpath = utf::concat("/tmp/.X11-unix/X", display_num.value()); os::fs::exists(x11unixpath))
        if (auto socket_link = os::ipc::socket::connect(x11unixpath))
        {
            auto display_str = std::to_string(display_num.value());
            auto cookie_data = x11::get_cookie(display_str);
            auto auth_packet = x11::build_auth_packet(cookie_data);
            socket_link->send(auth_packet);
            if (auto session_ptr = x11::parse_auth_reply(socket_link))
            {
                auto& session = *session_ptr;
                if (session.detect_argb_32bit())
                if (session.detect_extension<x11::req::shm::query_version>("MIT-SHM", session.shm_major_opcode, session.shm_completion_event, 1, 2)) // MIT-SHM ver >= 1.2
                if (session.detect_extension<x11::req::xfixes::query_version>("XFIXES", session.xfixes_major_opcode, session.xfixes_first_event, 2, 0)) // XFIXES ver >= 2.0
                if (session.get_atoms())
                if (session.listen_root_events())
                {
                    if constexpr (debugmode) log(session.str());
                    auto& x11screen = session.roots.front().s;
                    auto max_grid_size = x11screen.width_in_pixels * x11screen.height_in_pixels;
                    auto required_buffer_size = 2 * 3 * max_grid_size * sizeof(argb); // 2: Double buffer, 3: master+blinks+header/footer/tooltip.
                    if (session.resize_shared_buffer(required_buffer_size))
                    {
                        x11::session_ptr = session_ptr;
                    }
                }
            }
        }
        return !!x11::session_ptr;
    }
}