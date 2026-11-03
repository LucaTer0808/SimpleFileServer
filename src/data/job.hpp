#ifndef job_hpp
#define job_hpp

#include <future>

namespace SFS {
    /**
     * Data class representing a job composed of a promise to fullfill and a request string.
     */
    struct Job {
        std::promise<std::string> promise;
        std::string request;
        int client_fd = -1;
    };
}

#endif /* job_hpp */