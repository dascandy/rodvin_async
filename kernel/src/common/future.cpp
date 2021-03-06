#include "future.h"
#include <vector>

class queued_executor : public executor {
public:
  void queue(const std::function<void()>& f) {
    funcs.push_back(f);
  }
  void run_all() {
    std::vector<std::function<void()>> fs = funcs;
    funcs.clear();
    for (auto& f : fs) {
      f();
    }
  }
  std::vector<std::function<void()>> funcs;
};

queued_executor& q_executor() {
  static queued_executor ex;
  return ex;
}

executor& default_executor() {
  return q_executor();
}

void run_queued_tasks() {
  q_executor().run_all();
}

future<void> make_ready_void_future() {
  std::shared_ptr<shared_state<void>> state = std::make_shared<shared_state<void>>();
  future<void> f(state);
  state->set();
  return f;
}


