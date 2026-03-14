#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

std::atomic<int> success_count(0);

void send_requests(int num_requests) {
    for (int i = 0; i < num_requests; ++i) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(1234);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
            std::string msg = "SET key" + std::to_string(i) + " benchmark_data";
            write(fd, msg.c_str(), msg.length());
            
            char rbuf[128] = {};
            if (read(fd, rbuf, sizeof(rbuf) - 1) > 0) {
                success_count++;
            }
        }
        close(fd);
    }
}

int main() {
    const int NUM_CLIENTS = 10;
    const int REQUESTS_PER_CLIENT = 1000;
    
    std::cout << "Starting benchmark: " << (NUM_CLIENTS * REQUESTS_PER_CLIENT) << " requests...\n";

    
    auto start_time = std::chrono::high_resolution_clock::now();

    
    std::vector<std::thread> clients;
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        clients.emplace_back(send_requests, REQUESTS_PER_CLIENT);
    }

    
    for (auto& client : clients) {
        client.join();
    }

    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    double rps = success_count / elapsed.count();

    std::cout << "====================================\n";
    std::cout << "Benchmark Complete!\n";
    std::cout << "Successful Requests: " << success_count << "\n";
    std::cout << "Time Elapsed: " << elapsed.count() << " seconds\n";
    std::cout << "Throughput: " << rps << " Requests Per Second (RPS)\n";
    std::cout << "====================================\n";

    return 0;
}