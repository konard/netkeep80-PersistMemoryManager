#pragma once

#include "pmm/avl_tree_mixin.h"
#include "pmm/forest_registry.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pmm {

template <typename ManagerT> struct pstringview;

/*
## pmm::pstringview
*/
template <typename ManagerT> struct pstringview {

  using manager_type = ManagerT;

  using index_type = typename ManagerT::index_type;

  using psview_pptr = typename ManagerT::template pptr<pstringview>;

  /*
## pmm::pstringview::forest_domain_descriptor
  */
  struct forest_domain_descriptor {

    using manager_type = ManagerT;

    using index_type = typename ManagerT::index_type;

    using node_type = pstringview;

    using node_pptr = psview_pptr;

    /*
### pmm::pstringview::forest_domain_descriptor::name
    */
    static constexpr const char *name() noexcept {
      return detail::kSystemDomainSymbols;
    }

    /*
### pmm::pstringview::forest_domain_descriptor::root_index
    */
    static index_type root_index() noexcept {

      auto *domain = ManagerT::symbol_domain_record_unlocked();
      return ManagerT::forest_domain_root_index_unlocked(domain);
    }

    /*
### pmm::pstringview::forest_domain_descriptor::root_index_ptr
    */
    static index_type *root_index_ptr() noexcept {
      auto *domain = ManagerT::symbol_domain_record_unlocked();
      return ManagerT::forest_domain_root_index_ptr_unlocked(domain);
    }

    /*
### pmm::pstringview::forest_domain_descriptor::resolve_node
    */
    static node_type *resolve_node(node_pptr p) noexcept {
      return ManagerT::template resolve<node_type>(p);
    }

    /*
### pmm::pstringview::forest_domain_descriptor::compare_key
    */
    static int compare_key(const char *key, node_pptr cur) noexcept {
      if (key == nullptr)

        key = "";

      node_type *obj = resolve_node(cur);
      return (obj != nullptr) ? std::strcmp(key, obj->c_str()) : 0;
    }

    /*
### pmm::pstringview::forest_domain_descriptor::less_node
    */
    static bool less_node(node_pptr lhs, node_pptr rhs) noexcept {
      node_type *lhs_obj = resolve_node(lhs);
      node_type *rhs_obj = resolve_node(rhs);
      return lhs_obj != nullptr && rhs_obj != nullptr &&
             std::strcmp(lhs_obj->c_str(), rhs_obj->c_str()) < 0;
    }

    /*
### pmm::pstringview::forest_domain_descriptor::validate_node
    */
    static bool validate_node(node_pptr p) noexcept {
      return resolve_node(p) != nullptr;
    }
  };

  using forest_domain_policy =
      detail::ForestDomainOps<forest_domain_descriptor>;

  /*
### pmm::pstringview::forest_domain_ops
  */
  static forest_domain_policy forest_domain_ops() noexcept {
    return forest_domain_policy{};
  }

  std::uint32_t length;

  char str[1];

  /*
### pmm::pstringview::pstringview
  */
  explicit pstringview(const char *s) noexcept : length(0), str{'\0'} {
    _interned = _intern(s);
  }

  /*
### pmm::pstringview::operator_psview_pptr
  */
  operator psview_pptr() const noexcept { return _interned; }

  /*
### pmm::pstringview::c_str
  */
  const char *c_str() const noexcept { return str; }

  /*
### pmm::pstringview::size
  */
  std::size_t size() const noexcept { return static_cast<std::size_t>(length); }

  /*
### pmm::pstringview::empty
  */
  bool empty() const noexcept { return length == 0; }

  bool operator==(const char *s) const noexcept {
    if (s == nullptr)
      return length == 0;
    return std::strcmp(c_str(), s) == 0;
  }

  bool operator==(const pstringview &other) const noexcept {

    if (this == &other)
      return true;

    if (length != other.length)
      return false;
    return std::strcmp(str, other.str) == 0;
  }

  bool operator!=(const char *s) const noexcept { return !(*this == s); }

  bool operator!=(const pstringview &other) const noexcept {
    return !(*this == other);
  }

  /*
### pmm::pstringview::operator_less
  */
  bool operator<(const pstringview &other) const noexcept {
    return std::strcmp(c_str(), other.c_str()) < 0;
  }

  /*
### pmm::pstringview::intern
  */
  static psview_pptr intern(const char *s) noexcept { return _intern(s); }

  /*
### pmm::pstringview::reset
  */
  static void reset() noexcept {
    if (!ManagerT::is_initialized())

      return;

    typename ManagerT::thread_policy::unique_lock_type lock(ManagerT::_mutex);

    forest_domain_ops().reset_root();
  }

  /*
### pmm::pstringview::root_index
  */
  static index_type root_index() noexcept {
    if (!ManagerT::is_initialized())
      return static_cast<index_type>(0);
    typename ManagerT::thread_policy::shared_lock_type lock(ManagerT::_mutex);
    return forest_domain_ops().root_index();
  }

  ~pstringview() = default;

private:
  psview_pptr _interned;

  /*
### pmm::pstringview::_intern
  */
  static psview_pptr _intern(const char *s) noexcept {
    if (!ManagerT::is_initialized())
      return psview_pptr();
    typename ManagerT::thread_policy::unique_lock_type lock(ManagerT::_mutex);
    return ManagerT::intern_symbol_unlocked(s);
  }
};

}
