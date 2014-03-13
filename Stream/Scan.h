//
//  Scan.h
//  HttpServer
//
//  Created by RnMss on 14-1-29.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

template <class Reader, class Type>
struct Scan;

template <class Reader, class Type>
void scan(Reader &wr, Type &data) {
    Scan<Reader, Type>()(wr, data);
}

template <class Reader, class Type, class... Types>
void scan(Reader &wr, Type &data, Types... datas) {
    scan(wr, data);
    scan(wr, datas...);
}

template <class Reader>
struct Scan<Reader, int> {
    void operator() (Reader &wr, int& result) const {
        size_t len = std::strlen(str);
        write<Reader>(wr, str, len);
    }
};

template <class Reader>
struct Scan<Reader, const char*> {
    void operator() (Reader &wr, const char* str) const {
    }
};
