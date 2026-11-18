# 5D Mark IV 1.3.3: SHOOT05 experimental bracket capture

This branch ports the isolated SHOOT05 experiment through AB136-R1 from the
original development tree into the upstream-based fork. It is separate from upstream
`FEATURE_HDR_BRACKETING`, which remains disabled in the 5D4 feature list.
The menu entry is `Advanced Bracket [PREVIEW]`; its `Capture [TEST]`
child can run a gated sequence after closing the ML menu.

## Scope

- Manual photo mode, fixed ISO and single drive; LiveView, movie, Bulb,
  Canon AEB, ML HDR and Intervalometer are excluded by the preflight gate.
- AB133 extends capture from the fixed AB131 three-frame plan to 3–12 frames,
  with the selected EV spacing and sequence. The plan and initial Tv RAW are
  latched and checked before and during the worker run.
- AB134 rounds requested shutter RAW values using the existing ML shutter
  rule, rejects duplicate or invalid targets, and displays the Q count.
- AB135 allows another capture after a fully successful run and verified Tv
  restoration. A failed run remains locked until reboot; there is no
  automatic shutter retry.
- AB136 detects a failed task creation and leaves capture locked. It accepts
  an already-restored Tv only when the observed RAW equals the initial RAW.
- The dry-run controller and manual Tv Plan entries are retained as
  dependencies and diagnostics of the exact experimental source.

## Original camera evidence

The source used for AB131-R2 has SHA-256
`dffc81eed4737a9e69395a16c2bcb43eda33223336804d8bfed497c8d5c429a0`.
Its build artifact has SHA-256
`e492745cd3ce2e9bc2ed4ed9dc8bd5de89bb22814074368260c6067dccddabd1`
(293152 bytes; never committed). The user reported five successful
three-frame sequences on a physical EOS 5D Mark IV running firmware 1.3.3.
The first report records three saved photos, coherent exposure times, Tv
restored to 1/200 s and `3/3 rc:1`. That return code was a reporting bug.

AB131-R3 adds one outcome reset after preflight. Its source has SHA-256
`d8bff77d3a025aaafac9fe1143054e1ad05b8d0e96324286e02ed92f55425b2e`.
The user reported six successful three-frame sequences, with
`3/3 rc:10` on the first. Restore RAW 117 follows from the success code;
the display was not separately reported. EXIF for all frames was not
independently checked, and freeze/Err70 were not separately recorded in the
R3 report. These are hardware checkpoints with explicit limits, not a
general Advanced Bracketing milestone.

### Later original-camera checkpoints

| Candidate | Original `src/shoot.c` SHA-256 | Original `autoexec.bin` SHA-256 | Reported hardware result |
| --- | --- | --- | --- |
| AB133-R1 | `065097739054559cccc30c2ea28ecdbb85e8cfe3ad35c5733396ad0868cd6fd4` | `b28124a56777214f5344604486b724526a46c753d7a52a38738584a61b337628` | 3, 5, 7, 9 and 12 frames passed (reported in AB134 handoff). |
| AB134-R1 | `7b45c609d2d307b090d0df45c249b66c3226493a9680411ffed10c975e48018d` | `52d7ff320c15b28ac319e65101cfc672125881c52501c4945f4d2a1f51a15c98` | 4 frames at 1.25 EV, 10 at 1.625 EV (`rc:10 Q:6`), and 12 at 0.625 EV passed. SD binary identity was not independently verified at checkpoint time. |
| AB135-R1 | `5f0dc90e77f1b31021bfa3c032c732646a2895d8011acd7c583225f4fb486d64` | `e6382680b34aa9c0fc77dab4e3d67cadf89f8410bf67704fa83a6545221a54fe` | Five consecutive series with varied settings passed, without reboot. Per-series diagnostics were not recorded. |
| AB136-R1 | `f7a9ba3aec4990a98c22f2e8bec1a3a71feab22cf67d356796f6f6b60ad654c8` | `87cdba425df28ea0daba2616194b88db5bcdc72db04ae3730547e10c7c5a2197` | 3 frames at 1 EV and 5 frames at 1.25 EV passed consecutively, starting at 1/100 s. Exact per-series diagnostics were not transcribed. |

These are user-reported tests on the EOS 5D Mark IV firmware 1.3.3 of
separate isolated builds. No new fork binary was part of those tests.
For AB136, the task creation failure and already-restored RAW paths were not
forced on camera. No freeze or Err70 was reported; RTC was not separately
recorded. The configuration matrix remains incomplete.

## Fork provenance and validation

The original fork adaptation starts from `5d4/port` at
`ec7ee6a8fc5f5eaf4faee05bdf49dad9eb8c6b92`. It inserts the
SHOOT05-specific functions and menu entries from the AB131-R3 source into
`src/shoot.c`. Subsequent commits adapt AB133, AB134, AB135 and AB136 in
dependency order, with their exact isolated changes inside SHOOT05. Unrelated
differences in the experimental `src/shoot.c` are excluded. The original
source's hardware results do not validate this adapted fork build. A fork
build and physical camera retest remain pending.

The original evidence is in
`/home/corebee/5D4.133-WORKING/SHOOT05-ADVANCED-BRACKET/`, notably
`logs/SHOOT05-AB131-R2-HW2-HW5-20261117-213522-366881.txt` and
`logs/SHOOT05-AB131-R3-HW2-HW6-20261117-215554-369404.txt`.
The source patches are in the corresponding AB131-R2 and AB131-R3
artifact directories. No binary, Canon firmware or dump is included here.

AB132-R1 was an isolated five-frame precursor with a preflight conflict.
Its source is not ported as a separate checkpoint; AB133 implements the
corrected 3–12-frame path. AB134 and AB135 remain distinct prior checkpoints
in this branch history. No generated binary, Canon firmware or dump is
included in the repository. No main promotion, tag or release is planned.
