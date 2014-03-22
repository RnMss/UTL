#pragma once
//
//  BufferedWriter.h
//  HttpServer
//
//  Created by RnMss on 14-1-29.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#include <cassert>

namespace Stream {

    template <class Writer, class WriteFunc = Write<Writer> >
    class BufferedWriter {
    public:
        BufferedWriter() {}
        
        explicit
        BufferedWriter(Writer&& file, size_t batch_size = 1024, size_t buffer_size = 4096) {
            Body* pbody = new Body(std::move(file));
            body = std::unique_ptr<Body>(pbody);
            body->buffer = new char[buffer_size];
            body->buffer_size = buffer_size;
            body->flush_barrier = buffer_size;
            body->batch_size = batch_size;
            initialize();
        }
        
        explicit
        BufferedWriter(const Writer& file, size_t batch_size = 1024, size_t buffer_size = 4096) {
            Body* pbody = new Body(file);
            body = std::unique_ptr<Body>(pbody);
            body->buffer = new char[buffer_size];
            body->buffer_size = buffer_size;
            body->flush_barrier = buffer_size;
            body->batch_size = batch_size;
            initialize();
        }
        
        BufferedWriter(BufferedWriter&& that) {
            body = std::move(that.body);
        }
        
        BufferedWriter& operator= (BufferedWriter&& that) {
            body = std::move(that.body);
            return *this;
        }
        
        ~BufferedWriter() {
            if (body == nullptr) { return; }
            
            close();
            body->writer_thread.join();
            
            delete[] body->buffer;
        }
        
        BufferedWriter(const BufferedWriter&) = delete;
        BufferedWriter& operator= (const BufferedWriter&) = delete;
        
        size_t write(const void* data, size_t size) {
            return body->write(data, size);
        }
        
        void flush() {
            body->flush();
        }
        
        void close() {
            body->is_running = false;
            body->cnd_w.notify_all();
            body->cnd_r.notify_all();
        }
        
    private:
        struct Body {
            Writer file;
            WriteFunc writefunc;
            
            char* buffer;
            size_t q_head;
            size_t q_tail;
            bool q_empty;
            size_t buffer_size;
            size_t batch_size;
            size_t flush_barrier;
            
            std::thread writer_thread;
            
            bool is_running;
            
            std::mutex mtx_rw;
            std::condition_variable_any cnd_r;
            std::condition_variable_any cnd_w;
            
            Body(const Writer& f) : file(f) {}
            Body(Writer&& f) : file(std::move(f)) {}

        bool want_flush() const {
            return flush_barrier != buffer_size;
        }
            
        void keep_writing() {
            while (true) {

                size_t n_size;
                size_t q_size;
                
                mtx_rw.lock();
                for ( ; ; ) {
                    if (q_tail > q_head || q_empty) {
                        n_size = q_tail - q_head;
                        q_size = n_size;
                    } else {
                        n_size = buffer_size - q_head;
                        q_size = n_size + q_tail;
                    }
                   
                    if (!is_running) { break; }
                    if (want_flush()) { break; }
                    if (q_size > batch_size) { break; }
                    cnd_w.wait(mtx_rw);
                }
                
                if (!is_running && q_empty) {
                    mtx_rw.unlock();
                    break;
                }
                
                char* ptr_head = buffer + q_head;
                mtx_rw.unlock();
                
                ssize_t n_write = writefunc(file, ptr_head, n_size);
                
                if (n_write <= 0) {
                    mtx_rw.unlock();
                    break;
                }
                
                mtx_rw.lock();
                {
                    size_t q_head_2 = q_head + n_write;
                    if (q_head_2 == buffer_size) { q_head = 0; }

                    if (q_head < flush_barrier && flush_barrier <= q_head_2) {
                        flush_barrier = buffer_size;
                    }

                    q_head = q_head_2;
                    
                    q_empty = (q_head == q_tail);
                    
                }
                mtx_rw.unlock();
                cnd_r.notify_all();
            }

            is_running = false;
            cnd_r.notify_all();
        }
        
        size_t write(const void* data, size_t size) {
            const char* buf = (const char*)data;
            size_t len = size;
            
            while (len > 0) {
                mtx_rw.lock();
                
                while (!q_empty && q_head==q_tail && is_running) {
                    cnd_r.wait(mtx_rw);
                }
                if (!is_running) {
                    mtx_rw.unlock();
                    break;
                }
                
                size_t n_size = q_tail<q_head ? q_head-q_tail : buffer_size-q_tail ;
                if (n_size > len) { n_size = len; }
                
                char* ptr_tail = buffer+q_tail;
                
                mtx_rw.unlock();
                
                memcpy(ptr_tail, buf, n_size);
                
                mtx_rw.lock();
                
                q_tail += n_size;
                if (q_tail == buffer_size) { q_tail = 0; }
                q_empty = false;
                
                mtx_rw.unlock();
                cnd_w.notify_one();
                
                buf += n_size;
                len -= n_size;
            }
            
            return size - len;
        }
            
        void flush() {
            mtx_rw.lock();
            {
                assert(is_running);
                
                if (q_empty) {
                    mtx_rw.unlock();
                    return;
                }

                flush_barrier = q_tail;
                cnd_w.notify_all();

            }
            mtx_rw.unlock();
            
            mtx_rw.lock();
            while (want_flush()) {
                cnd_r.wait(mtx_rw);
            }
            mtx_rw.unlock();
        }
        
        };
        
        std::unique_ptr<Body> body;

        void initialize() {
            body->q_head = 0;
            body->q_tail = 0;
            body->q_empty = true;
            body->is_running = true;
            
            Body* pbody = body.get();
            body->writer_thread = std::thread(
                [ pbody ] { pbody->keep_writing(); }
            );
        }
    };

}
