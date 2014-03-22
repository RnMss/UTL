#pragma once

#include <memory>
#include <unistd.h>
#include <sys/socket.h>

#include "PosixFileDesc.h"

namespace Stream {

    class SocketWriter {
    public:
        SocketWriter() {}

        SocketWriter(PosixFile&& file) 
            : _sock(new PosixFile( std::move(file) )) {}

        SocketWriter(std::shared_ptr<PosixFile> pfile)
            : _sock(pfile) {}

        SocketWriter(const SocketWriter&) = delete;

        SocketWriter(SocketWriter&& that) {
            _sock = that._sock;
            that._sock.reset();
        }

        void close() {
            if (_sock) {
                ::shutdown(_sock->get(), SHUT_WR);
                _sock.reset();
            }
        }

        ~SocketWriter() {
            close();
        }

        size_t write(const void* data, size_t size) {
            return _sock->write(data, size);
        }

    protected:
        std::shared_ptr<PosixFile> _sock;
    };
}
