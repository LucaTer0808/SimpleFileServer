#ifndef connection_hpp
#define connection_hpp

#include <string>
#include <future>
#include <queue>

#include "connection_status.hpp"

namespace SFS {
    class Connection {
        private:
            static constexpr int MAXIMUM_BUFFER_SIZE = 64 * 1024; // 64 kb

            int client_fd = -1;
            std::string buffer_in;
            std::string buffer_out;

            std::queue<std::future<std::string>> responses;

            /**
             * @brief Checks if the first future in the responses queue has already been completed.
             * @return true, if the future is complete. false otherwise.
             */
            bool is_future_complete() const;

            /**
             * Appends data to be send to the output puffer and immediately tries to push it through the channel.
             * @param data The data to be appended to the output puffer.
             * @return WANT_WRITE, when there still is data in the puffer to be pushed through the socket after the first initial send.
             * @return COMPLETE, when all data could be pushed through the socket and nothing is left to do.
             * @return ERROR; when something unusual happened that demands the closing of the connection.
             */
            SFS::ConnectionStatus send(const std::string& data);

            /**
             * @brief Finds the index of the beginning of the \r\n\r\n sequence that indicates the end of a header.
             * @return The index as std::size_t or std::string::npos, if no substring with said sequence was found.
             */
            std::size_t find_header_ending() const;

            /**
             * Closing the connection to the socket.
             */
            void close();

        public:
            /**
             * Constructor for a connection object. Note, that due to non-blocking purposes, the fd has
             * to identify an existing client socket already!
             * @param client_fd The file descriptor identifiying the connection.
             */
            Connection(const int client_fd);

            /**
             * Desctructor for a connection object.
             */
            ~Connection();

            /**
             * Forbids copying the connection object
             */
            Connection(const Connection&) = delete;

            /**
             * Forbids the copying operator aswell.
             */
            Connection& operator=(const Connection&) = delete;

            /**
             * Explicit declaration of the move operation.
             * @param other The temporary object to move from.
             */
            Connection(Connection&& other) noexcept;
            
            /**
             * Explicit declaration of the move operator.
             * @param other The object to move assign from.
             */
            Connection& operator=(Connection&& other) noexcept;

            /**
             * Enqueues a new string feature that is filled automatically when a worker thread finished the job.
             */
            void enqueue_future(std::future<std::string> future);

            /**
             * Tries to serve a future in the queue. Checks if the first future is complete and if so
             * turns it into a std::string and internally calls the send operation on it.
             * @return WANT_WRITE, when the buffer still contains data to be pushed through.
             * @return COMPLETE, when the whole content from the buffer could be pushed through the channel or there is no more 
             * @return ERROR, when an error occured while pushing data through the socket.
             * @return CLOSED, when the connection was closed.
             */
            SFS::ConnectionStatus try_serve_future();

            /**
             * @brief Tries to push the data from the output buffer through the channel to the client socket.
             * @return WANT_WRITE, when the buffer still contains data to be pushed through.
             * @return COMPLETE, when the whole content from the buffer could be pushed through the channel.
             * @return ERROR, when an error occured while pushing data through the socket.
             * @return CLOSED, when the connection was closed.
             */
            SFS::ConnectionStatus push_data();
            
            /**
             * @brief Reveives data from the client socket and writes it to the input buffer.
             * @return true, if the receiving process finished without any issue.
             * @return false, if an error occured and the connection has to be closed.
             */
            bool receive();

            /**
             * @brief Removes the latest full request from the buffer and returns it.
             * @return The whole request as a string or "", if the buffer does not contain a full request.
             */
            std::string get_latest_request();
    };
}

#endif /* connection_hpp */