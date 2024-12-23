#include <iostream>
#include <queue>
#include <functional>
#include <future>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <random>
#include <sstream>

// Queue Class
class Queue {
private:
    struct Task {
        std::string id;
        std::function<void()> task;
        std::promise<void> promise;
    };

    std::queue<Task> tasks;
    bool isBusy = false;
    std::mutex mtx;
    std::condition_variable cv;

    // Generate a random ID
    std::string generateId() {
        std::ostringstream oss;
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<int> dis(0, 15);
        for (int i = 0; i < 20; ++i) {
            oss << std::hex << dis(gen);
        }
        return oss.str();
    }

    // Process the next task
    void next() {
        std::unique_lock<std::mutex> lock(mtx);
        if (tasks.empty()) {
            isBusy = false;
            return;
        }

        isBusy = true;
        Task currentTask = std::move(tasks.front());
        tasks.pop();

        lock.unlock();  // Unlock during task execution
        try {
            currentTask.task();
            currentTask.promise.set_value();
        } catch (...) {
            currentTask.promise.set_exception(std::current_exception());
        }

        lock.lock();
        isBusy = false;
        cv.notify_one();
        lock.unlock();
        next();
    }

public:
    // Add a task to the queue
    std::future<void> add(std::function<void()> task) {
        if (!task) {
            throw std::invalid_argument("Task is not a valid function");
        }

        Task newTask;
        newTask.id = generateId();
        newTask.task = task;
        auto future = newTask.promise.get_future();

        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(std::move(newTask));

        if (!isBusy) {
            std::thread(&Queue::next, this).detach();
        }

        return future;
    }

    // Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        while (!tasks.empty()) {
            tasks.pop();
        }
    }

    // Get the size of the queue
    size_t size() {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.size();
    }

    // Check if the queue is busy
    bool isBusyStatus() {
        std::lock_guard<std::mutex> lock(mtx);
        return isBusy;
    }
};

// Server Structure
struct Server {
    std::string address;
    std::string credentials;
    Queue queue;

    Server(const std::string& addr, const std::string& creds) : address(addr), credentials(creds) {}
};

int main() {
    // Define servers
    std::vector<Server> servers = {
        {"http://127.0.0.1:7860", "username:password"}
    };

    // Add tasks to the first server's queue
    servers[0].queue.add([]() {
        std::cout << "Task 1 is running..." << std::endl;
    });

    servers[0].queue.add([]() {
        std::cout << "Task 2 is running..." << std::endl;
    });

    servers[0].queue.add([]() {
        std::cout << "Task 3 is running..." << std::endl;
    });

    // Wait for the tasks to complete
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
