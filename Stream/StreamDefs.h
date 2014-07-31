//
//  File.hpp
//  HttpServer
//
//  Created by RnMss on 14-1-9.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>

#include "StreamExcept.h"

namespace Stream {

    template <class Writer>
    struct Write {
        size_t operator() (Writer& wr, const void* data, size_t size) const {
            return wr.write(data, size);
        }
    };
    
    template <class Reader>
    struct Read {
        size_t operator() (Reader& rd, void* data, size_t size) const {
            return rd.read(data, size);
        }
    };
    
    template <class File>
    struct Close {
        void operator() (File& file) const {
            return file.close();
        }
    };
    
    template <class Writer>
    size_t write(Writer& wr, const void* data, size_t size) {
        return Write<Writer>()(wr, data, size);
    }

    template <class Reader>
    size_t read(Reader& rd, void* data, size_t size) {
        return Read<Reader>()(rd, data, size);
    }
    
    template <class File>
    void close(File& file) {
        Close<File>()(file);
    }
    
    template <class Writer, class Data>
    struct Put {
        typename std::enable_if<std::is_trivial<Data>::value>::type
            operator() (Writer& wr, const Data& data) const
        {
            size_t s = write(wr, &data, sizeof(Data));
            if (s != sizeof(Data)) {
                throw StreamException("error occured writing stream");
            }
        }
    };

    template <class Reader, class Data>
    struct Get {
        typename std::enable_if<std::is_trivial<Data>::value, Data>::type
            operator() (Reader& rd) const
        {
            Data result;
            size_t s = read(rd, &result, sizeof(result));
            if (s != sizeof(Data)) {
                throw StreamException("error occured reading stream");
            }
            return std::move(result);
        }
    };
    
    template <class Data, class Writer>
    void put(Writer& wr, const Data& data) {
        Put<Writer, Data>()(wr, data);
    }
    
    template <class Data, class Reader>
    Data get(Reader& rd) {
        return std::move(Get<Reader, Data>()(rd));
    }

}
