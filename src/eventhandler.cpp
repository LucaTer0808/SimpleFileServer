#include <sys/epoll.h>
#include <stdexcept>
#include <array>
#include <unistd.h>

#include "eventhandler.hpp"
#include "common/log.hpp"

SFS::EventHandler::EventHandler() : epoll_fd(epoll_create1(EPOLL_CLOEXEC)) {
    if (epoll_fd == -1) {
        SFS::log_errno("Creating an epoll instance has failed! Event polling is not possible!");
        throw std::runtime_error("epoll_create1(EPOLL_CLOEXEC) failed!");
    }
}

SFS::EventHandler::~EventHandler() {
    this->close();
}

SFS::EventHandler::EventHandler(EventHandler&& other) noexcept {
    this->epoll_fd = other.epoll_fd;
    other.epoll_fd = -1;   
}

SFS::EventHandler& SFS::EventHandler::operator=(EventHandler&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    this->close();

    this->epoll_fd = other.epoll_fd;
    other.epoll_fd = -1;

    return *this;
}

std::unordered_map<int, uint32_t> SFS::EventHandler::wait_events() {
    std::array<epoll_event, SFS::EventHandler::MAX_EVENTS> events;
    std::unordered_map<int, uint32_t> result;

    int epoll_result = epoll_wait(this->epoll_fd, events.data(), SFS::EventHandler::MAX_EVENTS, NO_TIMEOUT);

    if (epoll_result < 0) {
        SFS::log_errno("Failed polling events!");
        return result; // should be empty at this point!
    }

    for (int i = 0; i < epoll_result; ++i) {
        const epoll_event &event = events.at(i);

        int fd = event.data.fd;
        uint32_t ev_mask = event.events;

        result.emplace(fd, ev_mask);
    }

    return result;
}

bool SFS::EventHandler::add(int fd, uint32_t events) {
    if (fd < 0) {
        SFS::log(SFS::LogLevel::WARNING, std::string("Negative fd passed to the EventHandler. No Socket can be added!"));
        return false;
    }

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    int result = epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    if (result < 0) {
        SFS::log_errno(std::string("Adding the socket with fd: ") + std::to_string(fd) + std::string(" for event polling failed!"));
        return false;
    }

    return true;
}

bool SFS::EventHandler::edit(int fd, uint32_t events) {
    if (fd < 0) {
        SFS::log(SFS::LogLevel::WARNING, std::string("Negative fd passed to the EventHandler. No Socket can be edited!"));
        return false;
    }

    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;

    int result = epoll_ctl(this->epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    if (result < 0) {
        SFS::log_errno(std::string("Editing the socket with fd: ") + std::to_string(fd) + std::string(" failed!"));
        return false;
    }

    return true;
}

bool SFS::EventHandler::remove(int fd) {
    if (fd < 0) {
        SFS::log(SFS::LogLevel::WARNING, std::string("Negative fd passed to the EventHandler. No Socket can be removed!"));
        return false;
    }

    int result = epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

    if (result < 0) {
        SFS::log_errno(std::string("Removing the event polling for socket with fd: ") + std::to_string(fd) + std::string(" has failed!"));
        return false;
    }

    return true;
}

void SFS::EventHandler::close() {
    if (this->epoll_fd >= 0) {
        ::close(epoll_fd);
        this->epoll_fd = -1;
    }
}