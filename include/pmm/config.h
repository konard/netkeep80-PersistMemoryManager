#pragma once

#include <mutex>
#include <shared_mutex>

namespace pmm {
namespace config {

/*
### pmm::config::sharedmutexlock
*/
struct SharedMutexLock {

  using mutex_type = std::shared_mutex;

  using shared_lock_type = std::shared_lock<std::shared_mutex>;

  using unique_lock_type = std::unique_lock<std::shared_mutex>;
};

/*
### pmm::config::nolock
*/
struct NoLock {

  /*
#### pmm::config::nolock::mutex_type
*/
  struct mutex_type {

    /*
##### pmm::config::nolock::mutex_type::lock
*/
    void lock() {}

    /*
##### pmm::config::nolock::mutex_type::unlock
*/
    void unlock() {}

    /*
##### pmm::config::nolock::mutex_type::lock_shared
*/
    void lock_shared() {}

    /*
##### pmm::config::nolock::mutex_type::unlock_shared
*/
    void unlock_shared() {}

    /*
##### pmm::config::nolock::mutex_type::try_lock
*/
    bool try_lock() { return true; }

    /*
##### pmm::config::nolock::mutex_type::try_lock_shared
*/
    bool try_lock_shared() { return true; }
  };

  /*
#### pmm::config::nolock::shared_lock_type
*/
  struct shared_lock_type {

    /*
##### pmm::config::nolock::shared_lock_type::shared_lock_type
*/
    explicit shared_lock_type(mutex_type &) {}
  };

  /*
#### pmm::config::nolock::unique_lock_type
*/
  struct unique_lock_type {

    /*
##### pmm::config::nolock::unique_lock_type::unique_lock_type
*/
    explicit unique_lock_type(mutex_type &) {}
  };
};

inline constexpr std::size_t kDefaultGrowNumerator = 5;

inline constexpr std::size_t kDefaultGrowDenominator = 4;

}
}
