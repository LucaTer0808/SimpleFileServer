#include<stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdexcept>
#include <array>
#include <fcntl.h>

#include "connection.hpp"
#include "common/log.hpp"

SFS::Connection::Connection(const int client_fd) : client_fd(client_fd), buffer_in(""), buffer_out(""), responses() {
    if (client_fd < 0) {
        this->close();
        SFS::log(SFS::LogLevel::ERROR, "Invalid client file descriptor! It can't be negative. FD: " + std::to_string(client_fd));
        throw std::invalid_argument("Negative file descriptor passed to the connection constructor! It can not represent a valid client socket!");
    }

    int flags = fcntl(client_fd, F_GETFL, 0);

    if (flags == -1) {
        this->close();
        SFS::log_errno("Could not get flags for FD: " + std::to_string(client_fd));
        throw std::runtime_error("fcntl F_GETFL failed");
    }

    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        this->close();
        SFS::log_errno("Could not set O_NONBLOCK for FD: " + std::to_string(client_fd));
        throw std::runtime_error("fcntl F_SETFL failed");
    }
    
    SFS::log(SFS::LogLevel::INFO, "Connection to client socket established and set to non-blocking with FD: " + std::to_string(this->client_fd));
}

SFS::Connection::~Connection() {
    this->close();
}

SFS::Connection::Connection(Connection&& other) noexcept : client_fd(other.client_fd), buffer_in(std::move(other.buffer_in)), buffer_out(std::move(other.buffer_out)), responses(std::move(other.responses)) {
    other.client_fd = -1;
}

SFS::Connection& SFS::Connection::operator=(Connection&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->close();

    this->client_fd = other.client_fd;
    other.client_fd = -1;
    this->buffer_in = std::move(other.buffer_in);
    this->buffer_out = std::move(other.buffer_out);
    this->responses = std::move(other.responses);

    return *this;
}

SFS::ConnectionStatus SFS::Connection::send(const std::string& data) {
    this->buffer_out.append(data);
    return this->push_data();
}

SFS::ConnectionStatus SFS::Connection::push_data() {
    while(!this->buffer_out.empty()) {
        ssize_t sent = ::send(this->client_fd,
            this->buffer_out.data(),
            this->buffer_out.size(),
            MSG_NOSIGNAL
        );

        if (sent < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                SFS::log_errno("Failed to send data to the client with FD: " + std::to_string(this->client_fd));
                return SFS::ConnectionStatus::ERROR;  // something else went wrong! This is not good!
            } else {
                return SFS::ConnectionStatus::WANT_WRITE; // send() buffer is most likey full, we need to send again once its free again!
            }
        }

        if (sent == 0) {
            SFS::log(SFS::LogLevel::INFO, "No active connection to client with FD: " + std::to_string(this->client_fd));
            return SFS::ConnectionStatus::ERROR;
        }

        this->buffer_out.erase(0, static_cast<size_t>(sent));
    }

    return SFS::ConnectionStatus::COMPLETE; // if the send buffer is empty, we can happily exit!
}

bool SFS::Connection::receive() {
    std::array<char, 4096> buffer;

    while(true) {
        ssize_t bytes_received = ::recv(this->client_fd, buffer.data(), buffer.size(), 0);

        if (bytes_received < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                SFS::log_errno("Failed to receive data from client with FD: " + std::to_string(this->client_fd));
                return false;
            } else {
                return true; // if the recv() buffer was cleared properly
            }
        }

        if (bytes_received == 0) {
            SFS::log(SFS::LogLevel::INFO, "No active connection to client with FD: " + std::to_string(this->client_fd));
            return false; // recv() only returns 0, when the connection has been closed!
        }

        this->buffer_in.append(buffer.data(), bytes_received);

        if (this->buffer_in.size() > MAXIMUM_BUFFER_SIZE) { // value is pickd arbitrarily. Can be changed if needed!
            SFS::log(SFS::LogLevel::WARNING, "Request size exceeded the healthy limit of " + std::to_string(MAXIMUM_BUFFER_SIZE / 1024) + " kb on client socket with FD: " + std::to_string(this->client_fd));  
            return false;
        }
    }
}

std::string SFS::Connection::get_latest_request() { 
    size_t pos = this->find_request_ending();

    if (pos == std::string::npos) {
        return "";
    }

    size_t total_len = pos + 4; // \r and \n are accounted for as one byte!

    std::string request = this->buffer_in.substr(0, total_len);
    this->buffer_in.erase(0, total_len);
    return request;
}

SFS::ConnectionStatus SFS::Connection::try_serve_future() {
    if (!this->is_future_complete()) {
        return SFS::ConnectionStatus::WAITING;
    }

    std::string response = this->responses.front().get();
    this->responses.pop();

    return this->send(response);
}

bool SFS::Connection::is_future_complete() const {
    if (this->responses.empty()) {
        return false;
    }

    if (this->responses.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        return true;
    }

    return false;
}

void SFS::Connection::enqueue_future(std::future<std::string> future) {
    this->responses.push(std::move(future));
}

void SFS::Connection::close() {
    if (this->client_fd >= 0) {
        ::close(this->client_fd);
        SFS::log(SFS::LogLevel::INFO, std::string("Client socket closed with FD: ") + std::to_string(this->client_fd));
        this->client_fd = -1;
    }
}

size_t SFS::Connection::find_request_ending() const {
    return this->buffer_in.find("\r\n\r\n");
}
