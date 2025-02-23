### Added:
- Drag click to select items in address list and scratchpad

### Updated:
- Use simpler and slightly faster `pread()` to read process memory
- Use smaller chunk sizes (128kb instead of 1mb) which seem to search slightly faster

### Fixed:
- Input box for Value +- is usable even when strictly integer types are selected
- Integer search with Value +- near integer limits works correctly instead of rolling over and giving no results
- Live value update thread sleeps proportionately to how long it takes to update values, should use ~5% CPU time on that core
