//
//  StdFile.hpp
//  HttpServer
//
//  Created by RnMss on 14-1-9.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

#include "StreamDefs.h"
#include "BufferedReader.h"

#include <unistd.h>

#include <algorithm>
#include <utility>

namespace Stream {

    class PosixFile {
    public:
        PosixFile() : id(-1) {}
        explicit PosixFile(int _fd) : id(_fd) {}
        
        PosixFile(PosixFile&& that) : id(that.id) {
            that.id = -1;
        }
        
        PosixFile& operator= (PosixFile&& that) {
            std::swap(id, that.id);
            return *this;
        }

        PosixFile(const PosixFile& that) {
            id = ::dup(that.id);
        }

        PosixFile& operator= (const PosixFile& that) {
            this->~PosixFile();
            id = ::dup(that.id);
            return *this;
        }
        
        ~PosixFile() {
            if (id >= 0) { ::close(id); }
        }
        
        int get() const {
            return id;
        }
        
        bool empty() const {
            return id < 0;
        }
        
        size_t read(void* data, size_t size) {
            ssize_t res = ::read(id, data, size);
            if (res < 0) res = 0;
            return res;
        }
        
        size_t write(const void* data, size_t size) {
            ssize_t res = ::write(id, data, size);
            if (res < 0) res = 0;
            return res;
        }
        
        void close() {
            if (id >= 0) {
                int ret = ::close(id);
                if (ret != 0) {
                    throw std::runtime_error("socket close failed.");
                }
                id = -1;
            }
        }
        
    protected:
        int id;

    };

}