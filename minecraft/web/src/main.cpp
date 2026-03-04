#include <string>
#include <fstream>
#include <cstdlib>

#include <png.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "NinecraftApp.h"
#include "AppPlatform.h"
#include "client/gui/screens/DialogDefinitions.h"
#include "client/renderer/TextureData.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"

#include "texture.h"
#include "keyboard.h"

namespace {
  typedef DialogDefinitions DDef;

static void png_funcReadFile(png_structp pngPtr, png_bytep data, png_size_t length) {
  ((std::istream*)png_get_io_ptr(pngPtr))->read((char*)data, length);
}

constexpr int kCanvasWidth = 1280, kCanvasHeight = 720;

class AppPlatform_web final : public AppPlatform {
 public:

  void showDialog(int id) override { dialogId = id; }

  bool useDialog(int wantId, std::vector<std::string> prompts, std::vector<std::string> defaults = {}) {
    if (dialogId != wantId) return false;

    for (int i = 0; i < (int)prompts.size(); ++i) {
      char* ptr = reinterpret_cast<char*>(EM_ASM_PTR({
        const ans = window.prompt(UTF8ToString($0), UTF8ToString($1));
        const ptr = _malloc(64);
        stringToUTF8(ans, ptr, 64);
        return ptr
      }, prompts[i].c_str(), defaults[i].c_str()));

      inputs.push_back(ptr);
      free(ptr);
    }

    return true;
  }

  void createUserInput() override {
    status = 0;
    inputs.clear();

    if (useDialog(DDef::DIALOG_CREATE_NEW_WORLD, {"name", "seed", "mode"}, {"world", "", "creative"})) status = 1;
    else if (useDialog(DDef::DIALOG_NEW_CHAT_MESSAGE, {"message"})) status = 1;
    else if (useDialog(DDef::DIALOG_RENAME_MP_WORLD, {"new name"})) status = 1;
  }

  int getUserInputStatus() override {
    int out = status;
    status = -1;
    return out;
  }

  StringVector getUserInput() override { return inputs; }

  TextureData loadTexture(const std::string& file, bool folder) override { return loadTextureHack(file, folder); }

  int getScreenWidth() override { return kCanvasWidth; }
  int getScreenHeight() override { return kCanvasHeight; }
  float getPixelsPerMillimeter() override { return 8.0f; }
  bool supportsTouchscreen() override { return false; }
  bool isNetworkEnabled(bool) override { return true; }
  std::string getDateString(int s) override { return std::to_string(s); }
  int checkLicense() override { return 1; }

 private:
  int dialogId = -1;
  int status = -1;
  StringVector inputs;
};

AppPlatform_web platform;
AppContext context;
NinecraftApp* app = nullptr;
bool booted = false;

void tick() {
  if (!booted) {
    app = new NinecraftApp();
    app->externalStoragePath = "/persist";
    app->externalCacheStoragePath = "/persist";
    static_cast<App*>(app)->init(context);
    app->options.useTouchScreen = false;
    app->options.isJoyTouchArea = false;
    app->reloadOptions();
    app->options.serverVisible = true;
    app->setSize(kCanvasWidth, kCanvasHeight);
    booted = true;
  } else app->update();
}

}  // namespace

int main() {
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);

  emscripten_set_canvas_element_size("#canvas", kCanvasWidth, kCanvasHeight);
  auto glContext = emscripten_webgl_create_context("#canvas", &attrs);
  emscripten_webgl_make_context_current(glContext);

  EM_ASM({
    Browser.useWebGL = true;
    GLImmediate.init();
    GL.generateTempBuffers(true, GL.currentContext)
  });

  context.platform = &platform;
  context.doRender = true;

  hookEmInputs("#canvas");

  EM_ASM({
    Module['canvas']?.addEventListener('contextmenu', e => e.preventDefault());
    FS.mkdir('/persist');
  });

  emscripten_set_main_loop(tick, 0, true);
  return 0;
}
