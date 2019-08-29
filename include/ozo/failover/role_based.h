#pragma once

#include <ozo/failover/strategy.h>
#include <ozo/failover/retry.h>
#include <ozo/core/options.h>
#include <ozo/ext/std/optional.h>
#include <ozo/ext/std/nullopt_t.h>

#include <boost/hana/tuple.hpp>
#include <boost/hana/empty.hpp>
#include <boost/hana/size.hpp>
#include <boost/hana/minus.hpp>
#include <boost/hana/not_equal.hpp>
#include <boost/hana/greater.hpp>

/**
 * @defgroup group-failover-role_based Role-Based Execution & Fallback
 * @ingroup group-failover
 * @brief Failover operation by role-based fallback
 *
 * This type of failover strategy is dedicated to a DBM cluster with several hosts are
 * playing different roles like Master, Replica, and so on.
 *
 * For example, you have a high-load and high-availability system which should provide
 * newest data from a master host under normal conditions, but it is ok to have outdated
 * data from a replica host if the master is down or under some overload conditions.
 *
 * For that kind of systems, the Role-Based Execution & Fallback strategy is.
 *
 * The base entities are Roles which are represented by `ozo::failover::role` template
 * specializations. Every role can recover several error conditions which should be defined via
 * `ozo::failover::can_recover` customization. So if a new recover condition set is needed - a new
 * role should be defined. This mechanism of static recovery conditions definition is a trade-off
 * between framework complexity and usability.
 *
 * There is a strategy which uses the roles. It is implemented via `ozo::failover::role_based_strategy`
 * class and can be instantiated via `ozo::failover::role_based` function.
 *
 * The strategy may work only with a special type of `ConnectionProvider` which allows being
 * bound to a specific role. Such provider is modelled by `ozo::failover::role_based_connection_provider`.
 * It has an additional method `ozo::failover::role_based_connection_provider::rebind_role`
 * which bind the provider to a specific role. Such a mechanism allows to switch between roles
 * and get connections to their hosts during the execution of the strategy.
 *
 * The example of the strategy use can be found in `examples/role_based_request.cpp` .
 *
 * @include examples/role_based_request.cpp
 */

namespace ozo::failover {

/**
 * @brief Options for role-based failover
 *
 * These options can be used with `ozo::failover::role_based_strategy`.
 * @ingroup group-failover-role_based
 */
struct role_based_options {
    class on_fallback_tag;
    class close_connection_tag;
    class roles_tag;

    constexpr static option<on_fallback_tag> on_fallback{}; //!< Handler for fallback event with signature `void(error_code, Connection, Fallback)`, may be useful for logging.
    constexpr static option<close_connection_tag> close_connection{}; //!< Close connection policy on retry, possible values `true`(default), `false`.
    constexpr static option<roles_tag> roles{}; //!< Strategy roles sequence.
};

template <typename Tag>
using role = ozo::option<Tag>;

template <typename Role>
struct can_recover_impl {
    static constexpr std::false_type apply(const Role&, const error_code&) {
        static_assert(std::is_void_v<Role>, "no can_recover_impl specified for a given role");
        return {};
    }
};

/**
 * General purpose master (read/write) host role.
 *
 * This role can recover these error conditions
 * - `ozo::errc::connection_error`,
 * - `ozo::errc::type_mismatch`,
 * - `ozo::errc::protocol_error`,
 * - `ozo::errc::database_readonly`
 *
 * @note In most cases, the user should define custom roles to specify conditions to recover.
 *
 * @sa `ozo::failover::can_recover()`
 * @ingroup group-failover-role_based
 */
constexpr role<class master_tag> master;

template <>
struct can_recover_impl<std::decay_t<decltype(master)>> {
    static constexpr auto value = hana::make_tuple(
        errc::connection_error,
        errc::type_mismatch,
        errc::protocol_error,
        errc::database_readonly
    );

    static constexpr auto apply(const std::decay_t<decltype(master)>&, const error_code& ec) {
        return errc::match_code(value, ec);
    }
};

/**
 * General purpose replica (read-only) host role.
 *
 * This role can recover these error conditions
 * - `ozo::errc::connection_error`,
 * - `ozo::errc::type_mismatch`,
 * - `ozo::errc::protocol_error`
 *
 * @note In most cases, the user should define custom roles to specify conditions to recover.
 *
 * @sa `ozo::failover::can_recover()`
 * @ingroup group-failover-role_based
 */
constexpr role<class replica_tag> replica;

template <>
struct can_recover_impl<std::decay_t<decltype(replica)>> {
    static constexpr auto value = hana::make_tuple(
        errc::connection_error,
        errc::type_mismatch,
        errc::protocol_error
    );

    static constexpr auto apply(const std::decay_t<decltype(replica)>&, const error_code& ec) {
        return errc::match_code(value, ec);
    }
};

/**
 * @brief Determines if an error can be recovered by executing an operation on host with a given role
 *
 * @param role --- role which should be checked
 * @param ec --- error code from the previous try of an operation execution
 * @return true --- a role can recover given error code and should be tried to recover an operation
 * @return false --- a role can not recover given error code and should not be tried to recover an operation
 *
 * ###Customization Point
 *
 * This function should be customized for a new role via `ozo::failover::can_recover_impl` template
 * specialization.
 *
 * E.g. for `ozo::failover::replica` role the specialization may look like this:
@code
template <>
struct can_recover_impl<std::decay_t<decltype(replica)>> {
    static constexpr auto apply(const std::decay_t<decltype(replica)>&, const error_code& ec) {
        return ec == errc::connection_error
            || ec == errc::type_mismatch
            || ec = errc::protocol_error;
    }
};
@endcode
 *
 * Such a customization provides an ability to use roles objects which contains some additional
 * compile-time or run-time information. E.g.:
 *
@code
namespace demo {

struct master_with_custom_error_conditions {
    std::vector<ozo::errc::code> conditions;
};

constexpr master_with_custom_error_conditions master;

} // namespace demo

namespace ozo::failover {

template <>
struct can_recover_impl<std::decay_t<decltype(demo::master)>> {
    static constexpr auto apply(const std::decay_t<decltype(demo::master)>& r, const error_code& ec) {
        return std::find_if(r.conditions.begin(), r.conditions.end(),
            [&](const auto& errc){ retrun ec == errc;}) != r.conditions.end();
    }
};

} // namespace ozo::failover
@endcode
 *
 * @ingroup group-failover-role_based
 */
template <typename Role>
constexpr auto can_recover(const Role& role, const error_code& ec) {
    return can_recover_impl<Role>::apply(role, ec);
}

namespace detail {

template <typename Source, typename Role, typename = hana::when<true>>
struct connection_source_supports_role : std::false_type {};

template <typename Source, typename Role>
struct connection_source_supports_role <
    Source, Role,
    hana::when_valid<decltype(std::declval<Source>().rebind_role(std::declval<const Role&>()))>
> : std::true_type {};

} // namespace detail

/**
 * #ConnectionProvider implementation for the role-based failover strategy.
 *
 * This is the role-based implementation of the #ConnectionProvider concept. It binds
 * `io_context` and role to a #ConnectionSource implementation object. It requires from
 * the underlying #ConnectionSource an ability to be bound to a certain role via `bind_role(role)`
 * method.
 *
 * @tparam Source --- #ConnectionSource implementation
 * @ingroup group-failover-role_based
 */
template <typename Source>
class role_based_connection_provider {
    Source source_;
    io_context& io_;
public:

    static_assert(ozo::ConnectionSource<Source>, "Source should model a ConnectionSource concept");
    /**
     * Current source type.
     * @note Source type depends on role type and maybe different for a different role.
     */
    using source_type = std::decay_t<Source>;

    /**
     * #Connection implementation type according to #ConnectionProvider requirements.
     * Specifies the #Connection implementation type which can be obtained from this provider.
     *
     * @note #Connection type depends on role type via `source_type` and maybe different for a different role.
     */
    using connection_type = typename connection_source_traits<source_type>::connection_type;

    /**
     * Indicates if a given role is supported by the source. The `role` is supported if
     * `source.rebind_role(role)` can be invoked successful.
     *
     * @param OtherRole --- role to examine.
     */
    template <typename OtherRole>
    static constexpr auto is_supported(const OtherRole&) {
        return typename detail::connection_source_supports_role<source_type, OtherRole>::type{};
    }

    /**
     * Construct a new `role_based_connection_provider` object
     *
     * @param source --- #ConnectionSource implementation
     * @param io --- `io_context` for asynchronous IO
     */
    template <typename T>
    role_based_connection_provider(T&& source, io_context& io)
     : source_(std::forward<T>(source)), io_(io) {
    }

    /**
     * @brief Rebind the #ConnectionProvider to an other role
     *
     * @param r --- other role to rebind to
     * @return new #ConnectionProvider object
     */
    template <typename OtherRole>
    constexpr decltype(auto) rebind_role(OtherRole r) const & {
        static_assert(is_supported(r), "role is not supported by a connection provider");
        using rebind_type = role_based_connection_provider<decltype(source_.rebind_role(r))>;
        return rebind_type{unwrap(source_).rebind_role(r), io_};
    }

    /**
     * @brief Rebind the #ConnectionProvider to an other role
     *
     * @param r --- other role to rebind to
     * @return new #ConnectionProvider object
     */
    template <typename OtherRole>
    constexpr decltype(auto) rebind_role(OtherRole r) & {
        static_assert(is_supported(r), "role is not supported by a connection provider");
        using rebind_type = role_based_connection_provider<decltype(source_.rebind_role(r))>;
        return rebind_type{unwrap(source_).rebind_role(r), io_};
    }

    /**
     * @brief Rebind the #ConnectionProvider to an other role
     *
     * @param r --- other role to rebind to
     * @return new #ConnectionProvider object
     */
    template <typename OtherRole>
    constexpr decltype(auto) rebind_role(OtherRole r) && {
        static_assert(is_supported(r), "role is not supported by a connection provider");
        using rebind_type = role_based_connection_provider<decltype(std::move(source_).rebind_role(r))>;
        return rebind_type{unwrap(std::forward<Source>(source_)).rebind_role(r), io_};
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) const & {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        unwrap(source_)(io_, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) & {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        unwrap(source_)(io_, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    void async_get_connection(TimeConstraint t, Handler&& h) && {
        static_assert(ozo::TimeConstraint<TimeConstraint>, "should model TimeConstraint concept");
        unwrap(std::forward<Source>(source_))(io_, std::move(t), std::forward<Handler>(h));
    }
};

template <typename T>
role_based_connection_provider(T&& source, io_context& io) -> role_based_connection_provider<T>;

/**
 * Default role-based connection source implementation
 *
 * This connection source allows defining connection sources for respective roles.
 *
 * @sa `ozo::failover::make_role_based_connection_source()`
 * @ingroup group-failover-role_based
 */
template <typename SourcesMap, typename Role>
struct role_based_connection_source {
    static_assert(decltype(hana::is_a<hana::map_tag>(std::declval<SourcesMap>()))::value, "SourcesMap should be a boost::hana::map");
    static_assert(decltype(hana::size(std::declval<const SourcesMap&>())!=hana::size_c<0>)::value, "sources should not be empty");

    SourcesMap sources_;
    Role role_;

    using source_type = std::decay_t<decltype(sources_[role_])>;
    /**
     * #Connection implementation type according to #ConnectionSource requirements.
     * Specifies the #Connection implementation type which can be obtained from this source.
     */
    using connection_type = typename connection_source_traits<source_type>::connection_type;

    template <typename OtherRole>
    static constexpr bool is_supported() {
        return decltype(hana::contains(std::declval<SourcesMap>(), std::declval<OtherRole>()))::value;
    }

    role_based_connection_source(SourcesMap sources, Role role)
    : sources_(std::move(sources)), role_(std::move(role)) {
        static_assert(is_supported<Role>(), "no default role found in sources");
    }

    template <typename OtherRole>
    constexpr auto rebind_role(OtherRole r) const & ->
            Require<is_supported<OtherRole>(), role_based_connection_source<SourcesMap, OtherRole>> {
        return {sources_, r};
    }

    template <typename OtherRole>
    constexpr auto rebind_role(OtherRole r) && ->
            Require<is_supported<OtherRole>(), role_based_connection_source<SourcesMap, OtherRole>> {
        return {std::move(sources_), r};
    }

    template <typename TimeConstraint, typename Handler>
    constexpr void operator() (io_context& io, TimeConstraint t, Handler&& h) const & {
        sources_[role_](io, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    constexpr void operator() (io_context& io, TimeConstraint t, Handler&& h) & {
        sources_[role_](io, std::move(t), std::forward<Handler>(h));
    }

    template <typename TimeConstraint, typename Handler>
    constexpr void operator() (io_context& io, TimeConstraint t, Handler&& h) && {
        std::move(sources_[role_])(io, std::move(t), std::forward<Handler>(h));
    }

    constexpr auto operator[] (io_context& io) const & {
        return role_based_connection_provider(*this, io);
    }

    constexpr auto operator[] (io_context& io) & {
        return role_based_connection_provider(*this, io);
    }

    constexpr auto operator[] (io_context& io) && {
        return role_based_connection_provider(std::move(*this), io);
    }
};

/**
 * Creates connection source which dispatches given connection sources for respective roles.
 *
 * ###Example
 *
 * Specify different `ozo::connection_info` objects for different roles.
 *
@code
#include <ozo/failover/role_based.h>

//...

auto conn_info = ozo::failover::make_role_based_connection_source(
    ozo::failover::master=ozo::connection_info(cfg.master_connstr),
    ozo::failover::replica=ozo::connection_info(cfg.replica_connstr)
);
@endcode
 * @ingroup group-failover-role_based
 */
template <typename Key, typename Value, typename ...Pairs>
constexpr decltype(auto) make_role_based_connection_source(hana::pair<Key, Value> p1, Pairs&& ...ps) {
    auto default_role = hana::first(p1);
    auto map = hana::make_map(std::move(p1), std::forward<Pairs>(ps)...);
    return role_based_connection_source{std::move(map), std::move(default_role)};
}

template <typename Options, typename Context, typename RoleIdx = decltype(hana::size_c<0>)>
class role_based_try {
    Context ctx_;
    Options options_;
    static constexpr RoleIdx idx_{};
    using opt = role_based_options;

public:
    /**
     * @brief Construct a new basic try object.
     *
     * @param options --- map of `ozo::failover::role_based_options`.
     * @param ctx --- operation context, compartible with `ozo::failover::basic_context`
     */
    constexpr role_based_try(Options options, Context ctx)
    : ctx_(std::move(ctx)), options_(std::move(options)) {
        static_assert(decltype(hana::is_a<hana::map_tag>(options))::value, "Options should be boost::hana::map");
    }

    constexpr role_based_try(const role_based_try&) = delete;
    constexpr role_based_try(role_based_try&&) = default;
    constexpr role_based_try& operator = (const role_based_try&) = delete;
    constexpr role_based_try& operator = (role_based_try&&) = default;

    /**
     * Strategy options
     *
     * @return boost::hana::map --- strategy options
     */
    constexpr const Options& options() const { return options_;}
    constexpr Options& options() { return options_;}

    /**
     * Try's role index in strategy roles sequence
     *
     * @return boos::hana::size_c<N> --- index constant type object
     */
    static constexpr auto role_index() { return idx_;}

    /**
     * Try's role based on strategy roles sequence and try's index.
     *
     * @return auto - role object
     */
    constexpr auto role() const { return roles_seq()[role_index()];}

    /**
     * Get the operation context.
     *
     * @return `boost::hana::tuple` --- operation initiation context for the try with proper
     *                                  #ConnectionProvider and time constraint;
     */
    auto get_context() const {
        return hana::concat(
            hana::make_tuple(unwrap(ctx_).provider.rebind_role(role()), time_constraint()),
            unwrap(ctx_).args
        );
    }

    /**
     * Time constraint for the try
     *
     * @return time constraint type value calculated for the try
     */
    auto time_constraint() const {
        return detail::get_try_time_constraint(unwrap(ctx_).time_constraint, tries_left().value);
    }

    /**
     * Roles sequence of the strategy
     *
     * @return constexpr decltype(auto)
     */
    constexpr decltype(auto) roles_seq() const {
        return get_option(options(), opt::roles);
    }

    /**
     * Tries left after this try
     *
     * @return hana::size_c<N> --- number of tries left, for the last try it will be boost::hana::size_c<0>
     */
    constexpr auto tries_left() const {
        return decltype(hana::size(roles_seq()) - role_index()){};
    }

    /**
     * Return next try object for retry operation if it possible. See
     * `ozo::fallback::get_next_try()` for details.
     *
     * @param ec --- error code which should be examined for retry ability.
     * @param conn --- #Connection object which should be closed anyway.
     * @return `std::optional<role_based_try>` --- initialized with role_based_try object if retry is possible.
     * @return `std::nullopt` --- retry is not possible due to `ec` value or tries remain
     *                            count.
     */
    template <typename Connection, typename Initiator>
    void initiate_next_try([[maybe_unused]] ozo::error_code ec, Connection& conn, [[maybe_unused]] Initiator&& init) const {
        auto guard = defer_close_connection(get_option(options(), opt::close_connection, true) ? std::addressof(conn) : nullptr);

        if constexpr (decltype(tries_left() > hana::size_c<1>)::value) {
            using fallback_try = role_based_try<Options, Context, decltype(role_index() + hana::size_c<1>)>;
            fallback_try fallback{std::move(options_), std::move(ctx_)};

            if (can_recover(fallback.role(), ec)) {
                get_option(fallback.options(), opt::on_fallback, [](auto&&...){})(ec, conn, std::as_const(fallback));
                init(std::move(fallback));
            } else {
                guard.release();
                fallback.initiate_next_try(ec, conn, std::move(init));
            }
        }
    }
};

template <typename Options, typename Context, typename RoleIdx>
struct initiate_next_try_impl<role_based_try<Options, Context, RoleIdx>> {
    template <typename Connection, typename Initiator>
    static void apply(role_based_try<Options, Context, RoleIdx>& a_try,
            const error_code& ec, Connection& conn, Initiator&& init) {
        a_try.initiate_next_try(ec, conn, std::forward<Initiator>(init));
    }
};

/**
 * @brief Role-based strategy
 *
 * Role-based strategy is a factory for `ozo::fallback::role_based_try` object.
 *
 * @ingroup group-failover-role_based
 */
template <typename Options = decltype(hana::make_map())>
class role_based_strategy : public options_factory_base<role_based_strategy<Options>, Options> {
    using opt = role_based_options;

    friend class ozo::options_factory_base<role_based_strategy<Options>, Options>;
    using base = ozo::options_factory_base<role_based_strategy<Options>, Options>;

    template <typename OtherOptions>
    constexpr static auto rebind_options(OtherOptions&& options) {
        return role_based_strategy<std::decay_t<OtherOptions>>(std::forward<OtherOptions>(options));
    }

public:
    /**
     * @brief Construct a new retry strategy object
     *
     * @param options --- `boost::hana::map` of `ozo::failover::role_based_options` and values.
     */
    role_based_strategy(Options options = Options{}) : base(std::move(options)) {
        static_assert(decltype(hana::is_a<hana::map_tag>(options))::value, "Options should be boost::hana::map");
    }

    template <typename Operation, typename Allocator, typename Source,
            typename TimeConstraint, typename ...Args>
    auto get_first_try(const Operation&, const Allocator&,
            role_based_connection_provider<Source> provider, TimeConstraint t, Args&& ...args) const {

        static_assert(decltype(this->has(opt::roles))::value, "roles should be specified");
        static_assert(!decltype(hana::is_empty(this->get(opt::roles)))::value, "roles should not be empty");

        return role_based_try {
            this->options(),
            basic_context{std::move(provider), ozo::deadline(t), std::forward<Args>(args)...}
        };
    }
};

/**
 * Try to perform an operation on first role with fallback to next items of
 * specified sequence which should recover an error.
 * E.g., in case of sequence consists of <i>role1</i>, <i>role2</i>, <i>role3</i> and <i>role4</i>.
 * If <i>role1</i> cause an error which could be recovered with <i>role3</i> and can not be
 * recovered with <i>role2</i> then the <i>role2</i> will be skipped and operation failover
 * continues from fallback <i>role3</i>.
 *
 * @param roles --- variadic of roles to try operation on.
 * @return `ozo::failover::role_based_strategy` specialization.
 *
 * @note This strategy works only with `ozo::failover::role_based_connection_provider`
 * #ConnectionProvider implementation.
 *
 * ### Time Constraint
 *
 * Operation time constraint interval will be divided between tries as follow.
 *
 * Given operation has a time constraint interval <i>T</i>, each try would have its own
 * time constraint according to the rule:
 *
 * | Try number | Time constraint |
 * |-----|------------------------------|
 * | 1   | <i>T/n</i>                      |
 * | 2   | <i>(T - t<small>1</small>) / (n - 1)</i> |
 * | 3   | <i>(T - (t<small>1</small> + t<small>2</small>)) / (n - 2)</i> |
 * | ... | |
 * | N   | <i>(T - (t<small>1</small> + t<small>2</small> + ... + t<small>n-1</small>)))</i> |
 *
 * , there
 * - <i>t<small>i</small></i> --- actual time of <i>i<small>th</small></i> try,
 * - <i>n</i> - total number of roles.
 *
 * ### Example
 *
 * Try to perform the request on master once and then twice on replica if
 * error code is recoverable.
 *
 * Each try has own time duration constraint calculated as:
 * - for the 1st try: <i>0,5/3 sec.</i>
 * - for the 2nd try: <i>(0,5 - t<small>1</small>) / 2 >= 0,5/3 sec.</i>
 * - for the 3rd try: <i>0,5 - (t<small>1</small> + t<small>2</small>) >= 0,5/3 sec.</i>
 *
 * @code
auto conn_info = ozo::failover::make_role_based_connection_source(
    ozo::failover::master=ozo::connection_info(cfg.master_connstr),
    ozo::failover::replica=ozo::connection_info(cfg.replica_connstr)
);

//...

auto fallback = failover::role_based(failover::master, failover::replica, failover::replica);
ozo::request[fallback](conn_info[io], query, .5s, out, yield);
 * @endcode
 *
 * @sa `ozo::failover::role_based_strategy`, `ozo::failover::role_based_connection_provider`
 * @ingroup group-failover-role_based
 */
template <typename ...Roles>
constexpr auto role_based(Roles ...roles) {
    if constexpr (sizeof...(roles) != 0) {
        return role_based_strategy{hana::make_map(role_based_options::roles=hana::make_tuple(roles...))};
    } else {
        return role_based_strategy{};
    }
}

} // namespace ozo::failover

namespace ozo {

template <typename ...Ts, typename Op>
struct construct_initiator_impl<failover::role_based_strategy<Ts...>, Op>
: failover::construct_initiator_impl {};

} // namespace ozo
