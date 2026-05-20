#include <sys/epoll.h>

#include "../includes/server.hpp"
#include "common/log.hpp"

SFS::Server::Server() : jobs(), worker_threads(), listening_socket(nullptr), conns(), event_handler(), http_handler() {
}

void SFS::Server::get(std::string route, HandlerFunc func) {
    std::string route_copy = route;
    SFS::HttpHandlerStatus status = this->http_handler.addRoute(SFS::HttpMethod::GET, std::move(route), std::move(func));

    if (status == SFS::HttpHandlerStatus::EXISTS) {
        SFS::log(SFS::LogLevel::WARNING, std::string("The registration of the GET Method with Route ") + route_copy + std::string(" did not work. Most likely, the route is registered alredy!"));
    }
}

void SFS::Server::start(uint16_t port, size_t num_workers = 0) {
    try {
        this->listening_socket = std::make_unique<SFS::Socket>(port);
    } catch (std::runtime_error &error) {
        SFS::log(SFS::LogLevel::ERROR, std::string("The SimpleWebServer could not be started!"));
        return;
    }

    this->event_handler.add(this->listening_socket->get_fd(), EPOLLIN); // registers listening socket for EPOLLIN

    size_t actual_concurrency = this->calculate_thread_number(num_workers);

    for (size_t i = 0; i < actual_concurrency; ++i) {
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
                handle_socket_events(fd, event_mask);
            } else {
                handle_connection_event(fd, event_mask);
            }
        }
    }
}

// TODO: Implement
void SFS::Server::worker_thread_loop() {
    while(true) {}
}

// TODO: Implement
void SFS::Server::handle_socket_events(int fd, uint32_t event_mask) {
    if (event_mask  )
    return;
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

    switch (status) {
        case SFS::ConnectionStatus::ERROR:
            this->event_handler.remove(fd);
            this->conns.erase(fd);
            return;
        
        case SFS::ConnectionStatus::WANT_WRITE:
            this->event_handler.edit(fd, EPOLLIN | EPOLLOUT);
            break;

        case SFS::ConnectionStatus::COMPLETE:
            this->event_handler.edit(fd, EPOLLIN);
            break;

        default:
            this->event_handler.edit(fd, EPOLLIN | EPOLLOUT);
            break;
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

size_t SFS::Server::calculate_thread_number(size_t num_workers) {
    size_t min_concurrency = std::thread::hardware_concurrency();

    if (num_workers == 0) {
        return min_concurrency * SFS::Server::THREAD_MULT;
    }
        
    if (num_workers > min_concurrency) {
        return num_workers;
    }

    return min_concurrency;
}