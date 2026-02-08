#include "test_utils.hpp"
#include <cstring>
#include <random>

bool binary_arr_eq(std::byte* arr1, std::byte* arr2, std::size_t size) {
    for (std::size_t i = 0; i < size; i++) {
        if (arr1[i] != arr2[i]) {
            return false;
        }
    }
    return true;
}

std::unique_ptr<std::byte[]> generate_random_bytes(std::size_t size) {
    auto buffer = std::make_unique<std::byte[]>(size);

    // Thread-local to avoid reseeding costs if used frequently
    thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<uint8_t> dist(0, 255);

    for (std::size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<std::byte>(dist(rng));
    }

    return buffer;
}

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

FILE* get_command_pipe(const std::string& command, const std::string& mode) {
    std::vector<char> output;
    const int buffer_size = 128;
    char buffer[buffer_size];

    FILE* pipe = popen(command.c_str(), mode.c_str());
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    return pipe;
}

