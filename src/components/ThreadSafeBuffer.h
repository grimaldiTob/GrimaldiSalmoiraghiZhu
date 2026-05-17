#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "../types/TelemetryBatch.h"
#include "../interfaces/ProducerBuffer.h"
#include "../interfaces/ConsumerBuffer.h"

/*====================== INTERFACES OFFERED BY THE QUEUE =======================*/
template<typename T>
class ProducerBuffer {
public:
    virtual void push(T item) = 0;
    virtual void finish_production() = 0;
};

template<typename T>
class ConsumerBuffer {
public:
    virtual bool pop(T& item) = 0;
};

/*==============================================================================*/

/**
 * This template class entirely hides(encaplusaltion) the synchronization logic (mutexes, locks, CVs) 
 * from the outside world.
 * 
 * In our specifc project (T = TelematryBatch) acts as a thread-safe broker between the BatchAccumulator and the RuleEngine.
 * It encapsulates the synchronization logic required to safely access the shared batch queue.
 */
template <typename T>
class ThreadSafeBuffer: public ProducerBuffer<T>, public ConsumerBuffer<T> {

public:
    explicit ThreadSafeBuffer(size_t max_size) 
        : m_maxSize(max_size), m_isFinished(false) {}

    // Safely push an item into the buffer
    void push(T item) override {
        std::unique_lock<std::mutex> lock(m_mtx); // only one thread at a time can hold this lock
        
        // Wait until there is room in the buffer
        m_cvProducer.wait(lock, [this]() { return m_queue.size() < m_maxSize; });

        m_queue.push(std::move(item));
        
        lock.unlock();
        m_cvConsumer.notify_one(); // Wake up a consumer
    }

    // Safely pop an item. Returns false if the buffer is empty AND production is done.
    // [the pop must return true or false and passed the batch through the parameter
    // to enable multithreaded programming, in particular in the RuleEngine.run() there is a loop
    // that continue try to fetch a batche from the queue as long as it reads true. 
    // For also safety reason, is better to keep this signature whenever exceptions occur] 
    bool pop(T& item) override {
        std::unique_lock<std::mutex> lock(m_mtx);
        
        // Wait until there is an item OR production is finished
        m_cvConsumer.wait(lock, [this]() { return !m_queue.empty() || m_isFinished; });

        if (m_queue.empty() && m_isFinished) {
            return false; // Nothing left to consume
        }

        // other approach would be to move the ownership instead.
        // we will do this operation many times on different cores so
        // having a copy of the batch in the RuleEngine is not good for memory.
        item = std::move(m_queue.front());
        m_queue.pop();
        
        lock.unlock();
        m_cvProducer.notify_one(); // Wake up a producer
        
        return true;
    }

    /** @brief Signal that no more items will be produced
    */
    void finish_production() override {
        std::unique_lock<std::mutex> lock(m_mtx); 
        m_isFinished = true; // safetly set the flag 
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