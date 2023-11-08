/*
    This file is part of EmbUI project
    https://github.com/vortigont/EmbUI

    Copyright © 2023 Emil Muratov (Vortigont)   https://github.com/vortigont/

    EmbUI is free software: you can redistribute it and/or modify
    it under the terms of MIT License https://opensource.org/license/mit/

    This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
    then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
    also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
    and others people
*/

// type traits I use for various literals

#pragma once
#include <type_traits>
#include "ArduinoJson.h"

// cast enum to int
template <class E>
constexpr std::common_type_t<int, std::underlying_type_t<E>>
e2int(E e) {
    return static_cast<std::common_type_t<int, std::underlying_type_t<E>>>(e);
}

// std::string_view.ends_with before C++20
bool ends_with(std::string_view str, std::string_view sv);

// std::string_view.ends_with before C++20
bool ends_with(std::string_view str, const char* sv);

// std::string_view.starts_with before C++20
bool starts_with(std::string_view str, std::string_view sv);

// std::string_view.starts_with before C++20
bool starts_with(std::string_view str, const char* sv);

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
        std::is_same<StringSumHelper, std::decay_t<T>>,          // String derived helper class
        std::is_same<detail::StaticStringAdapter, std::decay_t<T>>
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
    if constexpr(std::is_same_v<detail::StaticStringAdapter, std::decay_t<decltype(label)>>)
        return label.isNull();

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

