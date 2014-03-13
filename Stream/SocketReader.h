#pragma once

#include <memory>
#include <unistd.h>
#include <sys/socket.h>

#include "PosixFileDesc.h"

namespace Stream {

    class SocketReader {
    public:
        SocketReader() {}

        SocketReader(PosixFile&& file) 
            : _sock(new PosixFile( std::move(file) )) {}

        SocketReader(std::shared_ptr<PosixFile> pfile)
            : _sock(pfile) {}

        SocketReader(const SocketReader&) = delete;

        SocketReader(SocketReader&& that) {
            _sock = that._sock;
            that._sock.reset();
        }

        void close() {
            if (_sock) {
                ::shutdown(_sock.get(), SHUT_RD);
                _sock.reset();
            }
        }

        ~SocketReader() {
            close();
        }

        size_t read(void* data, size_t size) {
            return _sock->read(data, size);
        }

    protected:
        std::shared_ptr<PosixFile> _sock;
    };
}