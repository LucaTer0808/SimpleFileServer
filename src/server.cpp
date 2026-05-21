#include <sys/epoll.h>
#include <fstream>
#include <sstream>

#include "server.hpp"
#include "common/log.hpp"

SFS::Server::Server(std::string path) : base_dir(std::filesystem::canonical(path)), jobs(), worker_threads(), listening_socket(nullptr), conns(), event_handler() {
}

void SFS::Server::start(uint16_t port, std::size_t num_workers) {
    try {
        this->listening_socket = std::make_unique<SFS::Socket>(port);
    } catch (std::runtime_error &error) {
        SFS::log(SFS::LogLevel::ERROR, std::string("The SimpleWebServer could not be started!"));
        return;
    }

    this->event_handler.add(this->listening_socket->get_fd(), EPOLLIN); // registers listening socket for EPOLLIN

    std::size_t actual_concurrency = this->calculate_thread_number(num_workers);

    for (std::size_t i = 0; i < actual_concurrency; ++i) {
        this->worker_threads.emplace_back([this]() {
            this->worker_thread_loop();
        });
    }

    SFS::log(SFS::LogLevel::INFO, std::string("SimpleWebServer is now listening on port ") + std::to_string(port));
    this->master_thread_loop();
}

void SFS::Server::master_thread_loop() {
    while(true) {
        std::unordered_map<int, uint32_t> events = this->event_handler.wait_events();

        for (auto& [fd, event_mask] : events) {
            if (fd == this->listening_socket->get_fd()) {
                this->handle_socket_events(event_mask);
            } else {
                this->handle_connection_event(fd, event_mask);
            }
        }
    }
}

void SFS::Server::worker_thread_loop() {
    while(true) {
        Job jobToProcess;
        this->jobs.pop_and_block(jobToProcess);

        std::string path = std::move(jobToProcess.request);

        if (!path.empty() && path.front() == '/') {
            jobToProcess.promise.set_value("The path you passed is invalid! It must not start with '/' to prevent directory traversal attacks!");
            continue;
        }

        if (!path.empty() && path.back() == '/') {
            jobToProcess.promise.set_value("The path you passed is invalid! It must not end with '/' to prevent directory traversal attacks!");
            continue;
        }

        if (path.find("..") != std::string::npos) {
            jobToProcess.promise.set_value("The path you passed is invalid! It must not contain '..' to prevent directory traversal attacks!");
            continue;
        }

        std::filesystem::path base_dir = this->get_base_dir();
        std::filesystem::path full_path = base_dir / path;

        std::ifstream file(full_path, std::ios::binary);
        if (!file) {
            jobToProcess.promise.set_value("The requested file could not be found on the server!");
            continue;
        }
        

        std::stringstream buffer;
        buffer << file.rdbuf();
        jobToProcess.promise.set_value(buffer.str());
    }
}

// TODO: Implement
void SFS::Server::handle_socket_events(uint32_t event_mask) {
    if (event_mask & EPOLLIN) {
        std::vector<std::unique_ptr<SFS::Connection>> new_conns = this->listening_socket->accept();

        for (std::unique_ptr<SFS::Connection>& conn : new_conns) {
            int conn_fd = conn->get_client_fd();
            this->event_handler.add(conn_fd, EPOLLIN);
            this->conns.emplace(conn_fd, std::move(conn));
        }
    }
}

void SFS::Server::handle_connection_event(int fd, uint32_t event_mask) {
    auto it = this->conns.find(fd);
    if (it == this->conns.end()) {
        this->event_handler.remove(fd);
        SFS::log(SFS::LogLevel::WARNING, std::format("The fd: {} does not represent an active connection!", fd));
        return;
    }

    SFS::Connection& conn = *(it->second);

    if (event_mask & EPOLLIN) {
        bool correct = conn.receive();

        if (!correct) {
            this->event_handler.remove(fd);
            this->conns.erase(fd);
            return;
        }

        std::string request = conn.get_latest_request();

        if (!request.empty()) {
            this->append_job(conn, std::move(request));
        }
    }

    SFS::ConnectionStatus status = conn.try_serve_future();
    if (event_mask & EPOLLOUT) {
        status = conn.push_data();
    }

    bool event_handler_success;

    switch (status) {
        case SFS::ConnectionStatus::ERROR:
            event_handler_success = this->event_handler.remove(fd);
            this->conns.erase(fd);
            return;
        
        case SFS::ConnectionStatus::WANT_WRITE:
            event_handler_success = this->event_handler.edit(fd, EPOLLIN | EPOLLOUT);
            break;

        case SFS::ConnectionStatus::COMPLETE:
            event_handler_success = this->event_handler.edit(fd, EPOLLIN);
            break;

        default:
            event_handler_success = this->event_handler.edit(fd, EPOLLIN | EPOLLOUT);
            break;
    }

    if (!event_handler_success) {
        this->event_handler.remove(fd);
        this->conns.erase(fd);
    }
}

void SFS::Server::append_job(SFS::Connection& conn, std::string request_string) {
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    conn.enqueue_future(std::move(future));

    SFS::Job job;
    job.request = std::move(request_string);
    job.promise = std::move(promise);

    this->jobs.push(std::move(job));
}

std::size_t SFS::Server::calculate_thread_number(std::size_t num_workers) {
    std::size_t min_concurrency = std::thread::hardware_concurrency();

    if (num_workers == 0) {
        return min_concurrency * SFS::Server::THREAD_MULT;
    }
        
    if (num_workers > min_concurrency) {
        return num_workers;
    }

    return min_concurrency;
}

std::filesystem::path SFS::Server::get_base_dir() const {
    return this->base_dir;
}