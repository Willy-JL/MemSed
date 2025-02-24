### Added:
- Drag click to select items in address list and scratchpad
- Confirm dialog box when detaching process with an active search or addresses in scratchpad
- Close popups with X buttons, pressing Escape, and clicking outside th popup

### Updated:
- Use custom `thread_cancel()` implementation that is more portable and works with `-fexceptions`
- Use simpler and slightly faster `pread()` to read process memory
- Use smaller chunk sizes (128kb instead of 1mb) which seem to search slightly faster

### Fixed:
- Input box for Value +- is usable even when strictly integer types are selected
- Integer search with Value +- near integer limits works correctly instead of rolling over and giving no results
- Live value update thread sleeps proportionately to how long it takes to update values, should use ~5% CPU time on that core
- Decide to search with certain value type including deviation edges
