//
//  Print.h
//  HttpServer
//
//  Created by RnMss on 14-1-29.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

#include <cstring>
#include <string>

namespace Stream {
    
    template <class Writer, class Type>
    struct Print;
    
    template <class Writer, class Type>
    void print(Writer &wr, Type data) {
        Print<Writer, Type>()(wr, data);
    }
    
    template <class Writer, class Type, class... Types>
    void print(Writer &wr, Type data, Types... datas) {
        print(wr, data);
        print(wr, datas...);
    }
    
    template <class Writer>
    struct Print<Writer, const char*> {
        void operator() (Writer &wr, const char* str) const {
            size_t len = std::strlen(str);
            write<Writer>(wr, str, len);
        }
    };

    template <class Writer, class Int>
    void print_integer(Writer &wr, Int num) {
        char str[32] = {0};
        bool negetive = num < 0;
        if (negetive) { num = -num; }
        
        char* end = str+32;
        char* ptr = end;
        
        if (num == 0) {
            *(--ptr) = '0';
        } else {
            while (num != 0) {
                char digit = (num % 10) + '0';
                num = num / 10;
                *(--ptr) = digit;
            }
            if (negetive) {
                *(--ptr) = '-';
            }
        }
        
        write<Writer>(wr, ptr, end - ptr);
    }

#pragma push_macro("DECL_PRINT_INT")
#define DECL_PRINT_INT(INT) \
    template <class Writer> \
    struct Print<Writer, INT> { \
        void operator() (Writer &wr, INT n) const { \
            print_integer(wr, n); \
        } \
    }
    
    DECL_PRINT_INT(int);
    DECL_PRINT_INT(unsigned int);
    DECL_PRINT_INT(short);
    DECL_PRINT_INT(unsigned short);
    DECL_PRINT_INT(long);
    DECL_PRINT_INT(unsigned long);
    DECL_PRINT_INT(long long);
    DECL_PRINT_INT(unsigned long long);
    DECL_PRINT_INT(unsigned char);

#pragma pop_macro("DECL_PRINT_INT")
    
    template <class Writer>
    struct Print<Writer, std::string> {
        void operator() (Writer &wr, const std::string &str) const {
            write<Writer>(wr, str.c_str(), str.length());
        }
    };
    
    template <class Writer>
    struct Print<Writer, char> {
        void operator() (Writer &wr, char c) const {
            write<Writer>(wr, &c, 1);
        }
    };

}