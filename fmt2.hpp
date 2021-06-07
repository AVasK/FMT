#pragma once

#include <iostream>
#include <sstream>
#include <string>

//template <typename T>        
//class Error;

namespace fmt {

// METAPROGRAMMING PART:
namespace parser {

template <bool flag, typename T, typename F>
struct select {
    using type = T;
};

template<typename T, typename F>
struct select<false, T, F> {
    using type = F;
};

template <bool flag, typename T, typename F>
using select_t = typename select<flag, T, F>::type;


template <typename... Ts>
struct TypeList {
    template <typename U>
    using append = TypeList<Ts..., U>;

    template <typename U>
    using push = TypeList<U, Ts...>;
};

template <size_t Val>
struct Int2Type {
    enum { value = Val };
};

template <size_t Val> struct IndexT : Int2Type<Val> {};
template <size_t Val> struct CoordT : Int2Type<Val> {};


/// FWD declarations:
template <class CharT, size_t Idx, char Letter, CharT... Letters> struct scan;
template <class CharT, size_t Idx, char Letter, CharT... Letters> struct fmt;
template <class CharT, size_t Idx, size_t _id, char Letter, CharT... Letters> struct arg_id;

/// Defs:
struct true_value { enum {value = true}; };

template <char C>
struct is_digit {
    enum {value = false};
    enum {as_int = 0};
};

template <> struct is_digit<'0'> : true_value {enum {as_int=0};};
template <> struct is_digit<'1'> : true_value {enum {as_int=1};};
template <> struct is_digit<'2'> : true_value {enum {as_int=2};};
template <> struct is_digit<'3'> : true_value {enum {as_int=3};};
template <> struct is_digit<'4'> : true_value {enum {as_int=4};};
template <> struct is_digit<'5'> : true_value {enum {as_int=5};};
template <> struct is_digit<'6'> : true_value {enum {as_int=6};};
template <> struct is_digit<'7'> : true_value {enum {as_int=7};};
template <> struct is_digit<'8'> : true_value {enum {as_int=8};};
template <> struct is_digit<'9'> : true_value {enum {as_int=9};};

/// Scanning routines

template <class CharT, size_t Idx, char Letter>
struct wrong_fmt_error {
    // need static_assert to depend on a template parameter to defer
    // the error invocation to the templ.instantiation.
    static_assert(Letter && false, "wrong format");
};


// if '}' is met, just fall back to scanning
template <class CharT, size_t Idx, CharT... Letters>
struct fmt<CharT, Idx, '}', Letters...> {
    using list = typename scan<CharT, Idx+1, Letters...>::list :: template push< CoordT<Idx+1> >;
};

// else parse::arg_id
template <class CharT, size_t Idx, char Letter, CharT... Letters>
struct fmt {
    using list = typename parser::arg_id<CharT, Idx, 0, Letter, Letters...>::list;
};

template <class CharT, size_t Idx, size_t _id, char Letter, CharT... Letters>
struct arg_id {
private:
    enum {upd_id = _id*10 + is_digit<Letter>::as_int};
public:
    using list = select_t<
        is_digit<Letter>::value, 
        typename arg_id<CharT, Idx+1, upd_id, Letters...>::list, // PARSE ARG_ID
        parser::wrong_fmt_error<CharT, Idx, Letter>
    >;
};

template <class CharT, size_t Idx, size_t _id, CharT... Letters>
struct arg_id<CharT, Idx, _id, ':', Letters...> { // when : is met, change to parsing output format specifier.
    using list = TypeList<>;
};

template <class CharT, size_t Idx, size_t _id, CharT... Letters>
struct arg_id<CharT, Idx, _id, '}', Letters...> { // when } is met, change back to scanning.
    using list = typename scan<CharT, Idx+1, Letters...>::list :: template push< CoordT<Idx+1> > :: template push< IndexT<_id> >;
};

template <class CharT, size_t Idx, size_t _id, CharT... Letters>
struct arg_id<CharT, Idx, _id, '\0', Letters...> { // when '\0' is met, that's 100% an error.
    static_assert(Idx && false, "unexpected eof");
    using list = TypeList<>;
};

template <class CharT, size_t Idx, char Letter, CharT... Letters>
struct check_escape_char { // basic case: starting to parse format string
    //using list = typename scan<CharT, Idx+1, Letters...>::list :: template push< Int2Type<Idx-1> >;
    using list = typename parser::fmt<CharT, Idx, Letter, Letters...>::list :: template push< CoordT<Idx-1> >;
};

// HANDLE WCHAR_T mb?
// template <size_t Idx, wchar_t Letter, wchar_t... Letters>
// struct check_escape_char<wchar_t, Idx, Letter, Letters...> { // basic case: starting to parse format string
//     //using list = typename scan<CharT, Idx+1, Letters...>::list :: template push< Int2Type<Idx-1> >;
//     using list = typename parser::fmt<wchar_t, Idx, Letter, Letters...>::list :: template push< CoordT<Idx-1> >;
// };

template <class CharT, size_t Idx, CharT... Letters>
struct check_escape_char<CharT, Idx, '{', Letters...> { // an ecsape character met, returning to scanning
    using list = typename scan<CharT, Idx+1, Letters...>::list;
};

template <class CharT, size_t Idx, char Letter, CharT... Letters>
struct scan {
    using list = typename scan<CharT, Idx+1, Letters...>::list;
};

template <class CharT, size_t Idx, CharT... Letters>
struct scan<CharT, Idx, '{', Letters...> {
    using list = typename check_escape_char<CharT, Idx+1, Letters...>::list;
};

template <class CharT, size_t Idx>
struct scan<CharT, Idx, '\0'> {
    using list = TypeList<>;
};

/// Checking fmt type:
/// is_positional : TypeList< IndexT | CoordT >  --->  bool
/// Supported types are:
/// 1. plain {} 
/// 2. positional args: e.g. {1} and {0}
template <typename T, typename... Ts>
struct has_positional_args;

template <typename T>
struct has_positional_args< TypeList<T> > {
    enum { value = false };
};

template <typename T, typename... Ts>
struct has_positional_args< TypeList<T, Ts...> > {
    enum { value = has_positional_args< TypeList<Ts...> >::value };
};

template <size_t Id, typename... Ts>
struct has_positional_args< TypeList<IndexT<Id>, Ts...> > {
    enum { value = true };
};

/// Check if all the IDs are set
template <typename T>
struct positional_all_ids_set {
    enum { value = false };
};

template <size_t I1, size_t IDX, size_t I2, typename... Ts>
struct positional_all_ids_set< TypeList<CoordT<I1>, IndexT<IDX>, CoordT<I2>, Ts...> > {
    enum { value = positional_all_ids_set< TypeList<Ts...> >::value };
};

template <size_t I1, size_t IDX, size_t I2>
struct positional_all_ids_set< TypeList<CoordT<I1>, IndexT<IDX>, CoordT<I2>> > {
    enum { value = true };
};

// Id range:
template <typename... Ts>
struct id_range;

template <size_t Id, typename... Ts>
struct id_range< TypeList<IndexT<Id>, Ts...> > {
private:
    using tail_range = id_range< TypeList<Ts...> >;
public:
    enum {
        max = tail_range::max > Id ? tail_range::max : Id,
        min = tail_range::min < Id ? tail_range::min : Id
    };
};

template <size_t X, typename... Ts>
struct id_range< TypeList<CoordT<X>, Ts...> > {
private:
    using tail_range = id_range< TypeList<Ts...> >;
public:
    enum {
        max = tail_range::max,
        min = tail_range::min
    };
};

template <size_t Id, size_t X>
struct id_range< TypeList<IndexT<Id>, CoordT<X>> > {
    enum {
        max = Id,
        min = Id
    };
};

//using idx = typename scan<char, 0, 'a','{','1','}','{','{','\0'> :: list;
//using idx = typename scan<char, 0, 'a','{','1','}','{','2','}','{','{','\0'> :: list;
using idx = typename scan<char, 0, 'a','{','1','}','{','}','{','{','\0'> :: list;
using swtch = select_t<has_positional_args<idx>::value, bool, int>;
using check = select_t<positional_all_ids_set<idx>::value, bool, int>;
//static constexpr auto range = id_range< idx > :: min; // only use id_range if positional_all_ids_set<...>!

using TL = TypeList< Int2Type<0> >;
using TL2 = TL::push< Int2Type<1> >;

struct PlainMode;
struct PositionalMode;

};// namespace parser


template <size_t N, typename CharT>
struct NString {
    CharT s[N+1];
};

template <typename... Ts>
struct split_by {
    static void call(char * s) {}
};

template <size_t S, size_t Idx, size_t Next, typename... Ts>
struct split_by< parser::CoordT<S>, parser::IndexT<Idx>, parser::CoordT<Next>, Ts... >
{
    static void call(char * s) {
        s[S] = '\0';
        split_by< Ts... >::call(s);
    }
};

template <size_t S1>
struct split_by< parser::CoordT<S1> >
{
    static void call(char * s) {
        s[S1] = '\0';
    }
};

// for non-positional args
template <size_t S, size_t Next, typename... Ts>
struct split_by< parser::CoordT<S>, parser::CoordT<Next>, Ts... >
{
    static void call(char * s) {
        s[S] = '\0';
        split_by< Ts... >::call(s);
    }
};


template <typename Str, typename TList, typename FmtMode>
class FmtProxy;

template <size_t N, typename CharT, typename... Coords>
class FmtProxy < NString<N,CharT>, parser::TypeList<Coords...>, parser::PlainMode > {

    FmtProxy (NString<N,CharT> && s)
        : str {std::move(s)}
    {}

    template <typename... Ts>
    std::string operator() (Ts&&... args)
    {
        std::cerr << "Plain format\n";
        return {};
    }
private:
    NString<N,CharT> str;
};

// Getter for variadic template args
template <size_t i, typename T, typename... Ts>
struct get_arg_type {
    using type = typename get_arg_type<i-1, Ts...>::type;
};

template <typename T, typename... Ts>
struct get_arg_type<0, T, Ts...> {
    using type = T;
};

//using A = get_arg_type<1, int, float, double>::type;

template <size_t I>
struct get_arg {

    template <typename T, typename... Ts>
    static auto from_args(T&& arg, Ts&&... args) -> typename get_arg_type<I, T, Ts...>::type
    {
        return get_arg<I-1>::from_args(std::forward<Ts>(args)...);
    }
};

template <>
struct get_arg<0> {
    template <typename T, typename... Ts>
    static T from_args(T&& arg, Ts&&... args) 
    {
        return std::forward<T>(arg);
    }
};

template <size_t Idx, typename... Ts>
inline constexpr auto get(Ts&&... args) noexcept
{
    return get_arg<Idx>::from_args(std::forward<Ts>(args)...);
}


template <typename... Ts>
struct positional;

template <size_t Prev, size_t Fmt, size_t Index, size_t Next, typename... Coords>
struct positional<parser::CoordT<Prev>, parser::CoordT<Fmt>, parser::IndexT<Index>, parser::CoordT<Next>, Coords...> {
    template <typename Stream, typename... Ts>
    static void to_stream(Stream& os, char* s, Ts&&... args)
    {
        os << (char *)(s + Prev);
        //os << get_arg<Index>::from_args(args...);
        os << get<Index>(args...);
        positional<parser::CoordT<Next>, Coords...>::to_stream(os, s, args...);
    }
};

template <size_t Prev>
struct positional< parser::CoordT<Prev> > {
    template <typename Stream, typename... Ts>
    static void to_stream(Stream& os, char* s, Ts&&... args)
    {
        os << (char*)(s + Prev);
    }
};

template <size_t N, typename CharT, typename... Coords>
class FmtProxy < NString<N,CharT>, parser::TypeList<Coords...>, parser::PositionalMode > {
private:
    using parsed = parser::TypeList<Coords...>;
public:

    FmtProxy (NString<N,CharT> && s) 
        : str {std::move(s)} 
    {};

    template <typename... Ts>
    std::string operator() (Ts&&... args)
    {
        using range = parser::id_range< parsed >;
        static_assert(
            range::max <= sizeof...(args) - 1,
            "Argument index out of range!"
        );

        // replace '{' -> '\0' to print chunks
        split_by<Coords...>::call(static_cast<char*>( str.s )); 

        std::ostringstream oss;
        positional<parser::CoordT<0>, Coords...>::to_stream(oss, (char*)str.s, std::forward<Ts>(args)...);
        return oss.str();
    }

    // template <typename... Ts>
    // FmtProxy & operator() (Ts... args)
    // {
    //     using range = parser::id_range< parsed >;
    //     static_assert(
    //         range::max <= sizeof...(args) - 1,
    //         "Argument index out of range!"
    //     );

    //     std::cerr << "Positional format\n";
    //     std::cerr << str.s << "\n";

    //     return *this;
    // }


    // template <typename... Ts>
    // FmtProxy & operator() (Ts... args)
    // {
    //     using range = parser::id_range< parsed >;
    //     static_assert(
    //         range::max <= sizeof...(args) - 1,
    //         "Argument index out of range!"
    //     );

    //     std::cerr << "Positional format\n";
    //     std::cerr << str.s << "\n";
    //     return Formatted< NString<N,CharT>, std::tuple<Ts...>, parsed, parser::PositionalMode > {
    //         str,
    //         std::make_tuple( args... ) // WARNING: ALWAYS COPIES!
    //     };
    // }

private:
    NString<N,CharT> str;
};

namespace fmt_literal {

#if (defined(__GNUC__) || defined(__clang__)) && __cplusplus/100 >= 2014
template <class CharT, CharT... Letters>
constexpr auto operator""_f()
{
    using parsed = typename parser::scan<CharT, 0, Letters...,'\0'> :: list;
    constexpr auto is_positional_mode = parser::has_positional_args<parsed>::value;
    static_assert(
        !is_positional_mode || parser::positional_all_ids_set<parsed>::value,
        "Not all positional args set!"
    );

    //constexpr auto test = Error< parsed >();

    return FmtProxy< NString<sizeof...(Letters), CharT>, parsed, parser::select_t<is_positional_mode, parser::PositionalMode, parser::PlainMode> >{ 
        NString<sizeof...(Letters), CharT> {Letters..., '\0'}
     };
}
#else
#error C++14 GCC/Clang expected. (or mb i will roll some other non-compile-time impl in the future for such cases)
#endif

}; //fmt_literal
// can extract min. added size @ compile time:
// e.g.: {:5i} {} will be >= 5
// if all the fields widths are known, then we can construct 
// a buffer of known size for the whole output string.

}; //namespace fmt