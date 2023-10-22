// type traits I use for various literals

#pragma once
#include <type_traits>

// cast enum to int
template <class E>
constexpr std::common_type_t<int, std::underlying_type_t<E>>
e2int(E e) {
    return static_cast<std::common_type_t<int, std::underlying_type_t<E>>>(e);
}

namespace embui_traits{

template<typename T>
struct is_string : public std::disjunction<
        std::is_same<char *, std::decay_t<T>>,
        std::is_same<const char *, std::decay_t<T>>,
        std::is_same<std::string, std::decay_t<T>>,
        std::is_same<std::string_view, std::decay_t<T>>,
        std::is_same<String, std::decay_t<T>>,
        std::is_same<StringSumHelper, std::decay_t<T>>          // String derived helper class
    > {};

// value helper
template<typename T>
inline constexpr bool is_string_v = is_string<T>::value;

template<typename T>
inline constexpr bool is_string_t = is_string<T>::type;

template<typename T>
struct is_string_obj : public std::disjunction<
        std::is_same<std::string, std::decay_t<T>>,
        std::is_same<String, std::decay_t<T>>,
        std::is_same<StringSumHelper, std::decay_t<T>>          // String derived helper class
    > {};

// value helper
template<typename T>
inline constexpr bool is_string_obj_v = is_string_obj<T>::value;

template<typename T>
struct is_string_ptr : public std::disjunction<
        std::is_same<char *, std::decay_t<T>>,
        std::is_same<const char *, std::decay_t<T>>,
        std::is_same<std::string_view, std::decay_t<T>>
    > {};

// value helper
template<typename T>
inline constexpr bool is_string_ptr_v = is_string_ptr<T>::value;

template<typename T>
typename std::enable_if<is_string_obj_v<T>,bool>::type
is_empty_string(const T &label){
    if constexpr(std::is_same_v<std::string, std::decay_t<decltype(label)>>)            // specialisation for std::string
        return label.empty();
    if constexpr(std::is_same_v<String, std::decay_t<decltype(label)>>)                 // specialisation for String
        return label.isEmpty();
    return false;   // UB, not a known string type for us
};

template<typename T>
typename std::enable_if<is_string_ptr_v<T>,bool>::type
is_empty_string(const T label){
    if constexpr(std::is_same_v<std::string_view, std::decay_t<decltype(label)>>)       // specialisation for std::string_view
        return label.empty();
    if constexpr(std::is_same_v<const char*, std::decay_t<decltype(label)>>)            // specialisation for const char*
        return not (label && *label);
    if constexpr(std::is_same_v<char*, std::decay_t<decltype(label)>>)                  // specialisation for char*
        return not (label && *label);
    return false;   // UB, not a known string type for us
};

}

