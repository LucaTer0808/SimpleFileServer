#ifndef thread_safe_queue
#define thread_safe_queue

namespace SFS {
    /**
     * Custom thread-safe implementation of a queue using mutex and condition variable.
     */
    template <typename T>
    class ThreadSafeQueue {
        private:
            std::queue<T> queue;
            std::mutex queue_mutex;
            std::condition_variable not_empty;

        public:
            /**
             * @brief Checks if the queue contains any elements.
             * @return true, if the queue is empty. false otherwise.
             */
            bool is_empty() const {
                std::lock_guard<std::mutex> lock(this->queue_mutex);
                return this->queue.empty();
            }

            /**
             * @brief Pushes an element to the queue and appends it at the end.
             * @param element The element to push.
             */
            void push(const T element) {
                std::lock_guard<std::mutex> lock(this->queue_mutex);
                this->queue.push(std::move(element));
                this->not_empty.notify_one();
            }

            /**
             * @brief Blocking version of pop. Moves the first element in the queue into the passed variable in order
             * to avoid the missing entry problem.
             * @param target A reference to a variable in which the element to pop is copied into.
             */
            void pop_and_block(T& target) {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
    
                // Warte, bis die Queue NICHT mehr leer ist.
                this->not_empty.wait(lock, [this] { return !this->queue.empty(); });
    
                target = std::move(this->queue.front());
                this->queue.pop();
            }

            /**
             * @brief Non-blocking version of pop. Moves the first element in the queue into the passed variable in order
             * to avoid the missing entry problem.
             * @param target A reference to a variable in which the element to pop is copied into.
             * @return true, if an object was copied into target. false otherwise
             */
            bool try_pop(T& target) {
                std::lock_guard<std::mutex> lock(this->queue_mutex);
                if (!this->queue.empty()) {
                    target = std::move(this->queue.front());
                    this->queue.pop();
                    return true;
                }
                return false;
            }
    };
}

#endif /* thread_safe_queue */