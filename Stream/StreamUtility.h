//
//  StreamUtility.h
//  HttpServer
//
//  Created by RnMss on 14-2-1.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

#include "StreamDefs.h"
#include <memory>

namespace Stream {
    
    template <class Reader, class Writer>
    void copy_io(Reader& rd, Writer &wr, size_t bufsize = 4096) {
        std::unique_ptr<char[]> buf(bufsize);

        for ( ; ; ) {
            size_t sr = Stream::read<Reader>(rd, buf, bufsize);
            if (sr == 0) { break; }
            
            char* ptr = buf.get();
            while (sr > 0) {
                size_t sw = Stream::write<Writer>(wr, ptr, size);
                sr -= sw; ptr += sw;
            }
        }
    }
}
