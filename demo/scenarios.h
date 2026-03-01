/**
 * @file scenarios.h
 * @brief Load test scenario definitions for the PersistMemoryManager demo.
 *
 * Declares the base Scenario interface, ScenarioParams, and ScenarioCoordinator
 * used by all scenario implementations. Each scenario runs in its own thread and
 * exercises different allocation patterns to demonstrate PMM behaviour.
 *
 * Phase 10 addition: ScenarioCoordinator allows the PersistenceCycle scenario to
 * safely pause all other scenarios while performing destroy()/reload() of the PMM
 * singleton (fixes plan.md Risk #5).
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace demo
{

/**
 * @brief Tunable parameters passed to every scenario at runtime.
 */
struct ScenarioParams
{
    std::size_t min_block_size  = 64;
    std::size_t max_block_size  = 4096;
    float       alloc_freq      = 1000.0f; ///< target allocations per second
    float       dealloc_freq    = 900.0f;  ///< target deallocations per second
    int         max_live_blocks = 100;
};

/**
 * @brief Coordinates safe PMM singleton replacement across concurrent scenarios.
 *
 * When the PersistenceCycle scenario needs to call PersistMemoryManager::destroy()
 * and reload, it must ensure no other scenario thread is using the PMM singleton.
 * ScenarioCoordinator implements a two-phase pause/resume protocol with
 * acknowledgment counting:
 *
 *  1. register_participant() — each non-PersistenceCycle thread registers itself.
 *  2. unregister_participant() — called when a thread exits.
 *  3. pause_others() — sets a pause flag and waits until all registered threads
 *     have acknowledged the pause by calling yield_if_paused() or have exited.
 *  4. resume_others() — clears the pause flag and wakes all waiting threads.
 *  5. yield_if_paused() — called at safe points; increments ack counter, then
 *     blocks until resume_others() is called.
 *
 * Thread-safety: all methods are safe to call from any thread.
 */
class ScenarioCoordinator
{
  public:
    ScenarioCoordinator()  = default;
    ~ScenarioCoordinator() = default;

    // Non-copyable, non-movable.
    ScenarioCoordinator( const ScenarioCoordinator& )            = delete;
    ScenarioCoordinator& operator=( const ScenarioCoordinator& ) = delete;
    ScenarioCoordinator( ScenarioCoordinator&& )                 = delete;
    ScenarioCoordinator& operator=( ScenarioCoordinator&& )      = delete;

    /**
     * @brief Register a participant thread (non-PersistenceCycle scenario).
     *
     * Must be called once per thread before entering its main loop.
     * Increments the total participant count used by pause_others() to
     * determine when all threads have acknowledged the pause.
     */
    void register_participant();

    /**
     * @brief Unregister a participant thread (called when thread is exiting).
     *
     * Decrements the total participant count. If a pause is in progress this
     * also increments the acknowledgment counter so pause_others() is not
     * left waiting forever for an exited thread.
     */
    void unregister_participant();

    /**
     * @brief Request all other scenarios to pause and wait until they have.
     *
     * Sets the pause flag, then blocks until every registered participant has
     * acknowledged the pause via yield_if_paused() (or has unregistered).
     * The caller (PersistenceCycle) must call resume_others() when done.
     *
     * @param stop_flag  Caller's cancellation flag; returns early if set.
     */
    void pause_others( const std::atomic<bool>& stop_flag );

    /**
     * @brief Resume all paused scenarios.
     *
     * Clears the pause flag and notifies all blocked scenarios to continue.
     */
    void resume_others();

    /**
     * @brief Block the calling thread if a pause has been requested.
     *
     * Called by non-coordinator scenarios at safe points (after completing
     * an allocation/deallocation, before starting the next one).  Returns
     * immediately when no pause is active. If stop_flag is true the function
     * returns without blocking (to allow fast shutdown).
     *
     * @param stop_flag  The scenario's cooperative-cancellation flag.
     */
    void yield_if_paused( const std::atomic<bool>& stop_flag );

    /// Returns true if a pause is currently in effect.
    bool is_paused() const noexcept;

  private:
    std::atomic<bool>       paused_{ false };
    std::mutex              mutex_;
    std::condition_variable cv_;                ///< notified on resume
    std::condition_variable cv_ack_;            ///< notified when ack_count_ changes
    int                     participants_{ 0 }; ///< registered participant count
    int                     ack_count_{ 0 };    ///< threads that acknowledged pause
};

/**
 * @brief Abstract base class for a load scenario.
 *
 * Implementors override name() and run(). The run() method is called on a
 * dedicated thread; it must check stop_flag frequently and return promptly
 * once stop_flag is set.
 */
class Scenario
{
  public:
    virtual ~Scenario() = default;

    /// Human-readable scenario name shown in the UI.
    virtual const char* name() const = 0;

    /**
     * @brief Execute the scenario loop.
     *
     * @param stop_flag    Set to true by ScenarioManager to request shutdown.
     * @param op_counter   Atomically increment for every allocation/deallocation.
     * @param params       Tunable parameters (read-only snapshot taken at start).
     * @param coordinator  Shared coordinator for safe PMM singleton replacement.
     */
    virtual void run( std::atomic<bool>& stop_flag, std::atomic<uint64_t>& op_counter, const ScenarioParams& params,
                      ScenarioCoordinator& coordinator ) = 0;
};

} // namespace demo
