#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "platform/input/Mouse.h"
#include "platform/input/Keyboard.h"

bool onMouseDown(int, const EmscriptenMouseEvent* e, void*) {
  Mouse::feed(e->button == 0 ? MouseAction::ACTION_LEFT : MouseAction::ACTION_RIGHT, MouseAction::DATA_DOWN, e->targetX, e->targetY);
  emscripten_request_pointerlock("#canvas", true);
  return true;
}

bool onMouseUp(int, const EmscriptenMouseEvent* event, void*) {
  Mouse::feed(event->button == 0 ? MouseAction::ACTION_LEFT : MouseAction::ACTION_RIGHT, MouseAction::DATA_UP, event->targetX, event->targetY);
  return true;
}

bool onMouseMove(int, const EmscriptenMouseEvent* event, void*) {
  Mouse::feed(0, 0, event->targetX, event->targetY, event->movementX, event->movementY);
  return true;
}

bool onKeyDown(int, const EmscriptenKeyboardEvent* e, void*) {
  if (e->keyCode > 0 && e->keyCode < 256) Keyboard::feed(e->keyCode, true);
  return true;
}

bool onKeyUp(int, const EmscriptenKeyboardEvent* e, void*) {
  if (e->keyCode > 0 && e->keyCode < 256) Keyboard::feed(e->keyCode, false);
  return true;
}

bool onKeyPress(int, const EmscriptenKeyboardEvent* e, void*) {
  if (e->charCode > 31 && e->charCode < 256) Keyboard::feedText(static_cast<char>(e->charCode));
  return true;
}

bool onWindowBlur(int, const EmscriptenFocusEvent*, void*) {
  for (int key = 1; key < 256; ++key)
    if (Keyboard::isKeyDown(key)) Keyboard::feed(static_cast<unsigned char>(key), false);
  Keyboard::reset();
  return true;
}

void hookEmInputs(std::string canvas) {
    emscripten_set_mousedown_callback("#canvas", nullptr, true, onMouseDown);
    emscripten_set_mouseup_callback("#canvas", nullptr, true, onMouseUp);
    emscripten_set_mousemove_callback("#canvas", nullptr, true, onMouseMove);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onKeyDown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onKeyUp);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onKeyPress);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, onWindowBlur);
}