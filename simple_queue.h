#ifndef __MY_SIMPLE_QUEUE_H__
#define __MY_SIMPLE_QUEUE_H__

#include <queue>
#include <mutex>
#include <condition_variable>


template<typename T>
class simple_queue {
public:
  void push(const T &val) {
    std::lock_guard<std::mutex> l(m);
    q.push(val);
    cv.notify_one();
  }
  int pop_or_timeout(T *result, int secs) {
    std::unique_lock<std::mutex> l(m);
    cv.wait_for(l, std::chrono::seconds(secs), [this] () { return !this->q.empty(); });
    //cv.wait(l, [this] () { return !this->q.empty(); });
    if (q.empty()) {
      return -1;
    } else {
      *result = q.front();
      q.pop();
      return 0;
    }
  }
  bool empty() {
    std::lock_guard<std::mutex> l(m);
    return q.empty();
  }
private:
  std::queue<T> q;
  std::mutex m;
  std::condition_variable cv;
}; /*class simple_queue */


#endif /* __MY_SIMPLE_QUEUE_H__ */
