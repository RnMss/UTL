//
//  BufferedReader.h
//  HttpServer
//
//  Created by RnMss on 14-1-9.
//  Copyright (c) 2014年 RnMss. All rights reserved.
//

#pragma once

#include "StreamDefs.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <cstring>

namespace Stream {

    template <class Reader, class ReadFunc = Read<Reader> >
    class BufferedReader {
    public:
        BufferedReader() {}
        
        explicit
        BufferedReader(Reader&& file, size_t buffer_size = 4096) {
            Body* pbody = new Body(std::move(file));
            body = std::unique_ptr<Body>(pbody);
            body->buffer = new char[buffer_size];
            body->buffer_size = buffer_size;
            initialize();
        }

        explicit
        BufferedReader(const Reader& file, size_t buffer_size = 4096) {
            Body* pbody = new Body(file);
            body = std::unique_ptr<Body>(pbody);
            body->buffer = new char[buffer_size];
            body->buffer_size = buffer_size;
            initialize();
        }
        
        BufferedReader(BufferedReader&& that) {
            body = std::move(that.body);
        }
    
        BufferedReader& operator= (BufferedReader&& that) {
            body = std::move(that.body);
            return *this;
        }
        
        ~BufferedReader() {
            if (body == nullptr) { return; }
            
            body->mtx_rw.lock();
            body->is_running = false;
            body->cnd_r.notify_all();
            body->mtx_rw.unlock();
            
            Stream::close<Reader>(body->file);
            
            body->reader_thread.join();

            delete[] body->buffer;
        }

        BufferedReader(const BufferedReader&) = delete;
        BufferedReader& operator= (const BufferedReader&) = delete;
        
        size_t read(void* buffer, size_t length) {
            return body->read(buffer, length);
        }

        size_t readline(char* buffer, size_t length, const char* eol = "\n") {
            return body->readline(buffer, length, eol);
        }

        std::string getline(const std::string& eol) {
            return body->getline(eol);
        }

        std::string getline(size_t maxlen, const char* eol) {
            std::unique_ptr<char[]> buf(new char[maxlen]);
            size_t size = readline(buf.get(), maxlen, eol);
            if (size == 0) {
                throw std::runtime_error("BufferedReader::getline failed.");
            }
            
            return std::string(&buf[0], &buf[size]);
        }
        
    private:
        struct Body {
            Reader file;
            ReadFunc readfunc;
            
            char* buffer;
            size_t q_head;
            size_t q_tail;
            bool q_empty;
            size_t buffer_size;
            
            std::thread reader_thread;
            
            bool is_running;
            
            std::mutex mtx_rw;
            std::condition_variable_any cnd_r;
            std::condition_variable_any cnd_w;
            
            Body(const Reader& f) : file(f) {}
            Body(Reader&& f) : file(std::move(f)) {}
            
            void keep_reading() {
                while (true) {
                    size_t n_size;
                    
                    mtx_rw.lock();
                    while (is_running) {
                        n_size = (q_tail>q_head || q_empty)
                                    ? buffer_size-q_tail
                                    : q_head-q_tail ;
                        
                        if (n_size == 0) { cnd_r.wait(mtx_rw); }
                                    else { break; }
                    }
                    
                    char* ptr_tail = buffer+q_tail;
                    mtx_rw.unlock();
                    
                    if (!is_running) { break; }
                    
                    ssize_t n_read = readfunc(file, ptr_tail, n_size);
                    
                    if (n_read <= 0) { break; }
                    
                    mtx_rw.lock();
                    {
                        q_tail += n_read;
                        q_empty = false;
                        if (q_tail == buffer_size) { q_tail = 0; }
                    }
                    mtx_rw.unlock();
                    cnd_w.notify_one();
                }
                is_running = false;
                cnd_w.notify_all();
            }

            size_t read(void* o_buf, size_t i_len) {
                char* buf = (char*)o_buf;
                size_t len = i_len;
                
                while (len > 0) {
                    mtx_rw.lock();
                    
                    while (q_empty && is_running) {
                        cnd_w.wait(mtx_rw);
                    }
                    if (q_empty) {
                        mtx_rw.unlock();
                        break;
                    }
                    
                    size_t n_size = q_tail<=q_head ? buffer_size-q_head : q_tail-q_head;
                    if (n_size > len) { n_size = len; }
                    
                    char* ptr_head = buffer+q_head;
                    
                    mtx_rw.unlock();
                    
                    memcpy(buf, ptr_head, n_size);
                    
                    mtx_rw.lock();
                    
                    q_head += n_size;
                    if (q_head == buffer_size) { q_head = 0; }
                    if (q_head == q_tail) { q_empty = true; }
                    
                    mtx_rw.unlock();
                    cnd_r.notify_one();
                    
                    buf += n_size;
                    len -= n_size;
                }
                
                return i_len - len;
            }
            
            int peek() {
                mtx_rw.lock();
                
                while (q_empty && is_running) {
                    cnd_w.wait(mtx_rw);
                }
                if (q_empty) {
                    mtx_rw.unlock();
                    return -1;
                }
                
                char res = buffer[q_head];
                
                if (++q_head == buffer_size) { q_head = 0; }
                if (q_head == q_tail) { q_empty = true; }
                
                mtx_rw.unlock();
                cnd_r.notify_one();

                return res;
            }

            std::string getline(const std::string& eol) {
                const size_t LEN = buffer_size;
                std::unique_ptr<char[]> buf = new char[LEN+1];

                std::string result;
                for ( ; ; ) {
                    size_t n = readline(buf, LEN, eol.c_str());
                    std::string postfix(&buf[n - eol.length()], &buf[n]);
                    if (n < LEN) {
                        buf[n] = '\0';
                        result.append(buf.get());
                        break;
                    } 
                    if (eol == postfix) {
                        buf[n - eol.length()] = '\0';
                        result.append(buf.get());
                        break;
                    }
                }
                return result;
            }
            
            size_t readline(char* buf, size_t len, const char* eol) {
                #warning 这个实现有bug，将来请改为KMP算法
                
                size_t i_len = len;
                int matched = 0;
                while (len > 0 && eol[matched] != '\0') {
                    mtx_rw.lock();
                    
                    while (q_empty && is_running) {
                        cnd_w.wait(mtx_rw);
                    }
                    if (q_empty) {
                        mtx_rw.unlock();
                        break;
                    }
                    
                    size_t n_size = q_tail<=q_head ? buffer_size-q_head : q_tail-q_head;
                    if (n_size > len) { n_size = len; }
                    
                    char* ptr_head = buffer+q_head;
                    
                    mtx_rw.unlock();
                    
                    if (matched == 0) {
                        char* found = (char*)memchr(ptr_head, eol[0], n_size);
                        
                        if (found != nullptr) {
                            n_size = found - ptr_head + 1;
                            matched += 1;
                        }
                        
                        if (buf != nullptr) {
                            memcpy(buf, ptr_head, n_size);
                        }
                    } else {
                        if (ptr_head[0] == eol[matched]) {
                            buf[0] = ptr_head[0];
                            matched += 1;
                            n_size = 1;
                        } else {
                            matched = 0;
                            n_size = 0;
                        }
                    }

                    mtx_rw.lock();
                    
                    q_head += n_size;
                    if (q_head == buffer_size) { q_head = 0; }
                    if (q_head == q_tail) { q_empty = true; }
                    
                    mtx_rw.unlock();
                    cnd_r.notify_one();
                    
                    if (buf != nullptr) {
                        buf += n_size;
                    }
                    len -= n_size;
                }
                
                return i_len - len;
            }
        };
        
        std::unique_ptr<Body> body;
        
        void initialize() {
            body->q_head = 0;
            body->q_tail = 0;
            body->q_empty = true;
            body->is_running = true;
            
            Body* pbody = body.get();
            body->reader_thread = std::thread(
                [ pbody ] { pbody->keep_reading(); }
            );
        }
    };

}
