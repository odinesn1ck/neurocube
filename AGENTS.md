# NeuroCube engineering rules

Use C++17 and Clean Architecture. Dependencies point inward: domain defines ports and values; application orchestrates; games contain portable rules; presentation renders through a port; infrastructure alone imports Pico SDK. Never import hardware headers in domain, application, games or presentation.

Run native tests before firmware integration. Report actual evidence using VERIFIED_ON_HARDWARE, VERIFIED_IN_SIMULATOR, COMPILED_ONLY and NOT_TESTED. USB enumeration does not prove display, touch, motion or battery operation.

Target only Waveshare RP2350-Touch-LCD-1.54. Check the official schematic/examples before changing pins. Never use RESET as a game button. Battery latch GPIO28 HIGH and read-only GPIO24/17/29 diagnostics are authorized by docs/codex-final-polish-2026-10-21.md and explicit user approval. Follow the verified schematic and factory sequence. Do not change charger configuration, OTP, security configuration or OS protections. Never reset/flash arbitrary USB devices. Require explicit flash authorization and match serial identity across modes. Do not commit credentials, factory backups or build binaries.

Keep work on codex/neurocube-mvp, push incremental commits, create a PR but do not merge or force push. Record progress and unresolved hardware checks in docs/progress.md. Native and firmware builds share the same core sources. Prefer small interfaces and static game registration over frameworks.
