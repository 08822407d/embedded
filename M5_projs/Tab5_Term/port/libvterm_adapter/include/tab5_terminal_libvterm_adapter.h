#pragma once

#include <stddef.h>
#include <stdint.h>

extern "C" {
#include "vterm.h"
}

namespace tab5term {

struct TerminalRect {
  int start_row = 0;
  int end_row = 0;
  int start_col = 0;
  int end_col = 0;
};

struct TerminalRgb {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

struct TerminalCellAttrs {
  bool bold = false;
  bool italic = false;
  bool blink = false;
  bool reverse = false;
  bool conceal = false;
  bool strike = false;
  uint8_t underline = 0;
};

struct TerminalCell {
  uint32_t codepoints[VTERM_MAX_CHARS_PER_CELL] = {};
  uint8_t codepoint_count = 0;
  int width = 1;
  TerminalCellAttrs attrs;
  TerminalRgb foreground;
  TerminalRgb background;
  bool foreground_is_default = true;
  bool background_is_default = true;
};

struct TerminalCursor {
  int row = 0;
  int col = 0;
  bool visible = true;
};

class TerminalHostSink {
 public:
  virtual ~TerminalHostSink() = default;

  virtual void writeHostBytes(const char* bytes, size_t len) = 0;
  virtual void invalidate(const TerminalRect& rect) = 0;

  virtual void cursorChanged(const TerminalCursor& cursor) {}
  virtual void titleChanged(const char* text, size_t len) {}
  virtual void altScreenChanged(bool enabled) {}
  virtual void bell() {}
  virtual void resized(int rows, int cols) {}
};

class TerminalSession {
 public:
  TerminalSession();
  ~TerminalSession();

  TerminalSession(const TerminalSession&) = delete;
  TerminalSession& operator=(const TerminalSession&) = delete;

  bool begin(int rows, int cols, TerminalHostSink* sink);
  void end();

  bool isActive() const { return vt_ != nullptr; }
  int rows() const { return rows_; }
  int cols() const { return cols_; }

  void resize(int rows, int cols);
  size_t feedHostBytes(const char* bytes, size_t len);
  void flushDamage();

  bool getCell(int row, int col, TerminalCell* out) const;
  TerminalCursor cursor() const { return cursor_; }

  void sendUnicode(uint32_t codepoint, VTermModifier modifiers);
  void sendKey(VTermKey key, VTermModifier modifiers);
  void startPaste();
  void endPaste();

 private:
  static void handleOutput(const char* bytes, size_t len, void* user);
  static int handleDamage(VTermRect rect, void* user);
  static int handleMoveCursor(VTermPos pos, VTermPos oldpos, int visible,
                              void* user);
  static int handleSetTermProp(VTermProp prop, VTermValue* val, void* user);
  static int handleBell(void* user);
  static int handleResize(int rows, int cols, void* user);
  static int handleScrollbackPush(int cols, const VTermScreenCell* cells,
                                  void* user);
  static int handleScrollbackPop(int cols, VTermScreenCell* cells, void* user);
  static int handleScrollbackClear(void* user);

  static TerminalRect toRect(VTermRect rect);
  static TerminalRgb toRgb(const VTermScreen* screen, VTermColor color);
  static bool isDefaultFg(const VTermColor& color);
  static bool isDefaultBg(const VTermColor& color);

  VTerm* vt_ = nullptr;
  VTermScreen* screen_ = nullptr;
  TerminalHostSink* sink_ = nullptr;
  int rows_ = 0;
  int cols_ = 0;
  TerminalCursor cursor_;
};

}  // namespace tab5term
