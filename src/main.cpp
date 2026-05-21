#include "server.hpp"
#include "common/log.hpp"

int main(int argc, char *argv[]) {
    if (argc > 4) {
        SFS::log(SFS::LogLevel::ERROR, "Too many arguments passed! Usage: ./SimpleFileServer <port> <base_dir> <num_workers (optional)>");
        return EXIT_FAILURE;
    }

    if (argc < 3) {
        SFS::log(SFS::LogLevel::ERROR, "Too few arguments passed! Usage: ./SimpleFileServer <port> <base_dir> <num_workers (optional)>");
        return EXIT_FAILURE;
    }

    uint16_t port = static_cast<uint16_t>(std::stoi(argv[1]));
    std::string base_dir = argv[2];
    std::size_t num_workers = argc == 4 ? static_cast<std::size_t>(std::stoi(argv[3])) : 0;

    SFS::Server server(base_dir);
    server.start(port, num_workers);
    return EXIT_SUCCESS;
}