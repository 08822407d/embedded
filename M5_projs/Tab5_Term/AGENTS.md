# Project Operating Notes

This repository is now the Tab5 official-firmware terminal port workspace.

Persistent records are required. When changing requirements, architecture,
implementation details, validation status, or known problems, update the
relevant files under `docs/` in the same change:

- `docs/current_work.md` for the active checkpoint and next actions.
- `docs/requirements.md` for product and engineering requirements.
- `docs/decision_log.md` for accepted decisions and reasons.
- `docs/problem_log.md` for issues encountered, evidence, workarounds, and
  unresolved risks.
- `docs/libvterm_port_plan.md` for terminal-core integration design.

Do not rely on chat history for important project state. If a detail matters
for future work, record it in this repository.

The physical Tab5 is currently unavailable. Do not mark build, flash, or
hardware behavior as verified until a later real-device session confirms it.
