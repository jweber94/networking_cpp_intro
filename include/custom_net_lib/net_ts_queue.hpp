#ifndef TSQUEUENET
#define TSQUEUENET

#include "common_net_includes.hpp"
#include "connection_net_interface.hpp"
#include "net_message.hpp"
#include "net_ts_queue.hpp"

namespace custom_netlib {

template <typename T> // here the template is used for the custom message
                      // datatype || the custom owned message datatype
                      class TsNetQueue {
  // make it a template since we want to make it deal with everything that needs
  // to be thread save
public:
  // rule of five parts
  TsNetQueue() = default;
  TsNetQueue(const TsNetQueue<T> &) = delete; // forbit copy constructor
  virtual ~TsNetQueue() { this->clear(); }

  // methods
  // get pointer to the ends of the queue
  const T &front() {
    /*
     * Returns a pointer to the front of the thread save net queue
     */
    std::unique_lock<std::mutex> lck(mutex_q_);
    return data_queue_.front();
  }

  const T &back() {
    std::unique_lock<std::mutex> lck(mutex_q_);
    return data_queue_.back();
  }

  // insert at the front and at the back of the queue
  void pushBack(const T &input_element) {
    /*
     * Adding elements to the queue by using move semantics, since we want to
     * hand over ownership from one thread to another of the elements
     */
    std::unique_lock<std::mutex> lck(mutex_q_);
    data_queue_.emplace_back(std::move(input_element));
  }
  void pushFront(const T &input_element) {
    std::unique_lock<std::mutex> lck(mutex_q_);
    data_queue_.push_front(std::move(input_element));
  }

  bool isEmpty() {
    std::unique_lock<std::mutex> lck(mutex_q_);
    return data_queue_.empty();
  }

  size_t count() {
    /*
     * Returns the current number of elements within the queue
     */
    std::unique_lock<std::mutex> lck(mutex_q_);
    return data_queue_.size();
  }

  void clear() {
    std::unique_lock<std::mutex> lck(mutex_q_);
    data_queue_.clear();
  }

  T popFront() {
    std::unique_lock<std::mutex> lck(mutex_q_);
    auto tmp_element = std::move(
        data_queue_
            .front()); // using move semantics to make the code more efficient
    data_queue_.pop_front();
    return tmp_element;
  }

  T popBack() {
    std::unique_lock<std::mutex> lck(mutex_q_);
    auto tmp_element = std::move(data_queue_.back());
    data_queue_.pop_back();
    return tmp_element;
  }

protected:
  // protected --> classes that inherit from TsNetQueue can directly access this
  // member functions
  std::mutex mutex_q_;
  std::deque<T> data_queue_;
};

} // namespace custom_netlib

#endif /* TSQUEUENET */