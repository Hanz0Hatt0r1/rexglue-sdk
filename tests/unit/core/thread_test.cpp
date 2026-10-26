#include <atomic>
#include <chrono>

#include <catch2/catch_test_macros.hpp>
#include <rex/thread.h>

TEST_CASE("resuming a newly created suspended thread starts its routine", "[thread]") {
  constexpr int kIterations = 64;
  for (int iteration = 0; iteration < kIterations; ++iteration) {
    std::atomic<int> starts{0};
    rex::thread::Thread::CreationParameters parameters{};
    parameters.create_suspended = true;
    auto thread = rex::thread::Thread::Create(parameters, [&] {
      starts.fetch_add(1, std::memory_order_release);
    });
    REQUIRE(thread);

    uint32_t previous_suspend_count = 0;
    REQUIRE(thread->Resume(&previous_suspend_count));
    CHECK(previous_suspend_count == 1);
    REQUIRE(rex::thread::Wait(thread.get(), false, std::chrono::seconds(5)) ==
            rex::thread::WaitResult::kSuccess);
    CHECK(starts.load(std::memory_order_acquire) == 1);
  }
}
