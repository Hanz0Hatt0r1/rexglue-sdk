#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <vector>

#include <rex/cvar.h>
#include <rex/input/device_assignment.h>
#include <rex/input/input_system.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/ui/ui_event.h>

using rex::X_RESULT;
using rex::X_STATUS;
using rex::input::InputSystem;
using rex::input::SlotAssignment;
using rex::input::X_INPUT_FLAG_GAMEPAD;
using rex::input::X_INPUT_FLAG_KEYBOARD;
using rex::input::X_INPUT_KEYSTROKE;
using rex::input::X_INPUT_KEYSTROKE_KEYDOWN;
using rex::input::X_INPUT_KEYSTROKE_KEYUP;
using rex::input::X_INPUT_KEYSTROKE_REPEAT;
using rex::input::mnk::MnkInputDriver;

namespace {

class RestoreMnkMode {
 public:
  ~RestoreMnkMode() { rex::cvar::SetFlagByName("mnk_mode", "false"); }
};

}  // namespace

TEST_CASE("MNK keystrokes preserve retail keyboard query semantics", "[input]") {
  REQUIRE(rex::cvar::SetFlagByName("mnk_mode", "true"));
  RestoreMnkMode restore;

  auto driver = std::make_unique<MnkInputDriver>(nullptr, 0);
  auto* mnk = driver.get();
  REQUIRE(mnk->Setup() == X_STATUS_SUCCESS);

  InputSystem input(nullptr);
  input.AddDriver(std::move(driver));
  input.SetDeviceAssignment(std::make_unique<SlotAssignment>());

  X_INPUT_KEYSTROKE keystroke = {};
  REQUIRE(input.GetKeystroke(0, X_INPUT_FLAG_KEYBOARD, &keystroke) == X_ERROR_EMPTY);

  rex::ui::KeyEvent down(nullptr, rex::ui::VirtualKey::kReturn, 0, false, false, false, false,
                         false);
  mnk->OnKeyDown(down);

  // The keyboard event must remain queued when a gamepad-only query arrives.
  CHECK(input.GetKeystroke(0, X_INPUT_FLAG_GAMEPAD, &keystroke) == X_ERROR_DEVICE_NOT_CONNECTED);
  REQUIRE(input.GetKeystroke(0, X_INPUT_FLAG_KEYBOARD, &keystroke) == X_ERROR_SUCCESS);
  CHECK(keystroke.virtual_key == static_cast<uint16_t>(rex::ui::VirtualKey::kReturn));
  CHECK(keystroke.flags == X_INPUT_KEYSTROKE_KEYDOWN);

  rex::ui::KeyEvent repeat(nullptr, rex::ui::VirtualKey::kReturn, 1, true, false, false, false,
                           false);
  mnk->OnKeyDown(repeat);
  REQUIRE(input.GetKeystroke(0, X_INPUT_FLAG_KEYBOARD, &keystroke) == X_ERROR_SUCCESS);
  CHECK(keystroke.flags == (X_INPUT_KEYSTROKE_KEYDOWN | X_INPUT_KEYSTROKE_REPEAT));

  rex::ui::KeyEvent up(nullptr, rex::ui::VirtualKey::kReturn, 0, true, false, false, false, false);
  mnk->OnKeyUp(up);
  REQUIRE(input.GetKeystroke(0, X_INPUT_FLAG_KEYBOARD, &keystroke) == X_ERROR_SUCCESS);
  CHECK(keystroke.flags == X_INPUT_KEYSTROKE_KEYUP);
}

TEST_CASE("MNK drops queued keyboard events when focus is lost", "[input]") {
  REQUIRE(rex::cvar::SetFlagByName("mnk_mode", "true"));
  RestoreMnkMode restore;

  auto driver = std::make_unique<MnkInputDriver>(nullptr, 0);
  auto* mnk = driver.get();
  InputSystem input(nullptr);
  input.AddDriver(std::move(driver));
  input.SetDeviceAssignment(std::make_unique<SlotAssignment>());

  rex::ui::KeyEvent down(nullptr, rex::ui::VirtualKey::kSpace, 0, false, false, false, false,
                         false);
  mnk->OnKeyDown(down);
  rex::ui::UISetupEvent focus_lost(nullptr);
  mnk->OnLostFocus(focus_lost);

  X_INPUT_KEYSTROKE keystroke = {};
  CHECK(input.GetKeystroke(0, X_INPUT_FLAG_KEYBOARD, &keystroke) == X_ERROR_EMPTY);
}
