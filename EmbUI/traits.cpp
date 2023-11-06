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

#include <string_view>
#include "traits.hpp"

// std::string_view.ends_with before C++20
bool ends_with(std::string_view str, std::string_view sv){
    return str.size() >= sv.size() && str.compare(str.size() - sv.size(), str.npos, sv) == 0;
}

// std::string_view.ends_with before C++20
bool ends_with(std::string_view str, const char* sv){
    return ends_with(str, std::string_view(sv));
}

// std::string_view.starts_with before C++20
bool starts_with(std::string_view str, std::string_view sv){
    return str.substr(0, sv.size()) == sv;
}

// std::string_view.starts_with before C++20
bool starts_with(std::string_view str, const char* sv){
    return starts_with(str, std::string_view(sv));
}
