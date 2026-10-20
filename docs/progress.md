# NeuroCube overnight progress

## Authorization and scope

The user approved local tool installation, network access, USB diagnostics, BOOTSEL, resets, replacement firmware, git push and a PR without merging. No battery/charging control, OTP, secure boot or OS security changes.

## Current state

- Working branch: codex/neurocube-mvp, based on bf470ee.
- Device reconfirmed: 2E8A:0009, serial 419CB819454EA93D, CDC COM3 (discovered dynamically).
- Factory CDC read at 115200 produced no output in a three-second window.
- Official Waveshare source and schematic located; pin verification in progress.
- Isolated Python environment created; toolchain installation in progress.
- Portable core and build structure in progress; no firmware written yet.

## Evidence levels

VERIFIED_ON_HARDWARE: USB identification and CDC transport opening only.
NOT_TESTED: display, touch, IMU, gameplay, persistence and battery operation.
