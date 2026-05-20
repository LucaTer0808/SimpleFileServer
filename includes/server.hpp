#ifndef server_hpp
#define server_hpp

#include <unordered_map>
#include <memory>
#include <functional>
#include <queue>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>
#include <filesystem>

#include "socket.hpp"
#include "data/job.hpp"
#include "eventhandler.hpp"
#include "utils/thread_safe_queue.hpp"

namespace SFS {
    class Server {
        private:
            std::filesystem::path base_dir;

            SFS::ThreadSafeQueue<SFS::Job> jobs;

            std::vector<std::jthread> worker_threads;

            std::unique_ptr<SFS::Socket> listening_socket; // to let it also be null
            SFS::EventHandler event_handler;
            std::unordered_map<int, std::unique_ptr<SFS::Connection>> conns;

        public:
            static constexpr std::size_t THREAD_MULT = 3;
    
            /**
             * @brief Constructor for a Server object. Since all other objects are constructed via the basic constructor,
             * we may just use the default constructor here.
             * @param path The path to the directory containing the files to serve. This is used to calculate the full path of a requested file.
             */
            Server(std::string path);

            /**
             * @brief Desctructor for an existing Server object. We can just use the default destructor.
             */
            ~Server() = default;

            /**
             * Both the copy instructor and the copy assing operator are disabled!.
             */
            Server(const Server&) = delete;
            Server& operator=(const Server&) = delete;

            /**
             * @brief Declaration of the move constructor. Just use the default move constructor.
             * @param other The temporary Server object to move from.
             */
            Server(Server&& other) = default;

            /**
             * @brief Declaration of move operator. Just use the default move operator.
             * @param other The Server object to move from.
             */
            Server& operator=(Server&& other) = default;

            /**
             * Appends a new job to the given connection. It will contain the result of the request passed as a string.
             * @param conn The connection to append the job to.
             * @param request_string The request as a string to serve.
             */
            void append_job(SFS::Connection& conn, std::string request_string);

            void handle_socket_events(int fd, uint32_t event_mask);

            /**
             * Handles all event related logic for the connection with the given fd, including subscription for EPOLLOUT, job generation
             * and response sending logic. The actual work will be done in handle_socket_events.
             * @param fd The file descriptor representing said connection object.
             * @param event_mask The bitmask representing all incoming events.
             */
            void handle_connection_event(int fd, uint32_t event_mask);

        public:
            /**
             * @brief Start up the server. Before starting, all routes have to be added to ensure functionality!
             * @param port The port to listen on.
             * @param num_workers The amount of worker threads for increased concurrency. If none is passed, we multiply the value
             * std::thread::hardware_concurrency delivers by a fixed constant. std::thread::hardware_concurrency also works as the lower bound
             * for the amunt of threads running to at least make use of all cores. 0 by deafult.
             */
            void start(uint16_t port, std::size_t num_workers = 0);

            /**
             * @brief The main loop for the master thread responsible for managing connections and distributing orders.
             */
            void master_thread_loop();

            /**
             * @brief The main loop for any worker thread responsible for actually serving requests. It keeps checking if the queue is not empty
             * and continues to consume and serve requests until it is empty. It then makes the thread wait on the condition variable until it is needed again.
             */
            void worker_thread_loop();

            /**
             * Calculates the required number of threads as stated in the start()-method;
             * @param num_workers The desired number of worker threads or 0 if left basic.
             */
            std::size_t calculate_thread_number(std::size_t num_workers);

            /**
             * @returns The path of the directory containing the files to serve. This is used to calculate the full path of a requested file.
             */
            std::filesystem::path get_base_dir();
    };
}

#endif /* server_hpp */