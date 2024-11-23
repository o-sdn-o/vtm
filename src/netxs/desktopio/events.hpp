// Copyright (c) Dmitry Sapozhnikov
// Licensed under the MIT license.

#pragma once

#include "geometry.hpp"

#include <vector>
#include <mutex>
#include <map>
#include <list>
#include <functional>
#include <optional>
#include <thread>
#include <condition_variable>

namespace netxs::events
{
    struct tier
    {
        static constexpr auto counter = __COUNTER__ + 1;
        static constexpr auto general = __COUNTER__ - counter; // events: Run forwrad handlers for all objects. Preserve subscription order.
        static constexpr auto release = __COUNTER__ - counter; // events: Run forwrad handlers with fixed param. Preserve subscription order.
        static constexpr auto preview = __COUNTER__ - counter; // events: Run reverse handlers with fixed a param intended to change. Preserve subscription order.
        static constexpr auto request = __COUNTER__ - counter; // events: Run forwrad a handler that provides the current value of the param. To avoid being overridden, the handler should be the only one. Preserve subscription order.
        static constexpr auto anycast = __COUNTER__ - counter; // events: Run reverse handlers along the entire visual tree. Preserve subscription order.
    };

    /*************************************************************************************************
    toplevel = 0

    32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1
    0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0
    =  =  =  =

    level = toplevel + 4 = 4
    msg = 98

    32 31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1
    0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  1  0  0  1  0
    =  =  =  =
    **************************************************************************************************/

    static constexpr auto block = 4;

    // events: Return event level by its ID. Find the log base 2**block.
    static constexpr inline auto level(hint event)
    {
        if (event == 0) return 0;
        auto level = 1;
        while (event >>= block) { ++level; }
        return level;
    }
    // events: Return event level mask by its ID. Find the log base 2**block.
    static constexpr inline hint level_mask(hint event, si32 level = 0)
    {
        while (event >>= block) { level += block; }
        return (1 << level) - 1;
        //constexpr auto c = __COUNTER__ + 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //if (!(event >>= block)) return (1 << (__COUNTER__ - c) * block) - 1;
        //return std::numeric_limits<hint>::max();
    }
    // events: Return true if the event belongs to the branch.
    static constexpr inline auto subevent(hint event, hint branch)
    {
        return (event & level_mask(branch, block)) == branch;
    }
    // events: Return event index inside the group.
    static constexpr inline auto subindex(hint event)
    {
        auto offset = level(event) * block;
        auto number = (event >> (offset - block)) - 1;
        return number;
    }
    // events: Return event id by group + index.
    static constexpr inline auto makeid(hint group, hint index)
    {
        auto offset = level(group) * block;
        auto entity = group | ((index + 1) <<  offset);
        return entity;
    }
    template<hint Event>             constexpr auto offset = level(Event) * block;                         // events: Item/msg bit shift.
    template<hint Event>             constexpr auto parent = Event & ((1 << (offset<Event> - block)) - 1); // events: Event group ID.
    template<hint Event>             constexpr auto number = (Event >> (offset<Event> - block)) - 1;       // events: Item index inside the group by its ID.
    template<hint Group, auto Index> constexpr auto entity = Group | ((Index + 1) <<  offset<Group>);      // events: Event ID of the specified item inside the group.

    template<hint Group, auto... Index>
    constexpr auto _instantiate(std::index_sequence<Index...>)
    {
        return std::array<hint, sizeof...(Index)>{ entity<Group, Index>... };
    }
    template<hint Group, auto Count> constexpr auto subset = _instantiate<Group>(std::make_index_sequence<Count>{});

    struct bell;
    using ftor = std::function<bool(sptr<bell>)>;

    struct handler
    {
        virtual ~handler() { }
    };
    struct hook : sptr<handler>
    {
        using sptr<handler>::sptr;
        auto& operator - (si32) { return *this; }
    };
    struct reactor
    {
        template<class F>
        using hndl = std::function<void(F&)>;
        using list = std::list<wptr<handler>>;
        using vect = std::vector<wptr<handler>>;

        template<class F>
        struct wrapper : handler
        {
            hndl<F> proc;
            wrapper(hndl<F>&& proc)
                : proc{ proc }
            { }
        };

        enum class branch
        {
            fullstop,
            nothandled,
            proceed,
        };

        // Forward execution order: Execute concrete event  first. Forward means from particular to general: 1. event_group::item, 2. event_group::any
        // Reverse execution order: Execute global   events first. Reverse means from general to particular: 1. event_group::any , 2. event_group::item
        bool                 order; // reactor: Execution order. True means Forward.
        std::map<hint, list> stock; // reactor: Handlers repository.
        std::vector<hint>    queue; // reactor: Event queue.
        vect                 qcopy; // reactor: Copy of the current pretenders to exec on current event.
        branch               alive; // reactor: Current exec branch interruptor.
        bool                 handled{}; // reactor: Last notify operation result.

        void cleanup(ui64& ref_count, ui64& del_count)
        {
            auto lref = ui64{};
            auto ldel = ui64{};
            for (auto& [event, subs] : stock)
            {
                auto refs = subs.size();
                subs.remove_if([](auto&& a){ return a.expired(); });
                auto size = subs.size();
                lref += size;
                ldel += refs - size;
            }
            ref_count += lref;
            del_count += ldel;
        }
        void merge(reactor const& r)
        {
            for (auto& [event, src_subs] : r.stock)
            {
                auto& dst_subs = stock[event];
                dst_subs.insert( dst_subs.end(), src_subs.begin(), src_subs.end() );
            }
        }
        template<class F>
        hook subscribe(hint event, hndl<F> proc)
        {
            auto proc_ptr = std::make_shared<wrapper<F>>(std::move(proc));
            stock[event].push_back(proc_ptr);
            return proc_ptr;
        }
        inline void _refreshandcopy(list& target)
        {
            target.remove_if([&](auto&& a){ return a.expired() ? true : (qcopy.emplace_back(a), faux); });
        }
        // reactor: Calling delegates. Returns the number of active ones.
        template<class F>
        void notify(hint event, F& param)
        {
            alive = branch::proceed;
            queue.push_back(event);
            auto head = qcopy.size();
            if (order)
            {
                auto itermask = events::level_mask(event);
                auto subgroup = event;
                _refreshandcopy(stock[subgroup]);
                while (itermask > 1 << events::block) // Skip root event block.
                {
                    subgroup = event & itermask;
                    itermask >>= events::block;
                    _refreshandcopy(stock[subgroup]);
                }
            }
            else
            {
                static constexpr auto mask = hint{ (1 << events::block) - 1 };
                auto itermask = mask; // Skip root event block.
                auto subgroup = hint{};
                do
                {
                    itermask = itermask << events::block | mask;
                    subgroup = event & itermask;
                    _refreshandcopy(stock[subgroup]);
                }
                while (subgroup != event);
            }
            auto tail = qcopy.size();
            auto size = tail - head;
            if (size)
            {
                auto iter = head;
                do
                {
                    if (auto proc_ptr = qcopy[iter].lock()) // qcopy can be reallocated.
                    {
                        if (auto compatible = static_cast<wrapper<F>*>(proc_ptr.get()))
                        {
                            compatible->proc(param);
                        }
                    }
                }
                while (alive == branch::proceed && ++iter != tail);
                qcopy.resize(head);
            }
            queue.pop_back();
            handled = alive != branch::nothandled && size;
        }
        // reactor: Interrupt current invocation branch.
        void stop()
        {
            alive = branch::fullstop;
        }
        // reactor: Skip current invocation branch.
        void skip()
        {
            alive = branch::nothandled;
        }
    };

    struct auth
    {
        id_t                       newid;
        wptr<bell>                 empty;
        std::recursive_mutex       mutex;
        std::map<id_t, wptr<bell>> store;
        generics::jobs<wptr<bell>> agent;
        reactor                    general{ true };

        // auth: .
        auto sync()
        {
            return std::lock_guard{ mutex };
        }
        // auth: .
        auto try_sync()
        {
            struct try_sync_t : std::unique_lock<std::recursive_mutex>
            {
                using unique_lock::unique_lock;
                operator bool () { return unique_lock::owns_lock(); }
            };
            return try_sync_t{ mutex, std::try_to_lock };
        }
        // auth: .
        auto unique_lock()
        {
            return std::unique_lock{ mutex };
        }
        // auth: Return sptr of the object by its id.
        template<class T = bell>
        auto getref(id_t id)
        {
            auto lock = sync();
            if (auto item_ptr = netxs::get_or(store, id, empty).lock())
            if (auto real_ptr = std::dynamic_pointer_cast<T>(item_ptr))
            {
                return real_ptr;
            }
            return sptr<T>{};
        }
        // auth: Create a new object of the specified subtype and return its sptr.
        template<class T, class ...Args>
        auto create(Args&&... args) -> sptr<T>
        {
            auto lock = sync();
            // Use new/delete to be able lock before destruction.
            auto inst = std::shared_ptr<T>(new T(std::forward<Args>(args)...), [](T* inst)
                                                                               {
                                                                                    auto& indexer = inst->indexer;
                                                                                    auto lock = indexer.sync(); // Sync with all dtors.
                                                                                    auto id = inst->id;
                                                                                    delete inst;
                                                                                    indexer.store.erase(id);
                                                                               });
            store[inst->id] = inst;
            return inst;
        }
        // auth: Return next available id.
        auto new_id()
        {
            while (netxs::on_key(store, ++newid))
            { }
            return newid;
        }
        // auth: .
        template<bool Sync = true, class T>
        void enqueue(wptr<bell> object_wptr, T&& proc)
        {
            agent.add(object_wptr, [&, proc](auto& object_wptr) mutable
            {
                auto lock = unique_lock();
                if (auto object_ptr = object_wptr.lock())
                {
                    if constexpr (!Sync) lock.unlock();
                    proc(*object_ptr);
                    if constexpr (!Sync) lock.lock();
                }
            });
        }
        // auth: .
        void dequeue()
        {
            agent.stop();
        }
        // auth: .
        template<class T, class P>
        auto synced(T& object_sptr, P proc)
        {
            using buff = generics::buff<text>;
            return [&, proc, buffer = buff{}](auto utf8) mutable
            {
                auto lock = buffer.freeze();
                lock.block += utf8;
                if (!lock.await)
                {
                    if (auto sync = try_sync())
                    {
                        proc(view{ lock.block });
                        lock.block.clear();
                    }
                    else
                    {
                        lock.await = true;
                        enqueue(object_sptr, [&](auto& /*boss*/)
                        {
                            auto lock = buffer.freeze();
                            lock.await = faux;
                            proc(view{ lock.block });
                            lock.block.clear();
                        });
                    }
                }
            };
        }
    };

    class subs
    {
        std::vector<hook> tokens;

    public:
        auto& operator - (si32)    { return *this;                                                           }
        operator bool () const     { return tokens.size();                                                   }
        void  admit(hook&& t)      {        tokens.push_back(std::forward<hook>(t));                         }
        hook& extra()              { return tokens.emplace_back();                                           }
        auto  count() const        { return tokens.size();                                                   }
        void  clear()              {        tokens.clear();                                                  }
        void  reset()              {        tokens.clear();                                                  }
        void  merge(subs const& m) {        tokens.insert( tokens.end(), m.tokens.begin(), m.tokens.end() ); }
    };

    template<class Parent_t, class Object_t, auto Event_id>
    struct type_clue
    {
        using type = Object_t;
        using base = Parent_t;
        static constexpr auto id = Event_id;
        template<class ...Args> constexpr type_clue(Args&&...) { }
        template<class ...Args> static constexpr auto param(Args&&... args) { return type{ std::forward<Args>(args)... }; }
                                static constexpr auto param(type&&    arg ) { return std::move(arg);                      }
        template<auto N>        static constexpr auto group()               { return events::subset<id, N>;               }
                                static constexpr auto index()               { return events::number<id>;                  }
    };

    #define ARGsEVAL_XS(...) __VA_ARGS__
    #define ARG_EVAL_XS(...) ARGsEVAL_XS(__VA_ARGS__)
    #define GET_END1_XS(a, b, c, d, e, last, ...) last
    #define GET_END2_XS(a, b, c, d,    last, ...) last

    #define LISTEN_S(level, event, param              ) bell::submit(level, event )           = [&]                    ([[maybe_unused]] typename decltype( event )::type& param)
    #define LISTEN_T(level, event, param, token       ) bell::submit(level, event, token -0 ) = [&]                    ([[maybe_unused]] typename decltype( event )::type& param)
    #define LISTEN_V(level, event, param, token, byval) bell::submit(level, event, token -0 ) = [&, ARG_EVAL_XS byval ]([[maybe_unused]] typename decltype( event )::type& param) mutable
    #define LISTEN_X(...) ARG_EVAL_XS(GET_END1_XS(__VA_ARGS__, LISTEN_V, LISTEN_T, LISTEN_S))
    #define LISTEN(...) LISTEN_X(__VA_ARGS__)(__VA_ARGS__)

    #define EVENTPACK( name, base ) using _group_type = name; \
                                    static constexpr auto _counter_base = __COUNTER__; \
                                    public: static constexpr auto any = netxs::events::type_clue<_group_type, decltype(base)::type, decltype(base)::id>
    #define  EVENT_XS( name, type ) }; static constexpr auto name = netxs::events::type_clue<_group_type, type, decltype(any)::id | ((__COUNTER__ - _counter_base) << netxs::events::offset<decltype(any)::id>)>{ 777
    #define  GROUP_XS( name, type ) EVENT_XS( _##name, type )
    #define SUBSET_XS( name )       }; class name { EVENTPACK( name, _##name )
    #define  INDEX_XS(  ... )       }; template<auto N> static constexpr \
                                    auto _ = std::get<N>( std::tuple{ __VA_ARGS__ } ); \
                                    private: static constexpr auto _dummy = { 777

    struct ref_count_t
    {
        ui64 obj_count{};
        ui64 ref_count{};
        ui64 del_count{};
    };

    //todo unify seeding
    namespace userland
    {
        struct root
        {
            static constexpr auto root_event = type_clue<root, si32, 0>{};
            EVENTPACK( root, root_event )
            {
                EVENT_XS( dtor     , const id_t ),
                EVENT_XS( cascade  , ftor ),
                EVENT_XS( base     , root ),
                EVENT_XS( hids     , root ),
                EVENT_XS( scripting, root ),
                EVENT_XS( custom   , root ),
                EVENT_XS( cleanup  , ref_count_t ), // Garbage collection.
            };
        };
    }

    // events: Event x-mitter.
    struct bell
    {
        static constexpr auto noid = std::numeric_limits<id_t>::max();

        auth&        indexer;
        reactor&     general;
        const id_t   id;
        subs         tracker;

    private:
        reactor release{ true };
        reactor preview{ faux };
        reactor request{ true };
        reactor anycast{ faux };
        reactor* reactors[5] = { &general, &release, &preview, &request, &anycast };

        template<class Event>
        struct submit_helper
        {
            bell& owner;
            si32  level;
            submit_helper(si32 level, bell& owner)
                : owner{ owner },
                  level{ level }
            { }
            template<class F>
            void operator = (F h)
            {
                owner.submit(level, Event{}, h);
            }
        };
        template<class Event>
        struct submit_helper_token
        {
            bell& owner;
            hook& token;
            si32  level;
            submit_helper_token(si32 level, bell& owner, hook& token)
                : owner{ owner },
                  token{ token },
                  level{ level }
            { }
            template<class F>
            void operator = (F h)
            {
                owner.submit(level, Event{}, token, h);
            }
        };

    public:
        template<class Event> auto submit(si32 Tier, Event)               { return submit_helper      <Event>(Tier, *this);                 }
        template<class Event> auto submit(si32 Tier, Event, si32)         { return submit_helper      <Event>(Tier, *this);                 }
        template<class Event> auto submit(si32 Tier, Event, hook& token)  { return submit_helper_token<Event>(Tier, *this, token);          }
        template<class Event> auto submit(si32 Tier, Event, subs& tokens) { return submit_helper_token<Event>(Tier, *this, tokens.extra()); }
        template<class Event>
        void submit(si32 Tier, Event, std::function<void(typename Event::type&)> handler)
        {
            auto lock = indexer.sync();
            tracker.admit(reactors[Tier]->subscribe(Event::id, handler));
        }
        template<class Event>
        void submit(si32 Tier, Event, hook& token, std::function<void(typename Event::type&)> handler)
        {
            auto lock = indexer.sync();
            token = reactors[Tier]->subscribe(Event::id, handler);
        }
        auto accomplished(si32 Tier)
        {
            return reactors[Tier]->handled;
        }
        auto signal(si32 Tier, hint event, auto& param)
        {
            auto lock = indexer.sync();
            if (Tier == tier::anycast)
            {
                auto root = gettop();
                auto proc = ftor{ [&](auto boss_ptr)
                {
                    boss_ptr->anycast.notify(event, param);
                    return true;
                }};
                root->release.notify(userland::root::cascade.id, proc);
            }
            else reactors[Tier]->notify(event, param);
        }
        // bell: Fire an event.
        // Usage example:
        //          bell::signal(tier::preview, e2::form::prop::ui::header, txt);
        template<class Event>
        auto signal(si32 Tier, Event, Event::type&& param = {})
        {
            signal(Tier, Event::id, param);
            return param;
        }
        template<class Event>
        void signal(si32 Tier, Event, Event::type& param)
        {
            signal(Tier, Event::id, param);
        }
        template<class Event>
        void signal(si32 Tier, Event, Event::type const& param)
        {
            signal(Tier, Event::id, param);
        }
        // bell: Return initial event of the current event execution branch.
        auto protos(si32 Tier)
        {
            return reactors[Tier]->queue.empty() ? hint{} : reactors[Tier]->queue.back();
        }
        template<class Event>
        auto protos(si32 Tier, Event)
        {
            return bell::protos(Tier) == Event::id;
        }
        auto& router(si32 Tier)
        {
            return *reactors[Tier];
        }
        void expire(si32 Tier, bool skip = faux)
        {
            skip ? reactors[Tier]->skip()
                 : reactors[Tier]->stop();
        }
        // bell: Create a new object of the specified subtype and return its sptr.
        template<class T, class ...Args>
        auto create(Args&&... args) -> sptr<T>
        {
            return indexer.create<T>(indexer, std::forward<Args>(args)...);
        }
        // bell: .
        void dequeue()
        {
            indexer.agent.stop();
        }
        // bell: .
        template<bool Sync = true, class ...Args>
        void enqueue(Args&&... args)
        {
            indexer.enqueue<Sync>(std::forward<Args>(args)...);
        }
        // bell: .
        auto sync()
        {
            return indexer.sync();
        }
        // bell: .
        auto try_sync()
        {
            return indexer.try_sync();
        }
        // bell: .
        auto unique_lock()
        {
            return indexer.unique_lock();
        }
        // bell: Return sptr of the object by its id.
        template<class T = bell>
        auto getref(id_t id)
        {
            return indexer.getref<T>(id);
        }

        bell(auth& indexer)
            : indexer{ indexer },
              general{ indexer.general },
              id{ indexer.new_id() }
        {
            LISTEN(tier::general, userland::root::cleanup, counter)
            {
                counter.obj_count++;
                preview.cleanup(counter.ref_count, counter.del_count);
                request.cleanup(counter.ref_count, counter.del_count);
                release.cleanup(counter.ref_count, counter.del_count);
                anycast.cleanup(counter.ref_count, counter.del_count);
            };
        }
       ~bell()
        {
            signal(tier::release, userland::root::dtor, id);
        }
        virtual sptr<bell> gettop() { return sptr<bell>(this, noop{}); } // bell: Recursively find the root of the visual tree.
    };
}
namespace netxs
{
    using netxs::events::bell;
    using netxs::events::subs;
    using netxs::events::tier;
    using netxs::events::hook;
}