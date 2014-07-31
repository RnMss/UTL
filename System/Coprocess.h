#pragma once

/*****************************************************************
 *
 * [ bash ]
 *
 * /bin/awk 'print $1'  0<file_a 1>file_b 2>file_c
 * 
 * [ C++ ]
 *
 * File file_a, file_b, file_c;
 * Coprocess proc( { "/bin/awk", "{print $1}" },
 *                 { 0_FD <<file_a, 1_FD >>file_b, 2_FD>>file_c } 
 *                 { "HOME=/home/work" }
 *               );
 * ...
 * proc.wait();
 *
 *****************************************************************/

#include <vector>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include "Stream/Stream.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

namespace System {

    enum class redirect_method { input, output };

    struct redirect_clause_t {
        Stream::PosixFile& file;
        redirect_method method;
        int coprocess_fileno;
    };

    struct FileNo {
        int fileno;

        FileNo(int f) : fileno(f) {}

        redirect_clause_t operator<< (Stream::PosixFile& file) {
            return redirect_clause_t
                { file, redirect_method::input, fileno };
        }

        redirect_clause_t operator>> (Stream::PosixFile& file) {
            return redirect_clause_t
                { file, redirect_method::output, fileno };
        }
    };

    /*
    constexpr FileNo operator"" _FNO (unsigned long long int fileno) {
        return FileNo(fileno);
    }

    constexpr FileNo FNO(int fileno)  {
        return FileNo(fileno);
    }
    */

    class PipeException
        : public std::runtime_error
    {
    public:
        PipeException(const char*        reason) : std::runtime_error(reason) {}
        PipeException(const std::string& reason) : std::runtime_error(reason) {}
    };

    std::pair<int, int> _pipe_throw() {
        int filed[2];
        if (::pipe(filed) != 0) {
            throw PipeException("failed to create pipe");
        }
        return std::pair<int, int>(filed[0], filed[1]);
    }

    void _dup2_ordie(int old_fd, int new_fd) {
        if (::dup2(old_fd, new_fd) != new_fd) {
            ::exit(-1);
        }
    }

    class Coprocess {
      
      public:

        Coprocess() : _id(0) {}
        Coprocess(const Coprocess&) = delete;
        Coprocess& operator= (const Coprocess&) = delete;
        
        Coprocess(Coprocess&& that) {
            _id = that._id; that._id = 0;
        }
        Coprocess& operator= (Coprocess&& rhs) {
            _id = rhs._id; rhs._id = 0;
            return *this;
        }

        Coprocess(
            const std::string&                          command,
            std::initializer_list<redirect_clause_t>    redirection = {},
            std::initializer_list<const char*>          environs    = {}
        )
            : Coprocess("/bin/sh", { "sh", "-c", command.c_str() },
                        redirection, environs) 
        {
        }


        Coprocess(
            const char*                                 command,
            std::initializer_list<redirect_clause_t>    redirection = {},
            std::initializer_list<const char*>          environs    = {}
        )
            : Coprocess("/bin/sh", { "sh", "-c", command }, 
                        redirection, environs) 
        {
        }

        Coprocess(
            const char*                                 filename,
            std::initializer_list<const char*>          arguments,
            std::initializer_list<redirect_clause_t>    redirection,
            std::initializer_list<const char*>          environs
        ) 
        {
            using namespace std;
    
            vector<pair<int, int>> pipes;

            for (const redirect_clause_t& r: redirection) {
                pipes.push_back(_pipe_throw());
            }

            pid_t pid = vfork();

            if (pid == 0) {
                int n = 0;
                for (const redirect_clause_t& r : redirection) {
                    switch (r.method) {
                        case redirect_method::input :
                            ::close(pipes[n].second);
                            _dup2_ordie(pipes[n].first, r.coprocess_fileno);
                            break;

                        case redirect_method::output :
                            ::close(pipes[n].first);
                            _dup2_ordie(pipes[n].second, r.coprocess_fileno);
                            break;
                    }

                    n += 1;
                }

                vector<const char*> args(std::begin(arguments), std::end(arguments));
                args.push_back(nullptr);
                
                vector<const char*> envs(std::begin(environs), std::end(environs));
                for (char** e = environ; ; ++e) {
                    envs.push_back(*e);
                    if (*e == nullptr) break;
                }

                ::execve(filename, const_cast<char* const*>(args.data()), 
                                   const_cast<char* const*>(envs.data()) );
                ::exit(-1);
            } else {
                int n = 0;
                for (const redirect_clause_t& r : redirection) {
                    switch (r.method) {
                        case redirect_method::output :
                            ::close(pipes[n].second);
                            r.file = Stream::PosixFile(pipes[n].first);
                            break;

                        case redirect_method::input :
                            ::close(pipes[n].first);
                            r.file = Stream::PosixFile(pipes[n].second);
                            break;
                    }

                    n += 1;
                }

                _id = pid;
             }
        }

        int wait() {
            assert(_id > 0);

            int status;
            ::waitpid(_id, &status, 0);

            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return -1;
            }
        }

        void kill(int sig) {
            assert(_id > 0);

            ::kill(_id, sig);
        }

      private:
        pid_t _id;
    };

}
