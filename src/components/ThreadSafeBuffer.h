#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "TelemetryBatch.h"

/**
 * This template class entirely hides the synchronization logic (mutexes, locks, CVs) 
 * from the outside world.
 * 
 * In our specifc project (T = TelematryBatch) acts as a thread-safe broker between the BatchAccumulator and the RuleEngine.
 * It encapsulates the synchronization logic required to safely access the shared batch queue.
 */
template <typename T>
class ThreadSafeBuffer {

public:
    explicit ThreadSafeBuffer(size_t max_size) 
        : m_maxSize(max_size), m_isFinished(false) {}

    // Safely push an item into the buffer
    void push(T item) {
        std::unique_lock<std::mutex> lock(m_mtx);
        
        // Wait until there is room in the buffer
        m_cvProducer.wait(lock, [this]() { return m_queue.size() < m_maxSize; });

        m_queue.push(item);
        
        lock.unlock();
        m_cvConsumer.notify_one(); // Wake up a consumer
    }

    // Safely pop an item. Returns false if the buffer is empty AND production is done.
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_mtx);
        
        // Wait until there is an item OR production is finished
        m_cvConsumer.wait(lock, [this]() { return !m_queue.empty() || m_isFinished; });

        if (m_queue.empty() && m_isFinished) {
            return false; // Nothing left to consume
        }

        item = m_queue.front();
        m_queue.pop();
        
        lock.unlock();
        m_cvProducer.notify_one(); // Wake up a producer
        
        return true;
    }

    // Signal that no more items will be produced
    void finish_production() {
        std::unique_lock<std::mutex> lock(m_mtx);
        m_isFinished = true;
        lock.unlock();
        m_cvConsumer.notify_all(); // Wake up all waiting consumers so they can exit
    }

private:

    std::queue<T>           m_queue;         // Underlying container holding buffered items; access guarded by `m_mtx`.
    std::mutex              m_mtx;           // Mutex protecting `m_queue`, `m_isFinished`, and other shared state.
    std::condition_variable m_cvProducer;    // Notified when space becomes available so a producer can push.
    std::condition_variable m_cvConsumer;    // Notified when an item is pushed or production is finished so consumers can pop/exit.
    size_t                  m_maxSize;       // Maximum number of items allowed in the buffer before producers block.
    bool                    m_isFinished;    // True when producers have finished producing (no more items will be pushed).

};

template class ThreadSafeBuffer<TelemetryBatch>;