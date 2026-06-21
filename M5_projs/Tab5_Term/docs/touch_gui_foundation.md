# Touch And GUI Foundation

This document designs the foundation for Tab5 touch interaction and small GUI
controls in the terminal firmware. It is a design record, not an implemented
feature. Keep touch work incremental so the accepted UART terminal, keyboard
input, status bar, and Stage 10 render pipeline remain stable.

## Goals

- Add touch handling as a first-class local input path without mixing it with
  remote terminal byte streams.
- Provide reusable GUI controls, starting with a rectangular button, so future
  features do not hand-roll hit testing and pressed-state drawing.
- Keep UI rendering bounded and dirty-region based.
- Use English/ASCII for local firmware UI labels by default unless the user
  explicitly asks for Chinese. The terminal character area is excluded from
  this rule: it displays host terminal output as received and must not translate
  text.
- Preserve the current status bar, terminal viewport, keyboard-mounted
  orientation, and `69x32` terminal geometry unless a future stage explicitly
  changes them.
- Leave terminal mouse-reporting as a later feature. GUI controls are local
  firmware UI; xterm mouse/touch reports to the host are a separate contract.

## Non-Goals

- Do not bring in a full GUI framework before the project needs one.
- Do not let local UI touches enter the Linux login shell unless a control is
  explicitly designed to emit terminal input.
- Do not make terminal cell rendering depend on the GUI widget tree.
- Do not redraw the whole screen for every touch sample.

## Existing Touch Source

M5Unified exposes `M5.Touch`, updated from `M5.update()`. The underlying
`Touch_Class` tracks up to five points and reports converted display
coordinates through `touch_detail_t`.

Useful detail methods include:

- `wasPressed()` for touch begin.
- `isPressed()` for current contact.
- `wasReleased()` and `wasClicked()` for release/click.
- `wasHold()` and `isHolding()` for long press.
- `wasDragStart()`, `isDragging()`, and `wasDragged()` for movement.
- `deltaX()/deltaY()` and `distanceX()/distanceY()` for motion.
- `id` for stable multi-touch identity.

Before accepting any touch UI stage, run a diagnostic that logs a few points in
the configured keyboard-mounted orientation and confirms that reported
coordinates match display pixels after `ui::applyConfiguredDisplayOrientation()`.

## Layers

Use three layers:

```text
M5.Touch / M5.update()
  -> touch_input: normalized pointer events and gesture state
  -> gui: widget tree, hit testing, capture, invalidation, drawing
  -> feature callbacks: local actions such as menu, baud dialog, power dialog
```

### touch_input

`touch_input` should translate M5Unified touch details into a small project
event model:

```cpp
namespace touch {

enum class Phase : uint8_t {
    Down,
    Move,
    Up,
    Cancel,
    Hold,
};

struct Point {
    int16_t x;
    int16_t y;
};

struct Event {
    uint8_t pointer_id;
    Phase phase;
    Point position;
    Point start_position;
    Point delta;
    uint32_t time_ms;
    uint8_t click_count;
};

void begin();
size_t pollEvents(Event *events, size_t capacity);

} // namespace touch
```

Guidelines:

- Poll after `M5.update()` in `src/main.cpp`.
- Keep per-loop event count bounded, similar to keyboard events.
- Preserve pointer ids so GUI capture works with multi-touch even if early GUI
  controls only use one pointer.
- Emit `Cancel` if a tracked point disappears unexpectedly or a modal/widget is
  removed while pressed.
- Do not call terminal input mapping from this layer.

### gui

`gui` owns local controls and hit testing:

```cpp
namespace gui {

struct Rect {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
};

enum class EventResult : uint8_t {
    Ignored,
    Handled,
    Captured,
};

class Widget {
public:
    virtual Rect bounds() const = 0;
    virtual bool visible() const = 0;
    virtual bool enabled() const = 0;
    virtual EventResult handleTouch(const touch::Event& event) = 0;
    virtual void draw(bool force) = 0;
};

void begin();
void addRoot(Widget *widget);
void removeRoot(Widget *widget);
void update();
void invalidate(const Rect& rect);
void drawDirty();

} // namespace gui
```

Guidelines:

- Hit test topmost controls first.
- Track one captured widget per pointer id. While captured, subsequent move/up
  events go to that widget even when the pointer moves outside its rectangle.
- Redraw only invalidated control rectangles. Full-screen redraw remains an
  explicit recovery/debug action.
- Keep widgets out of the terminal content area by default. Overlays that cover
  the terminal must be modal and must mark their covered area dirty when closed.
- GUI drawing must restore terminal text style after drawing, following the
  pattern in `status_bar.cpp`.

## Button Control

The first control should be a rectangular button. It provides the behavior that
future controls can reuse.

### State

```cpp
enum class ButtonState : uint8_t {
    Idle,
    PressedInside,
    PressedOutside,
    Disabled,
};
```

Recommended state fields:

- `Rect rect`
- `const char *label`
- `bool enabled`
- `bool visible`
- `uint8_t active_pointer_id`
- `bool has_capture`
- callbacks: `onPress`, `onRelease`, `onClick`, `onHold`, `onCancel`

### Touch Behavior

Button behavior should match common commercial touch UI:

1. Down inside the button:
   - capture the pointer;
   - enter `PressedInside`;
   - redraw in pressed style;
   - optionally call `onPress`.
2. Move while captured:
   - if the pointer remains inside, stay `PressedInside`;
   - if it moves outside by more than a small slop threshold, enter
     `PressedOutside` and redraw as unpressed/cancel-ready;
   - moving back inside restores `PressedInside`.
3. Hold while captured and inside:
   - call `onHold` once if the button supports it;
   - do not also emit a normal click unless the control explicitly asks for
     repeated actions.
4. Up while captured:
   - if inside and not consumed by hold, call `onClick`;
   - call `onRelease`;
   - release capture;
   - return to `Idle` and redraw.
5. Up outside or cancel:
   - call `onCancel`;
   - release capture;
   - return to `Idle` and redraw.

The important rule is capture: a button that was pressed should keep receiving
events until release/cancel. Without capture, sliding a finger outside and back
inside produces inconsistent UI.

### Drawing

Use the existing theme instead of ad hoc colors. Add GUI colors to
`TerminalTheme` when the first control is implemented:

- `button_background`
- `button_pressed_background`
- `button_border`
- `button_text`
- `button_disabled_background`
- `button_disabled_text`

Button drawing should:

- fill its own rect;
- draw a simple border;
- center the label;
- use stable dimensions; touch state must not resize the control;
- avoid covering the battery/status area unless the control is explicitly part
  of the status bar;
- restore the previous text datum/cursor and terminal text style afterward.

### LVGL-Compatible Configuration Shape

Keep local controls close to LVGL concepts even while M5GFX is the only
renderer:

- A control has a spec object that owns geometry, text metrics, item/action
  metadata, and scale.
- Standard geometry is kept separately from scale, so a visual size change does
  not erase the base value.
- Runtime state such as `Default` and `Pressed` is separate from the spec.
- Drawing resolves a style from `(part, state)` at render time instead of
  scattering color and size conditionals through event handlers.
- Event handlers update state or select an action; they should not own visual
  constants.

This maps cleanly onto LVGL's model later: the spec becomes object creation and
properties, the resolved styles become `lv_style_t` applied to parts and
states, and the action callbacks become LVGL event callbacks.

## Placement Strategy

Start with a non-overlapping local UI area before trying terminal overlays:

- Status-bar action buttons are safest because the status bar is already local
  firmware UI. Example future controls: menu, power, baud.
- Terminal-overlaid controls need a modal overlay manager and restore/redraw
  rules because they cover terminal pixels.
- Touch-to-terminal mouse reporting should wait until local GUI controls have
  proven the coordinate and capture pipeline.

## Main Loop Integration

Add only bounded calls to `src/main.cpp`:

```cpp
M5.update();
touch::pollEvents(...);
gui::dispatch(...);
gui::drawDirty();
```

The exact placement should preserve current responsiveness:

- `M5.update()` remains periodic.
- Touch polling follows `M5.update()`.
- GUI dispatch should process only a small fixed number of events per loop.
- GUI drawing should happen after terminal/status updates and must stay
  bounded; large modal redraws should be split or explicitly forced.

## Diagnostics And Validation

Add diagnostic-first tooling before enabling user-facing controls:

- A touch diagnostic build or CDC command that reports `x,y,id,state` for a few
  touches.
- A framebuffer capture of one button in idle, pressed, and disabled states.
- A simple manual validation checklist:
  - tap inside button triggers one click;
  - press, drag outside, release does not click;
  - press, drag outside, drag back inside, release clicks;
  - long press produces exactly one hold action where enabled;
  - status bar and terminal text style remain correct after redraw;
  - keyboard input and UART output remain responsive during touch interaction.

## Current Implementation Note

The first runtime touch control is deliberately narrower than the full
foundation proposed here. The status-bar battery/power area now owns a small
local menu in `src/status_bar.cpp`:

- tapping the battery/power area opens a menu below the status bar;
- the menu contains `Power Off` and `Restart`;
- the menu is configured by a `PowerMenuSpec` with standard geometry, shared
  scale, item/action metadata, text metrics, and state-derived drawing style;
- menu item labels are vertically centered and inset by `2 * TERMINAL_CELL_WIDTH`
  from the left edge;
- menu item touches use pointer capture, pressed highlighting, drag-out cancel,
  and release-inside activation;
- `Power Off` calls `M5.Power.powerOff()`;
- `Restart` calls `ESP.restart()`;
- closing the overlay clears its rectangle and calls `terminal::redraw()`.

This proves the touch/control behavior on the real UI surface, but the generic
`touch_input` and `gui` modules are still future work.

The central terminal character area is also now wrapped as a UI-facing control
in `terminal_view`. It is not a touch widget yet, but it gives future GUI work
one place to change the terminal bounds, centering, background clearing, and
redraw behavior before a broader GUI manager or LVGL port exists.

## Suggested Stage

Open a new stage for this work rather than extending Stage 10:

### Stage 11: Local Touch GUI Foundation

Milestones:

- G1: touch coordinate diagnostic and normalized `touch_input` event model.
- G2: minimal `gui` manager with hit testing, pointer capture, dirty redraw,
  and one button widget.
- G3: migrate the status-bar power menu behavior onto the generic manager, or
  add one harmless diagnostic button first if the migration needs a lower-risk
  stepping stone.
- G4: optional modal overlay pattern for future baud/menu dialogs.

Exit condition: a button can be drawn, pressed, cancelled by dragging out,
clicked by releasing inside, and redrawn without disturbing the terminal,
keyboard input, status bar, or login-UART path.
