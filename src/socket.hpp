#ifndef socket_hpp
#define socket_hpp

#include <stdio.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>

#include "connection.hpp"
#include "connection_status.hpp"

namespace SFS { /* SimpleWebServer */
    class Socket {
        public:
            /**
             * Constructor for the creation of a socket. Creates the socket, binds it to the given port and listens on that port.
             */
            Socket(const uint16_t port);

            /**
             * Descturctor for an existing socket. Closes all open connections and also closes the listening socket itself.
             */
            ~Socket();

            /**
             * Copy constructor is forbidden. Socket is a move-only type.
             */
            Socket(const Socket&) = delete;
            Socket& operator=(const Socket&) = delete;

            /**
             * Declaration for move constructor.
             */
            Socket(Socket&& other) noexcept;

            /**
             * Declaration of move assignment operator.
             */
            Socket& operator=(Socket&& other) noexcept;

            /**
             * Accepts all new connections in the accept buffer, collects them and returns them as a vector for further processing
             * @return A vector of unique_ptr<SFS::Connection> containing all successfully established connections.
             */
            std::vector<std::unique_ptr<SFS::Connection>> accept();

            /**
             * Returns the file descriptor of the listening socket instance.
             */
            int get_fd() const;

            /**
             * Closes the socket
             */
            void close();
            
        private:
            int socket_fd;
    };
}

#endif /* socket_hpp*/