# NeuroCube overnight progress

## Completed implementation

- Clean Architecture core, replaceable Renderer and RecordStore; 16-slot static registry and paged menu.
- Russian 240x240 UI, rules, pause/restart/next/menu, three difficulties and calibration.
- Sequence, maze and command games; separate Magic Ball mode.
- Two-sector CRC records journal, recovery from damaged writes, schema version 2 with version 1 reads.
- Waveshare HAL for LCD, CST816 touch, QMI8658 IMU and GPIO25 button. Battery/charging pins untouched.
- Hardware Bridge: detect, info, logs, build, flash, selftest, records, screenshot; strict UF2 and USB identity checks.
- Windows simulator and pinned bootstrap; GitHub Actions workflow prepared in docs/ci/build.yml.
- BLOCKED_EXTERNAL_PERMISSION: GitHub rejected the workflow upload because the PAT lacks workflow scope. Code publishing continues; CI is not active. Only the unpublished local commit was recreated without the protected workflow path; no remote history was rewritten.

## Verification so far

- VERIFIED_IN_SIMULATOR: 467 native checks, including full 32-step sequence, maze victory, Commands completion, pause/navigation, gestures, record corruption recovery and registry extension.
- VERIFIED_IN_SIMULATOR: 11 Python bridge tests; 600-frame simulator run and Russian menu rendering.
- VERIFIED_ON_HARDWARE: runtime identity, programmatic BOOTSEL, complete factory backup with verification, firmware load/readback verification, runtime hello.
- VERIFIED_ON_HARDWARE: QMI8658 identity 5 and samples, touch identity 0xB6, no I2C errors in sampled telemetry, framebuffer capture over CDC.
- NOT_TESTED: visible LCD image, physical touch/button/motion, battery operation.

## Work in progress

Debug firmware integration testing with synthetic input, Flash persistence across reboot, final release firmware, CI run and PR. Synthetic input is never presented as physical control verification.

## Recovery

Factory backup is outside git in %USERPROFILE%/neurocube-tools/backups/factory-419CB819454EA93D.uf2, SHA256 5871beca6837ba3758b6c0bf6a0408e0e2aa84e53888e290a29d9e67635a7b66. The device serial is checked before and after BOOTSEL. No OTP, secure boot, charging or battery control modifications.

## Remaining human acceptance

Follow docs/morning-checklist.md. Gift acceptance requires real operation of all three games, Magic Ball, records after reboot and battery autonomy; this has not yet been established.
