#pragma once /* eventhandler_hpp */

#include <unordered_map>

namespace SFS {
    class EventHandler {
        public:
            static constexpr int MAX_EVENTS = 128;
            static constexpr int NO_TIMEOUT = -1;

            /**
             * Constructor for an EventHandler;
             */
            EventHandler();

            /**
             * Desctructor for an EventHandler;
             */
            ~EventHandler();

            /**
             * Since we use RAII, copying an EventHandler is strictly forbidden.
             */
            EventHandler(const EventHandler&) = delete;
            EventHandler operator=(const EventHandler&) = delete;

            /**
             * @brief Move constructor for an EventHandler.
             * @param other The temporary EventHandler to moveconstruct from.
             */
            EventHandler(EventHandler&& other) noexcept;

            /**
             * @brief Move operator for an EventHandler.
             * @param other The EventHandler to move from.
             */
            EventHandler& operator=(EventHandler&& other) noexcept;

            /**
             * @brief Collects events for events and returns the events for each file descriptor. See the static constexpr above, how many events are polled at once.
             * @return An unordered map with the fd as key and a bitmap representing the events as value.
             */
            std::unordered_map<int, uint32_t> wait_events();

            /**
             * @brief Add a file descriptor representing a socket for event polling.
             * @param fd The file descriptor in question.
             * @param events The events connected by |.
             * @return true, when the operation was successful, false otherwise.
             */
            bool add(int fd, uint32_t events);

            /**
             * @brief Editing the events polled for the socket with the given fd.
             * @param fd The file descriptor in question.
             * @param events The new events connected with |.
             * @return true, when the operation was successful, false otherwise.
             */
            bool edit(int fd, uint32_t events);

            /**
             * @brief Removes a file descriptor representing a socket from event polling, e.g. when it is closed.
             * @param fd The file descriptor representing the socket.
             * @return true, when the operation was successful, false otherwise.
             */
            bool remove(int fd);
        
        private:
            int epoll_fd;

            /**
             * Closes the epoll instance represented by the file descriptor.
             */
            void close();
    };
}