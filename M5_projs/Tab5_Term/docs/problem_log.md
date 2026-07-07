# Problem Log

## 2026-07-07: Official libvterm Tarball Download Failed In PowerShell

Symptom: downloading
`https://www.leonerd.org.uk/code/libvterm/libvterm-0.3.3.tar.gz` with
`Invoke-WebRequest` failed with a TLS protocol alert:

`Authentication failed because the remote party sent a TLS alert: 'ProtocolVersion'.`

Action taken: cloned `https://github.com/neovim/libvterm.git` with depth 1,
removed its nested `.git`, and kept the resulting source snapshot in
`third_party/libvterm-src/`.

Status: workaround accepted for starting the port quickly. Revisit the source
provenance before release-quality integration.

## 2026-07-07: No Hardware Validation In Current Phase

Symptom: the Tab5 board is not currently available to the user.

Action taken: no build, flash, or runtime claims are made. New code is recorded
as source-level scaffold only.

Status: unresolved until a later device session.

## 2026-07-07: Adapter Callback Access Control

Symptom: the first C++ adapter pass placed a `VTermScreenCallbacks` table at
file scope while callback functions were private static methods. That would
violate C++ access rules.

Action taken: moved the callback table into `TerminalSession::begin()`, where
private static callbacks are accessible.

Status: fixed at source level; not yet compiler-validated.
