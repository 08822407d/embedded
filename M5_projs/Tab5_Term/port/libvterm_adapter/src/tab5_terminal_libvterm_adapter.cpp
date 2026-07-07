#include "tab5_terminal_libvterm_adapter.h"

#include <string.h>

namespace tab5term {

TerminalSession::TerminalSession() = default;

TerminalSession::~TerminalSession() { end(); }

bool TerminalSession::begin(int rows, int cols, TerminalHostSink* sink) {
  end();
  if (rows <= 0 || cols <= 0 || sink == nullptr) {
    return false;
  }

  vt_ = vterm_new(rows, cols);
  if (vt_ == nullptr) {
    return false;
  }

  sink_ = sink;
  rows_ = rows;
  cols_ = cols;
  cursor_ = {};
  cursor_.visible = true;

  vterm_set_utf8(vt_, 1);
  vterm_output_set_callback(vt_, &TerminalSession::handleOutput, this);

  screen_ = vterm_obtain_screen(vt_);
  if (screen_ == nullptr) {
    end();
    return false;
  }

  static const VTermScreenCallbacks kScreenCallbacks = {
      TerminalSession::handleDamage,
      nullptr,
      TerminalSession::handleMoveCursor,
      TerminalSession::handleSetTermProp,
      TerminalSession::handleBell,
      TerminalSession::handleResize,
      TerminalSession::handleScrollbackPush,
      TerminalSession::handleScrollbackPop,
      TerminalSession::handleScrollbackClear,
      nullptr,
  };

  vterm_screen_set_callbacks(screen_, &kScreenCallbacks, this);
  vterm_screen_enable_altscreen(screen_, 1);
  vterm_screen_enable_reflow(screen_, true);
  vterm_screen_set_damage_merge(screen_, VTERM_DAMAGE_ROW);
  vterm_screen_reset(screen_, 1);
  vterm_screen_flush_damage(screen_);
  return true;
}

void TerminalSession::end() {
  if (vt_ != nullptr) {
    vterm_free(vt_);
  }
  vt_ = nullptr;
  screen_ = nullptr;
  sink_ = nullptr;
  rows_ = 0;
  cols_ = 0;
  cursor_ = {};
}

void TerminalSession::resize(int rows, int cols) {
  if (vt_ == nullptr || rows <= 0 || cols <= 0) {
    return;
  }
  rows_ = rows;
  cols_ = cols;
  vterm_set_size(vt_, rows, cols);
  if (sink_ != nullptr) {
    sink_->resized(rows, cols);
  }
}

size_t TerminalSession::feedHostBytes(const char* bytes, size_t len) {
  if (vt_ == nullptr || bytes == nullptr || len == 0) {
    return 0;
  }
  const size_t written = vterm_input_write(vt_, bytes, len);
  flushDamage();
  return written;
}

void TerminalSession::flushDamage() {
  if (screen_ != nullptr) {
    vterm_screen_flush_damage(screen_);
  }
}

bool TerminalSession::getCell(int row, int col, TerminalCell* out) const {
  if (screen_ == nullptr || out == nullptr || row < 0 || col < 0 ||
      row >= rows_ || col >= cols_) {
    return false;
  }

  VTermScreenCell cell;
  memset(&cell, 0, sizeof(cell));
  if (!vterm_screen_get_cell(screen_, VTermPos{row, col}, &cell)) {
    return false;
  }

  *out = {};
  out->width = cell.width;
  out->attrs.bold = cell.attrs.bold;
  out->attrs.italic = cell.attrs.italic;
  out->attrs.blink = cell.attrs.blink;
  out->attrs.reverse = cell.attrs.reverse;
  out->attrs.conceal = cell.attrs.conceal;
  out->attrs.strike = cell.attrs.strike;
  out->attrs.underline = static_cast<uint8_t>(cell.attrs.underline);
  out->foreground_is_default = isDefaultFg(cell.fg);
  out->background_is_default = isDefaultBg(cell.bg);
  out->foreground = toRgb(screen_, cell.fg);
  out->background = toRgb(screen_, cell.bg);

  for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i] != 0; ++i) {
    out->codepoints[i] = cell.chars[i];
    out->codepoint_count++;
  }
  return true;
}

void TerminalSession::sendUnicode(uint32_t codepoint,
                                  VTermModifier modifiers) {
  if (vt_ != nullptr) {
    vterm_keyboard_unichar(vt_, codepoint, modifiers);
  }
}

void TerminalSession::sendKey(VTermKey key, VTermModifier modifiers) {
  if (vt_ != nullptr) {
    vterm_keyboard_key(vt_, key, modifiers);
  }
}

void TerminalSession::startPaste() {
  if (vt_ != nullptr) {
    vterm_keyboard_start_paste(vt_);
  }
}

void TerminalSession::endPaste() {
  if (vt_ != nullptr) {
    vterm_keyboard_end_paste(vt_);
  }
}

void TerminalSession::handleOutput(const char* bytes, size_t len, void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self != nullptr && self->sink_ != nullptr && bytes != nullptr && len > 0) {
    self->sink_->writeHostBytes(bytes, len);
  }
}

int TerminalSession::handleDamage(VTermRect rect, void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self != nullptr && self->sink_ != nullptr) {
    self->sink_->invalidate(toRect(rect));
  }
  return 1;
}

int TerminalSession::handleMoveCursor(VTermPos pos, VTermPos,
                                      int visible, void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self != nullptr) {
    self->cursor_.row = pos.row;
    self->cursor_.col = pos.col;
    self->cursor_.visible = visible != 0;
    if (self->sink_ != nullptr) {
      self->sink_->cursorChanged(self->cursor_);
    }
  }
  return 1;
}

int TerminalSession::handleSetTermProp(VTermProp prop, VTermValue* val,
                                       void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self == nullptr || self->sink_ == nullptr || val == nullptr) {
    return 1;
  }

  switch (prop) {
    case VTERM_PROP_TITLE:
      self->sink_->titleChanged(val->string.str, val->string.len);
      break;
    case VTERM_PROP_ALTSCREEN:
      self->sink_->altScreenChanged(val->boolean != 0);
      break;
    case VTERM_PROP_CURSORVISIBLE:
      self->cursor_.visible = val->boolean != 0;
      self->sink_->cursorChanged(self->cursor_);
      break;
    default:
      break;
  }
  return 1;
}

int TerminalSession::handleBell(void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self != nullptr && self->sink_ != nullptr) {
    self->sink_->bell();
  }
  return 1;
}

int TerminalSession::handleResize(int rows, int cols, void* user) {
  auto* self = static_cast<TerminalSession*>(user);
  if (self != nullptr) {
    self->rows_ = rows;
    self->cols_ = cols;
    if (self->sink_ != nullptr) {
      self->sink_->resized(rows, cols);
    }
  }
  return 1;
}

int TerminalSession::handleScrollbackPush(int, const VTermScreenCell*, void*) {
  return 1;
}

int TerminalSession::handleScrollbackPop(int, VTermScreenCell*, void*) {
  return 0;
}

int TerminalSession::handleScrollbackClear(void*) { return 1; }

TerminalRect TerminalSession::toRect(VTermRect rect) {
  return TerminalRect{
      rect.start_row,
      rect.end_row,
      rect.start_col,
      rect.end_col,
  };
}

TerminalRgb TerminalSession::toRgb(const VTermScreen* screen,
                                   VTermColor color) {
  if (screen != nullptr) {
    vterm_screen_convert_color_to_rgb(screen, &color);
  }
  if (VTERM_COLOR_IS_RGB(&color)) {
    return TerminalRgb{color.rgb.red, color.rgb.green, color.rgb.blue};
  }
  return TerminalRgb{};
}

bool TerminalSession::isDefaultFg(const VTermColor& color) {
  return VTERM_COLOR_IS_DEFAULT_FG(&color);
}

bool TerminalSession::isDefaultBg(const VTermColor& color) {
  return VTERM_COLOR_IS_DEFAULT_BG(&color);
}

}  // namespace tab5term
