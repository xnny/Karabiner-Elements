#include "../../src/lib/libkrbn/include/libkrbn/libkrbn.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include <pqrs/thread_wait.hpp>
#include <thread>

namespace {
void system_preferences_monitor_callback(const struct libkrbn_system_preferences_properties* properties,
                                         void* refcon) {
  std::cout << "system_preferences_monitor_callback" << std::endl;
  std::cout << "  use_fkeys_as_standard_function_keys: "
            << properties->use_fkeys_as_standard_function_keys
            << std::endl;
}

void connected_devices_monitor_callback(libkrbn_connected_devices* devices,
                                        void* refcon) {
  // Check that get_device_address returns a fixed allocated address.
  auto* p1 = reinterpret_cast<const void*>(libkrbn_connected_devices_get_device_address(devices, 0));
  auto* p2 = reinterpret_cast<const void*>(libkrbn_connected_devices_get_device_address(devices, 0));
  assert(p1 == p2);
}

auto global_wait = pqrs::make_thread_wait();
} // namespace

int main(int argc, const char* argv[]) {
  libkrbn_initialize();

  signal(SIGINT, [](int) {
    global_wait->notify();
  });

  libkrbn_enable_connected_devices_monitor(connected_devices_monitor_callback, nullptr);

  {
    libkrbn_enable_complex_modifications_assets_manager();

    libkrbn_complex_modifications_assets_manager_reload();

    {
      auto size = libkrbn_complex_modifications_assets_manager_get_files_size();
      std::cout << "libkrbn_complex_modifications_assets_manager_get_files_size: " << size << std::endl;

      for (size_t i = 0; i < size; ++i) {
        std::cout << "  " << libkrbn_complex_modifications_assets_manager_get_file_title(i) << std::endl;

        auto rules_size = libkrbn_complex_modifications_assets_manager_get_rules_size(i);
        std::cout << "    libkrbn_complex_modifications_assets_manager_get_rules_size: " << rules_size << std::endl;

        for (size_t j = 0; j < rules_size; ++j) {
          std::cout << "      " << libkrbn_complex_modifications_assets_manager_get_rule_description(i, j) << std::endl;

          if (j >= 2) {
            if (rules_size - j > 1) {
              std::cout << "      ..." << std::endl;
            }
            break;
          }
        }

        if (i >= 2) {
          if (size - i > 1) {
            std::cout << "  ..." << std::endl;
          }
          break;
        }
      }
    }

    libkrbn_disable_complex_modifications_assets_manager();
  }

  std::cout << std::endl;
  for (int i = 0; i < 10; ++i) {
    std::cout << "." << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::cout << std::endl;

  libkrbn_enable_system_preferences_monitor(
      system_preferences_monitor_callback,
      nullptr);

  std::cout << "libkrbn_get_notification_message_body: " << libkrbn_get_notification_message_body() << std::endl;

  std::thread thread([] {
    global_wait->wait_notice();

    libkrbn_terminate();

    CFRunLoopStop(CFRunLoopGetMain());
  });

  // ============================================================

  CFRunLoopRun();

  // ============================================================

  thread.join();

  std::cout << "finished" << std::endl;

  return 0;
}
