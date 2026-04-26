#pragma once
#include <type_traits>
#include <utility>
namespace pmm {
template <typename T>
concept HasFreeData = requires(T &t) {
  { t.free_data() } noexcept;
};
template <typename T>
concept HasFreeAll = requires(T &t) {
  { t.free_all() } noexcept;
};
template <typename T>
concept HasPersistentCleanup = HasFreeData<T> || HasFreeAll<T>;
/*
## pmm-typed_guard
*/
template <typename T, typename ManagerT> class typed_guard {
public:
  using pptr_type = typename ManagerT::template pptr<T>;
  /*
### pmm-typed_guard-typed_guard
*/
  explicit typed_guard(pptr_type p) noexcept : _ptr(p) {}
  typed_guard() noexcept = default;
  typed_guard(const typed_guard &) = delete;
  typed_guard &operator=(const typed_guard &) = delete;
  typed_guard(typed_guard &&other) noexcept : _ptr(other._ptr) {
    other._ptr = pptr_type();
  }
  typed_guard &operator=(typed_guard &&other) noexcept {
    if (this != &other) {
      reset();
      _ptr = other._ptr;
      other._ptr = pptr_type();
    }
    return *this;
  }
  ~typed_guard() { reset(); }
  /*
### pmm-typed_guard-reset
*/
  void reset() noexcept {
    if (!_ptr.is_null()) {
      cleanup(*_ptr);
      ManagerT::destroy_typed(_ptr);
      _ptr = pptr_type();
    }
  }
  /*
### pmm-typed_guard-release
*/
  pptr_type release() noexcept {
    pptr_type p = _ptr;
    _ptr = pptr_type();
    return p;
  }
  /*
### pmm-typed_guard-operator_arrow
*/
  T *operator->() const noexcept { return &(*_ptr); }
  /*
### pmm-typed_guard-operator_deref
*/
  T &operator*() const noexcept { return *_ptr; }
  /*
### pmm-typed_guard-get
*/
  pptr_type get() const noexcept { return _ptr; }
  /*
### pmm-typed_guard-operator_bool
*/
  explicit operator bool() const noexcept { return !_ptr.is_null(); }
private:
  pptr_type _ptr;
  /*
### pmm-typed_guard-cleanup
*/
  static void cleanup(T &obj) noexcept {
    if constexpr (HasFreeData<T>)
      obj.free_data();
    else if constexpr (HasFreeAll<T>)
      obj.free_all();
  }
};
}
