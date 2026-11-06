# 5D4.133 SHOOT04 — native Bulb Timer and Intervalometer

Status: **RESEARCH UPDATE on the fork**, not a hardware-validated fork release. Target: Canon EOS 5D Mark IV firmware 1.3.3.

## Fork integration

Commit `3b40b1c3cdf832fe3eacf469d66b99f9d97c8009` on `5d4/port` follows `ced8eeab2cf67e70d824c0a27753414044e96e18`. It ports only:

- `platform/5D4.133/cfn.c`: the exact R122N Bulb source snapshot (Git blob `30a850dddcb476a11b3146daec69162f87507b20`), 864 added lines.
- `platform/5D4.133/property_whitelist.h`: the narrow R122AC permission for native Canon Bulb property `0x80040060`, two added lines.

The existing upstream Intervalometer implementation is reused; the broad R122N development-tree patch was **not** applied to this fork. In particular, unrelated GUI, Zebra and compositor changes were excluded. The source retains its historical R093/R109/R116/R120 diagnostic-era names and an eight-write cap; the early “read-only observer” comment does **not** describe the entire later source, which also contains controlled property writes and a one-shot action. Do not treat this as a cleaned-up production implementation.

The fork build returned `BUILD_RC=0`; generated `platform/5D4.133/build/autoexec.bin` was 233440 bytes, SHA-256 `1f1b00f68c5487794489896f2bc95fa879f142f1dd7c54e9607d96dd6ea88431`. The missing `ML-SETUP.FIR` packaging notice was ignored by the Makefile. The binary is build evidence only and is not committed.

## Original hardware evidence — separate development tree

The original SHOOT04 R122-AF workcard records a user-reported **HARDWARE CHECKPOINT / coexistence PASS** for native Bulb Timer and Intervalometer on a physical 5D Mark IV FW 1.3.3. Its candidate binary was 276416 bytes, SHA-256 `0d175c22cddba16aa4427fe2e13bb67a29296f9debe2b8b66ef499835b26ced3`. Its corrected static recheck was PASS.

Evidence provenance: `/home/corebee/5D4.133-WORKING/SHOOT04-INTERVALOMETER-MENU/logs/SHOOT04-R122-AF-MILESTONE-20261103-221430-6715.txt` (SHA-256 `5dba13d45a725668882bc3cf57936ef0d9d4ef96a83a25d5b613abec7162138b`). The original source baseline was local branch `5d4-133-candidate`, HEAD `77bfce0ff4b08b93e0ae6719a883f14ad2ab8a33`.

The exact number of hardware repetitions was not recorded. Freeze and Err70 were not explicitly reported in that result; RTC was not checked and unrelated features were not regression-tested. None of these absences is evidence that such problems cannot occur.

## Remaining validation

**This fork combination has not been run on hardware.** Before promoting it beyond a research update, test boot, native Bulb UI/read/write, one-shot behavior, Intervalometer alone and Bulb/Intervalometer coexistence on FW 1.3.3; record repetitions, freeze/Err70, RTC and relevant GUI regressions. Keep SHOOT05 waveform-cache R22 and R24C as separate dependent checkpoints; R24C has no hardware test. No merge to `main`, tag or release follows from this checkpoint.
