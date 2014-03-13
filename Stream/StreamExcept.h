//
//  StreamExcept.h
//  HttpServer
//
//  Created by RnMss on 14-1-9.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

#include <stdexcept>

namespace Stream {
    class StreamException
        : public std::runtime_error
    {
    public:
        StreamException(const char*        reason) : std::runtime_error(reason) {}
        StreamException(const std::string& reason) : std::runtime_error(reason) {}
    };
}