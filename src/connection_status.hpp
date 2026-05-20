#ifndef connection_status_hpp
#define connection_status_hpp

namespace SFS {
    enum class ConnectionStatus {
        // status codes for receive operation
        OPEN,
        CLOSED,
        PAYLOAD_TOO_LARGE,

        // status code for send operation
        WANT_WRITE,
        COMPLETE,

        // shared between both operations
        ERROR,

        // When the socket is trying to accept but there is no connection to accept
        WAITING
    };
}

#endif /* connection_status_hpp */
