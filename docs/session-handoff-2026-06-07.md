# Session Handoff (2026-06-07)

## Branch and Commit

- Branch: `fix/new-can-library`
- Latest committed change: `68a9dbf` (`CAN: migrate to TWAI and harden critical AC/DC reliability`)

## Current Uncommitted Changes

- `lib_common/CAN/CANBus.cpp`
- `lib_common/CAN/CANBus.h`

Purpose of these uncommitted changes:

- Set TWAI interrupt flags to shared low/medium priority:
  - `ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED`
- This addresses DC boot abort:
  - `ESP_ERR_NOT_FOUND` in `twai_driver_install` during interrupt allocation.

## Observed Runtime Symptoms

- DC crash after startup in `twai_driver_install` with `ESP_ERR_NOT_FOUND`.
- AC keeps printing `CAN transmit timeout/fail (W_notAvail=...)` while DC is down.

## Resume Checklist After Reboot

1. Open repository and switch branch:
   - `git checkout fix/new-can-library`
2. Verify state:
   - `git status --short --branch`
3. Build/flash DC first and verify it passes CAN init.
4. Start AC and confirm `W_notAvail` no longer increments continuously.
5. If stable, commit current uncommitted fix with message:
   - `CAN: use shared lowmed TWAI interrupt flags to avoid startup abort`

## Notes

- PlatformIO CLI in this environment previously failed to run (`resultcallback` mismatch), so build validation may need your local known-good setup.
