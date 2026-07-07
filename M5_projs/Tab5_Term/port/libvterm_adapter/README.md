# libvterm Adapter

This directory is the first bridge between the Tab5 official firmware and
`libvterm`.

It is deliberately GUI-neutral:

- transport code feeds host bytes into `TerminalSession`;
- `libvterm` owns parsing and screen state;
- callbacks report dirty rectangles and host-output bytes;
- the official firmware GUI later decides how to draw cells in a popup.

No hardware build has been attempted for this adapter yet.
