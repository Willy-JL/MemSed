### Added:
- Drag click to select items in address list and scratchpad
- Confirm dialog box when detaching process / closing MemSed with an active search or addresses in scratchpad
- Close popups with X buttons, pressing Escape, and clicking outside th popup
- Good amount of keyboard shortcuts allowing for almost keyboard-only usage, for example:
  - Ctrl+O to open process select
  - Type process name
  - Press Enter, select with Arrows, press Enter
  - Ctrl+F to focus Value field, type search Value
  - Use Tab and Arrows to navigate and configure other options
  - Ctrl+Enter to search
  - Ctrl+Space to stop Search, Ctrl+Z to undo/reset search
  - Ctrl+D to Detach process
  - Ctrl+Q to Quit MemSed

### Updated:
- Use custom `thread_cancel()` implementation that is more portable and works with `-fexceptions`
- Use simpler and slightly faster `pread()` to read process memory
- Use smaller chunk sizes (128kb instead of 1mb) which seem to search slightly faster

### Fixed:
- Input box for Value +- is usable even when strictly integer types are selected
- Integer search with Value +- near integer limits works correctly instead of rolling over and giving no results
- Live value update thread sleeps proportionately to how long it takes to update values, should use ~5% CPU time on that core
- Decide to search with certain value type including deviation edges
