#pragma once

#include "pmm/types.h"

#include <cstddef>
#include <cstdio>

namespace pmm {
namespace logging {

/*
### pmm-logging-nologging
*/
struct NoLogging {

  /*
#### pmm-logging-nologging-on_allocation_failure
*/
  static void on_allocation_failure(std::size_t, PmmError) noexcept {}

  /*
#### pmm-logging-nologging-on_expand
*/
  static void on_expand(std::size_t, std::size_t) noexcept {}

  /*
#### pmm-logging-nologging-on_corruption_detected
*/
  static void on_corruption_detected(PmmError) noexcept {}

  /*
#### pmm-logging-nologging-on_create
*/
  static void on_create(std::size_t) noexcept {}

  /*
#### pmm-logging-nologging-on_destroy
*/
  static void on_destroy() noexcept {}

  /*
#### pmm-logging-nologging-on_load
*/
  static void on_load() noexcept {}
};

/*
### pmm-logging-stderrlogging
*/
struct StderrLogging {

  /*
#### pmm-logging-stderrlogging-on_allocation_failure
*/
  static void on_allocation_failure(std::size_t user_size,
                                    PmmError err) noexcept {

    std::fprintf(stderr, "[pmm] allocation_failure: size=%zu error=%d\n",
                 user_size, static_cast<int>(err));
  }

  /*
#### pmm-logging-stderrlogging-on_expand
*/
  static void on_expand(std::size_t old_size, std::size_t new_size) noexcept {
    std::fprintf(stderr, "[pmm] expand: %zu -> %zu\n", old_size, new_size);
  }

  /*
#### pmm-logging-stderrlogging-on_corruption_detected
*/
  static void on_corruption_detected(PmmError err) noexcept {
    std::fprintf(stderr, "[pmm] corruption_detected: error=%d\n",
                 static_cast<int>(err));
  }

  /*
#### pmm-logging-stderrlogging-on_create
*/
  static void on_create(std::size_t initial_size) noexcept {
    std::fprintf(stderr, "[pmm] create: size=%zu\n", initial_size);
  }

  /*
#### pmm-logging-stderrlogging-on_destroy
*/
  static void on_destroy() noexcept { std::fprintf(stderr, "[pmm] destroy\n"); }

  /*
#### pmm-logging-stderrlogging-on_load
*/
  static void on_load() noexcept { std::fprintf(stderr, "[pmm] load\n"); }
};

}
}
