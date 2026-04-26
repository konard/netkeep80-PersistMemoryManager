#pragma once

#include "pmm/block_state.h"
#include "pmm/diagnostics.h"
#include "pmm/free_block_tree.h"
#include "pmm/types.h"
#include "pmm/validation.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

namespace pmm {

template <typename FreeBlockTreeT = AvlFreeTree<DefaultAddressTraits>,
          typename AddressTraitsT = DefaultAddressTraits>

/*
## pmm::AllocatorPolicy
*/
class AllocatorPolicy {

  static_assert(
      FreeBlockTreePolicyForTraitsConcept<FreeBlockTreeT, AddressTraitsT>,

      "AllocatorPolicy: FreeBlockTreeT must satisfy FreeBlockTreePolicy for "
      "AddressTraitsT");

public:
  using address_traits = AddressTraitsT;

  using free_block_tree = FreeBlockTreeT;

  using index_type = typename AddressTraitsT::index_type;

  using BlockT = Block<AddressTraitsT>;

  using BlockState = BlockStateBase<AddressTraitsT>;

  /*
  ### pmm::AllocatorPolicy::AllocatorPolicy
  */
  AllocatorPolicy() = delete;
  AllocatorPolicy(const AllocatorPolicy &) = delete;

  AllocatorPolicy &operator=(const AllocatorPolicy &) = delete;

  /*
  ### pmm::AllocatorPolicy::allocate_from_block
  */
  static void *allocate_from_block(std::uint8_t *base,
                                   detail::ManagerHeader<AddressTraitsT> *hdr,
                                   index_type blk_idx, std::size_t user_size) {

    FreeBlockTreeT::remove(base, hdr, blk_idx);
    FreeBlock<AddressTraitsT> *fb =

        FreeBlock<AddressTraitsT>::cast_from_raw(
            detail::block_at<AddressTraitsT>(base, blk_idx));

    FreeBlockRemovedAVL<AddressTraitsT> *removed = fb->remove_from_avl();

    static constexpr index_type kBlkHdrGran =

        detail::kBlockHeaderGranules_t<AddressTraitsT>;

    index_type blk_total_gran =

        detail::block_total_granules(
            base, hdr, detail::block_at<AddressTraitsT>(base, blk_idx));

    index_type data_gran =
        detail::bytes_to_granules_t<AddressTraitsT>(user_size);

    if (data_gran > std::numeric_limits<index_type>::max() - kBlkHdrGran)
      return nullptr;

    index_type needed_gran = kBlkHdrGran + data_gran;

    index_type min_rem_gran = kBlkHdrGran + 1;

    bool can_split = false;
    if (needed_gran <= std::numeric_limits<index_type>::max() - min_rem_gran)
      can_split = (blk_total_gran >= needed_gran + min_rem_gran);

    if (can_split) {

      SplittingBlock<AddressTraitsT> *splitting = removed->begin_splitting();

      index_type new_idx = blk_idx + needed_gran;
      void *new_blk_ptr = detail::block_at<AddressTraitsT>(base, new_idx);

      index_type curr_next = splitting->next_offset();
      BlockT *old_next = (curr_next != AddressTraitsT::no_block)
                             ? detail::block_at<AddressTraitsT>(base, curr_next)
                             : nullptr;

      splitting->initialize_new_block(new_blk_ptr, new_idx, blk_idx);

      splitting->link_new_block(old_next, new_idx);
      if (old_next == nullptr)
        hdr->last_block_offset = new_idx;

      hdr->block_count++;
      hdr->free_count++;
      hdr->used_size += kBlkHdrGran;
      FreeBlockTreeT::insert(base, hdr, new_idx);

      AllocatedBlock<AddressTraitsT> *alloc =
          splitting->finalize_split(data_gran, blk_idx);
      (void)alloc;
    } else {

      AllocatedBlock<AddressTraitsT> *alloc =
          removed->mark_as_allocated(data_gran, blk_idx);
      (void)alloc;
    }

    hdr->alloc_count++;

    hdr->free_count--;

    hdr->used_size += data_gran;

    return detail::user_ptr<AddressTraitsT>(
        detail::block_at<AddressTraitsT>(base, blk_idx));
  }

  /*
  ### pmm::AllocatorPolicy::coalesce
  */
  static void coalesce(std::uint8_t *base,
                       detail::ManagerHeader<AddressTraitsT> *hdr,
                       index_type blk_idx) {

    FreeBlockNotInAVL<AddressTraitsT> *not_avl =
        FreeBlockNotInAVL<AddressTraitsT>::cast_from_raw(
            detail::block_at<AddressTraitsT>(base, blk_idx));

    CoalescingBlock<AddressTraitsT> *coalescing = not_avl->begin_coalescing();

    static constexpr index_type kBlkHdrGran =
        detail::kBlockHeaderGranules_t<AddressTraitsT>;

    index_type b_idx = blk_idx;

    index_type curr_next = coalescing->next_offset();
    if (curr_next != AddressTraitsT::no_block) {
      void *nxt_raw = detail::block_at<AddressTraitsT>(base, curr_next);
      if (BlockState::get_weight(nxt_raw) == 0) {
        index_type nxt_idx = curr_next;
        index_type nxt_next = BlockState::get_next_offset(nxt_raw);
        BlockT *nxt_nxt_blk =
            (nxt_next != AddressTraitsT::no_block)
                ? detail::block_at<AddressTraitsT>(base, nxt_next)
                : nullptr;

        FreeBlockTreeT::remove(base, hdr, nxt_idx);

        coalescing->coalesce_with_next(
            detail::block_at<AddressTraitsT>(base, nxt_idx), nxt_nxt_blk,
            b_idx);

        if (nxt_nxt_blk == nullptr)
          hdr->last_block_offset = b_idx;

        hdr->block_count--;
        hdr->free_count--;
        if (hdr->used_size >= kBlkHdrGran)
          hdr->used_size -= kBlkHdrGran;
      }
    }

    index_type curr_prev = coalescing->prev_offset();
    if (curr_prev != AddressTraitsT::no_block) {
      void *prv_raw = detail::block_at<AddressTraitsT>(base, curr_prev);
      if (BlockState::get_weight(prv_raw) == 0) {
        index_type prv_idx = curr_prev;
        index_type blk_next = coalescing->next_offset();
        BlockT *next_blk =
            (blk_next != AddressTraitsT::no_block)
                ? detail::block_at<AddressTraitsT>(base, blk_next)
                : nullptr;

        FreeBlockTreeT::remove(base, hdr, prv_idx);

        CoalescingBlock<AddressTraitsT> *result_coalescing =
            coalescing->coalesce_with_prev(prv_raw, next_blk, prv_idx);

        if (next_blk == nullptr)
          hdr->last_block_offset = prv_idx;

        hdr->block_count--;
        hdr->free_count--;
        if (hdr->used_size >= kBlkHdrGran)
          hdr->used_size -= kBlkHdrGran;

        FreeBlock<AddressTraitsT> *fb = result_coalescing->finalize_coalesce();
        (void)fb;
        FreeBlockTreeT::insert(base, hdr, prv_idx);
        return;
      }
    }

    FreeBlock<AddressTraitsT> *fb = coalescing->finalize_coalesce();
    (void)fb;

    FreeBlockTreeT::insert(base, hdr, b_idx);
  }

  /*
  ### pmm::AllocatorPolicy::rebuild_free_tree
  */
  static void rebuild_free_tree(std::uint8_t *base,
                                detail::ManagerHeader<AddressTraitsT> *hdr) {

    hdr->free_tree_root = AddressTraitsT::no_block;

    hdr->last_block_offset = AddressTraitsT::no_block;

    index_type idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {
      void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);

      BlockState::recover_state(blk_ptr, idx);

      if (BlockState::get_weight(blk_ptr) == 0) {

        BlockState::reset_avl_fields_of(blk_ptr);
        FreeBlockTreeT::insert(base, hdr, idx);
      }

      index_type next_idx = BlockState::get_next_offset(blk_ptr);
      if (next_idx == AddressTraitsT::no_block)
        hdr->last_block_offset = idx;
      idx = next_idx;
    }
  }

  /*
  ### pmm::AllocatorPolicy::repair_linked_list
  */
  static void repair_linked_list(std::uint8_t *base,
                                 detail::ManagerHeader<AddressTraitsT> *hdr) {
    index_type idx = hdr->first_block_offset;

    index_type prev = AddressTraitsT::no_block;
    while (idx != AddressTraitsT::no_block) {

      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      BlockState::repair_prev_offset(blk_ptr, prev);
      prev = idx;
      index_type next_offset = BlockState::get_next_offset(blk_ptr);
      idx = next_offset;
    }
  }

  /*
  ### pmm::AllocatorPolicy::recompute_counters
  */
  static void recompute_counters(std::uint8_t *base,
                                 detail::ManagerHeader<AddressTraitsT> *hdr) {

    static constexpr index_type kBlkHdrGran =
        detail::kBlockHeaderGranules_t<AddressTraitsT>;

    index_type block_count = 0, free_count = 0, alloc_count = 0;

    index_type used_gran = 0;
    index_type idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {

      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      block_count++;
      used_gran += kBlkHdrGran;
      index_type w = BlockState::get_weight(blk_ptr);
      if (w > 0) {
        alloc_count++;
        used_gran += w;
      } else {
        free_count++;
      }
      idx = BlockState::get_next_offset(blk_ptr);
    }
    hdr->block_count = block_count;
    hdr->free_count = free_count;
    hdr->alloc_count = alloc_count;
    hdr->used_size = used_gran;
  }

  /*
  ### pmm::AllocatorPolicy::verify_linked_list
  */
  static void
  verify_linked_list(const std::uint8_t *base,
                     const detail::ManagerHeader<AddressTraitsT> *hdr,
                     VerifyResult &result) noexcept {
    index_type idx = hdr->first_block_offset;
    index_type prev = AddressTraitsT::no_block;
    while (idx != AddressTraitsT::no_block) {
      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      index_type stored_prev = BlockState::get_prev_offset(blk_ptr);
      if (stored_prev != prev) {
        result.add(ViolationType::PrevOffsetMismatch,
                   DiagnosticAction::NoAction, static_cast<std::uint64_t>(idx),
                   static_cast<std::uint64_t>(prev),
                   static_cast<std::uint64_t>(stored_prev));
      }
      prev = idx;
      index_type next_offset = BlockState::get_next_offset(blk_ptr);
      idx = next_offset;
    }
  }

  /*
  ### pmm::AllocatorPolicy::verify_counters
  */
  static void verify_counters(const std::uint8_t *base,
                              const detail::ManagerHeader<AddressTraitsT> *hdr,
                              VerifyResult &result) noexcept {
    static constexpr index_type kBlkHdrGran =
        detail::kBlockHeaderGranules_t<AddressTraitsT>;

    index_type block_count = 0, free_count = 0, alloc_count = 0;
    index_type used_gran = 0;
    index_type idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {
      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      block_count++;
      used_gran += kBlkHdrGran;
      index_type w = BlockState::get_weight(blk_ptr);
      if (w > 0) {
        alloc_count++;
        used_gran += w;
      } else {
        free_count++;
      }
      idx = BlockState::get_next_offset(blk_ptr);
    }
    if (hdr->block_count != block_count || hdr->free_count != free_count ||
        hdr->alloc_count != alloc_count || hdr->used_size != used_gran) {
      result.add(ViolationType::CounterMismatch, DiagnosticAction::NoAction, 0,
                 static_cast<std::uint64_t>(block_count),
                 static_cast<std::uint64_t>(hdr->block_count));
    }
  }

  /*
  ### pmm::AllocatorPolicy::verify_block_states
  */
  static void
  verify_block_states(const std::uint8_t *base,
                      const detail::ManagerHeader<AddressTraitsT> *hdr,
                      VerifyResult &result) noexcept {
    index_type idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {
      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      BlockState::verify_state(blk_ptr, idx, result);
      idx = BlockState::get_next_offset(blk_ptr);
    }
  }

  /*
  ### pmm::AllocatorPolicy::verify_free_tree
  */
  static void verify_free_tree(const std::uint8_t *base,
                               const detail::ManagerHeader<AddressTraitsT> *hdr,
                               VerifyResult &result) noexcept {

    std::size_t expected_count = 0;
    index_type idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {
      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      if (BlockState::get_weight(blk_ptr) == 0)
        ++expected_count;
      idx = BlockState::get_next_offset(blk_ptr);
    }

    const bool root_present = (hdr->free_tree_root != AddressTraitsT::no_block);
    if (expected_count == 0) {
      if (root_present) {
        result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0,
                   0, static_cast<std::uint64_t>(hdr->free_tree_root));
      }
      return;
    }
    if (!root_present) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0,
                 static_cast<std::uint64_t>(expected_count),
                 static_cast<std::uint64_t>(hdr->free_tree_root));
      return;
    }
    if (!detail::validate_block_index<AddressTraitsT>(hdr->total_size,
                                                      hdr->free_tree_root)) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(hdr->free_tree_root), 1, 0);
      return;
    }
    const void *root =
        detail::block_at<AddressTraitsT>(base, hdr->free_tree_root);
    if (BlockState::get_weight(root) != 0 ||
        BlockState::get_parent_offset(root) != AddressTraitsT::no_block) {
      result.add(
          ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
          static_cast<std::uint64_t>(hdr->free_tree_root), 0,
          static_cast<std::uint64_t>(BlockState::get_parent_offset(root)));
    }

    std::size_t visited_count = 0;

    verify_free_tree_node(base, hdr, hdr->free_tree_root,
                          AddressTraitsT::no_block, {}, false, {}, false,

                          expected_count, visited_count, result);

    idx = hdr->first_block_offset;
    while (idx != AddressTraitsT::no_block) {
      if (static_cast<std::size_t>(idx) * AddressTraitsT::granule_size +
              sizeof(BlockT) >
          hdr->total_size)
        break;
      const void *blk_ptr = detail::block_at<AddressTraitsT>(base, idx);
      if (BlockState::get_weight(blk_ptr) == 0 &&
          !free_tree_contains(base, hdr, hdr->free_tree_root, idx,
                              expected_count)) {
        result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                   static_cast<std::uint64_t>(idx), 1, 0);
      }
      idx = BlockState::get_next_offset(blk_ptr);
    }
    if (visited_count != expected_count) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0,
                 static_cast<std::uint64_t>(expected_count),
                 static_cast<std::uint64_t>(visited_count));
    }
  }

  /*
  ### pmm::AllocatorPolicy::free_tree_block_granules
  */
  static index_type
  free_tree_block_granules(const std::uint8_t *base,
                           const detail::ManagerHeader<AddressTraitsT> *hdr,
                           index_type block_idx) noexcept {
    const void *n = detail::block_at<AddressTraitsT>(base, block_idx);

    index_type n_next = BlockState::get_next_offset(n);
    index_type total =
        detail::byte_off_to_idx_t<AddressTraitsT>(hdr->total_size);
    return (n_next != AddressTraitsT::no_block)
               ? static_cast<index_type>(n_next - block_idx)

               : static_cast<index_type>(total - block_idx);
  }

  /*
  ### pmm::AllocatorPolicy::free_tree_less_key
  */
  static bool
  free_tree_less_key(const std::uint8_t *base,
                     const detail::ManagerHeader<AddressTraitsT> *hdr,
                     index_type a, index_type b) noexcept {

    index_type a_gran = free_tree_block_granules(base, hdr, a);
    index_type b_gran = free_tree_block_granules(base, hdr, b);
    return (a_gran < b_gran) || (a_gran == b_gran && a < b);
  }

  /*
  ### pmm::AllocatorPolicy::free_tree_contains
  */
  static bool
  free_tree_contains(const std::uint8_t *base,
                     const detail::ManagerHeader<AddressTraitsT> *hdr,
                     index_type node_idx, index_type target,
                     std::size_t step_limit) noexcept {
    while (node_idx != AddressTraitsT::no_block && step_limit-- > 0) {
      if (!detail::validate_block_index<AddressTraitsT>(hdr->total_size,
                                                        node_idx))
        return false;
      const void *node = detail::block_at<AddressTraitsT>(base, node_idx);
      if (BlockState::get_weight(node) != 0)
        return false;
      if (node_idx == target)
        return true;
      node_idx = free_tree_less_key(base, hdr, target, node_idx)
                     ? BlockState::get_left_offset(node)
                     : BlockState::get_right_offset(node);
    }
    return false;
  }

  /*
  ### pmm::AllocatorPolicy::verify_free_tree_node
  */
  static std::int16_t verify_free_tree_node(
      const std::uint8_t *base,
      const detail::ManagerHeader<AddressTraitsT> *hdr, index_type node_idx,
      index_type parent, index_type lower, bool has_lower, index_type upper,
      bool has_upper, std::size_t expected_count, std::size_t &visited_count,
      VerifyResult &result) noexcept {
    if (node_idx == AddressTraitsT::no_block)
      return 0;
    if (visited_count >= expected_count) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(node_idx), 1, 2);
      return 0;
    }

    ++visited_count;

    if (!detail::validate_block_index<AddressTraitsT>(hdr->total_size,
                                                      node_idx)) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(parent), 0,
                 static_cast<std::uint64_t>(node_idx));
      return 0;
    }

    const void *node = detail::block_at<AddressTraitsT>(base, node_idx);
    if (BlockState::get_weight(node) != 0) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(node_idx), 0,
                 static_cast<std::uint64_t>(BlockState::get_weight(node)));
      return 0;
    }
    if (BlockState::get_parent_offset(node) != parent) {
      result.add(
          ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
          static_cast<std::uint64_t>(node_idx),
          static_cast<std::uint64_t>(parent),
          static_cast<std::uint64_t>(BlockState::get_parent_offset(node)));
    }
    if (has_lower && !free_tree_less_key(base, hdr, lower, node_idx)) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(node_idx),
                 static_cast<std::uint64_t>(lower),
                 static_cast<std::uint64_t>(node_idx));
    }
    if (has_upper && !free_tree_less_key(base, hdr, node_idx, upper)) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(node_idx),
                 static_cast<std::uint64_t>(node_idx),
                 static_cast<std::uint64_t>(upper));
    }

    index_type left = BlockState::get_left_offset(node);

    index_type right = BlockState::get_right_offset(node);

    std::int16_t left_h = verify_free_tree_node(
        base, hdr, left, node_idx, lower, has_lower, node_idx, true,
        expected_count, visited_count, result);
    std::int16_t right_h =
        verify_free_tree_node(base, hdr, right, node_idx, node_idx, true, upper,
                              has_upper, expected_count, visited_count, result);

    std::int16_t expected_h =
        static_cast<std::int16_t>(1 + (left_h > right_h ? left_h : right_h));

    std::int16_t stored_h = BlockState::get_avl_height(node);
    if (stored_h != expected_h || left_h - right_h > 1 ||
        right_h - left_h > 1) {
      result.add(ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                 static_cast<std::uint64_t>(node_idx),
                 static_cast<std::uint64_t>(expected_h),
                 static_cast<std::uint64_t>(stored_h));
    }
    return expected_h;
  }

  /*
  ### pmm::AllocatorPolicy::realloc_shrink
  */
  static void realloc_shrink(std::uint8_t *base,
                             detail::ManagerHeader<AddressTraitsT> *hdr,
                             index_type blk_idx, void *blk_raw,
                             index_type old_data_gran,
                             index_type new_data_gran) noexcept {
    static constexpr index_type kBlkHdrGran =
        detail::kBlockHeaderGranules_t<AddressTraitsT>;

    index_type remainder = old_data_gran - new_data_gran;
    if (remainder >= kBlkHdrGran + 1) {
      index_type new_free_idx = blk_idx + kBlkHdrGran + new_data_gran;
      void *new_free_blk = detail::block_at<AddressTraitsT>(base, new_free_idx);
      index_type old_next = BlockState::get_next_offset(blk_raw);
      auto *old_next_blk =
          (old_next != AddressTraitsT::no_block)
              ? detail::block_at<AddressTraitsT>(base, old_next)
              : nullptr;
      std::memset(new_free_blk, 0, sizeof(BlockT));
      BlockState::init_fields(new_free_blk, blk_idx, old_next, 1, 0, 0);
      BlockState::set_next_offset_of(blk_raw, new_free_idx);
      if (old_next_blk != nullptr)
        BlockState::set_prev_offset_of(old_next_blk, new_free_idx);
      else
        hdr->last_block_offset = new_free_idx;
      BlockState::set_weight_of(blk_raw, new_data_gran);
      hdr->block_count++;
      hdr->free_count++;
      hdr->used_size += kBlkHdrGran;
      hdr->used_size -= (old_data_gran - new_data_gran);
      coalesce(base, hdr, new_free_idx);
    } else {
      BlockState::set_weight_of(blk_raw, new_data_gran);
      hdr->used_size -= (old_data_gran - new_data_gran);
    }
  }

  /*
  ### pmm::AllocatorPolicy::realloc_grow
  */
  static bool realloc_grow(std::uint8_t *base,
                           detail::ManagerHeader<AddressTraitsT> *hdr,
                           index_type blk_idx, void *blk_raw,
                           index_type old_data_gran,
                           index_type new_data_gran) noexcept {
    static constexpr index_type kBlkHdrGran =
        detail::kBlockHeaderGranules_t<AddressTraitsT>;
    index_type next_idx = BlockState::get_next_offset(blk_raw);
    if (next_idx == AddressTraitsT::no_block)
      return false;
    void *next_blk = detail::block_at<AddressTraitsT>(base, next_idx);
    if (BlockState::get_weight(next_blk) != 0)
      return false;
    index_type next_total = detail::block_total_granules(
        base, hdr, detail::block_at<AddressTraitsT>(base, next_idx));

    index_type available = old_data_gran + next_total;
    if (available < new_data_gran)
      return false;
    FreeBlockTreeT::remove(base, hdr, next_idx);
    index_type next_next = BlockState::get_next_offset(next_blk);

    BlockState::set_next_offset_of(blk_raw, next_next);
    if (next_next != AddressTraitsT::no_block)

      BlockState::set_prev_offset_of(
          detail::block_at<AddressTraitsT>(base, next_next), blk_idx);
    else
      hdr->last_block_offset = blk_idx;

    std::memset(next_blk, 0, sizeof(BlockT));
    hdr->block_count--;
    hdr->free_count--;
    if (hdr->used_size >= kBlkHdrGran)
      hdr->used_size -= kBlkHdrGran;

    index_type rem = available - new_data_gran;
    if (rem >= kBlkHdrGran + 1) {
      index_type rem_idx = blk_idx + kBlkHdrGran + new_data_gran;
      void *rem_blk = detail::block_at<AddressTraitsT>(base, rem_idx);
      index_type blk_new_next = BlockState::get_next_offset(blk_raw);
      std::memset(rem_blk, 0, sizeof(BlockT));
      BlockState::init_fields(rem_blk, blk_idx, blk_new_next, 1, 0, 0);
      BlockState::set_next_offset_of(blk_raw, rem_idx);
      if (blk_new_next != AddressTraitsT::no_block)
        BlockState::set_prev_offset_of(
            detail::block_at<AddressTraitsT>(base, blk_new_next), rem_idx);
      else
        hdr->last_block_offset = rem_idx;
      hdr->block_count++;
      hdr->free_count++;
      hdr->used_size += kBlkHdrGran;
      FreeBlockTreeT::insert(base, hdr, rem_idx);
    }

    BlockState::set_weight_of(blk_raw, new_data_gran);
    hdr->used_size += (new_data_gran - old_data_gran);
    return true;
  }
};

using DefaultAllocatorPolicy =
    AllocatorPolicy<AvlFreeTree<DefaultAddressTraits>, DefaultAddressTraits>;

}
