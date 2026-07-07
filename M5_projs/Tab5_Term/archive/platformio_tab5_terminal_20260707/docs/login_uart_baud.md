# Login UART Baud Management

The formal firmware can change the external Linux login UART at runtime. A
firmware rebuild or reflash is not required after this feature is installed.

## Supported Rates

- `115200` (default)
- `230400`
- `460800`
- `921600`

The tested Module LLM `/dev/ttyS1` link passed real shell round trips at
`460800` and `921600` on 2026-06-08. The installed persistent baseline is now
`921600 8N1` on both endpoints.

## Host Commands

Query the Tab5 state:

```powershell
.\tools\tab5.ps1 baud -Port COM3
```

Temporarily coordinate both Tab5 and the current Linux login shell at 921600:

```powershell
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600
```

Save the Tab5 side in NVS:

```powershell
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600 -Persist
```

Return both sides to 115200 and clear the Tab5 NVS key:

```powershell
.\tools\tab5.ps1 baud -Port COM3 -DefaultBaud
```

`-LinuxTty` overrides the current Module LLM path `/dev/ttyS1`.

## Safety Model

Changing only one endpoint immediately breaks the login link. The normal tool:

1. queries the current Tab5 rate over USB CDC;
2. asks Linux to prove that the target `stty` rate is accepted;
3. schedules the Tab5 switch;
4. changes the Linux TTY and replaces the current login shell with
   `exec bash -l`, so Bash does not restore its old termios state;
5. verifies a shell marker at the target rate;
6. queries the final Tab5 state over USB CDC.

The default change is temporary on Tab5. This is deliberate while the external
Linux boot-time serial configuration remains at 115200: persisting only Tab5
would cause a mismatch after the Linux board reboots.

Use `-Persist` only when the Linux boot configuration is also deliberately
kept at the same rate, or while testing Tab5-only reset persistence.

Current installed baseline, confirmed on 2026-06-08:

- Module LLM M5Bus login UART boot configuration: `921600 8N1`.
- The Module LLM retained the setting after a Linux reboot.
- Tab5 NVS login UART setting: `921600`.
- Tab5 retained the setting after a hard reset and reported
  `baud=921600 persisted=saved`.
- The post-reset strict shell probe returned
  `shell-path-ok: m5stack-LLM`.

## Recovery

USB CDC management is independent of the login UART. If the endpoints are
already mismatched, set only Tab5 to the rate Linux is believed to use:

```powershell
.\tools\tab5.ps1 baud -Port COM3 -Baud 115200 -Tab5Only
```

If Linux is known to remain at 921600:

```powershell
.\tools\tab5.ps1 baud -Port COM3 -Baud 921600 -Tab5Only
```

Then run:

```powershell
.\tools\tab5.ps1 probe -Port COM3
```

The probe now fails unless it captures an executed
`shell-path-ok: <hostname>` result; seeing only the echoed command is not a
pass.

## Firmware Architecture

- `login_uart`: owns `HardwareSerial(1)`, supported rates, delayed runtime
  switching, and optional NVS state.
- `usb_management`: intercepts the private USB CDC frame
  `ESC ] 777 ; ... BEL` before ordinary CDC bytes are forwarded to Linux.
- `tools/login_uart_baud.py`: coordinates the two endpoints and performs
  recovery/query operations.
- `tools/tab5.ps1 baud`: stable project-local command wrapper.

The status bar displays the active login-UART rate after every switch.

## Performance Note

921600 raises the raw UART limit from about 11.5 KB/s to about 92 KB/s, but it
does not guarantee an eightfold visual refresh improvement. The current
true-proportional renderer can repaint the rest of a row for each changed cell,
and terminal input is still parsed byte by byte. If text remains visibly
character-by-character at 921600, profile and batch rendering next; increasing
the UART rate further would not address that bottleneck.
