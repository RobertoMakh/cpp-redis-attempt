#include <iostream>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sstream>
//threads:
#include <queue>
#include <thread>
#include <vector>
//ds and locks:
#include <list>
#include <unordered_map>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <string>

class KVStore {
private:
    int capacity;
    std::list<std::string> lru_list;
    
    std::unordered_map<std::string, std::pair<std::string, std::list<std::string>::iterator>> store;
    
    std::mutex db_lock;

public:
    KVStore(int cap = 1000) : capacity(cap) {} 

    void set(const std::string& key, const std::string& value) {
        std::unique_lock<std::mutex> lock(db_lock);
        
        auto it = store.find(key);
        if (it != store.end()) {
            it->second.first = value;
            lru_list.erase(it->second.second);
            lru_list.push_front(key);
            it->second.second = lru_list.begin();
        } else {
            if (store.size() >= capacity) {
                std::string oldest_key = lru_list.back();
                lru_list.pop_back();
                store.erase(oldest_key);
            }
            lru_list.push_front(key);
            store[key] = {value, lru_list.begin()};
        }
    }

    std::string get(const std::string& key) {
        std::unique_lock<std::mutex> lock(db_lock);
        
        auto it = store.find(key);
        if (it != store.end()) {
            lru_list.erase(it->second.second);
            lru_list.push_front(key);
            it->second.second = lru_list.begin();
            return it->second.first;
        }
        return "Key not found";
    }
};

KVStore db;

void do_something(int connfd) {
    char rbuf[1024] = {}; 
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n <= 0) {
        if (n < 0) perror("read() failed");
        return;
    }

    
    std::string request(rbuf);
    std::istringstream iss(request);
    std::string command, key, value;

    iss >> command; 

    std::string response;

    if (command == "SET") {
        iss >> key;
        
        std::getline(iss >> std::ws, value); 
        
        if (key.empty() || value.empty()) {
            response = "ERR: Usage SET <key> <value>\n";
        } else {
            db.set(key, value);
            response = "Done\n";
        }
    } 
    else if (command == "GET") {
        iss >> key;
        if (key.empty()) {
            response = "ERR: Usage GET <key>\n";
        } else {
            response = db.get(key) + "\n";
        }
    } 
    else {
        response = "ERR: Unknown command\n";
    }

    write(connfd, response.c_str(), response.length());
}




std::queue<int> client_queue;
std::mutex queue_mutex;
std::condition_variable condition;


void worker_thread() {
    while (true) {
        int connfd = -1;
        { 
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            while (client_queue.empty()) {
                condition.wait(lock);
            }
            
            connfd = client_queue.front();
            client_queue.pop();
        } 
        
        do_something(connfd);
        close(connfd);
    }
}

int main(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (const struct sockaddr *)&addr,sizeof(addr));

    rv = listen(fd, SOMAXCONN);
    const int NUM_THREADS = 4;
    std::vector<std::thread> pool;
    for (int i = 0; i < NUM_THREADS; ++i) {
        pool.emplace_back(worker_thread);
    }

    while(1){
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if(connfd < 0) continue;
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            client_queue.push(connfd);
        }
        
        condition.notify_one();
    }
}