#include <thread>
#include <chrono>
#include <functional>
#include <cstdio>
#include <atomic>
#include <stdexcept>

class Killswitch {
public:
    Killswitch(): mRunning(false) {}
    ~Killswitch() { if (mRunning) stop(); }

    void start(const int seconds) {
        mRunning = true;
        mThread = std::thread([this, seconds] {
            for(int s = seconds; s > 0 && mRunning; s--) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (mRunning) throw std::runtime_error("timeout expired");
        });
    }
    void stop() {
        mRunning = false;
        mThread.join();
    }

private:
    std::thread mThread{};
    std::atomic_bool mRunning{};
};
