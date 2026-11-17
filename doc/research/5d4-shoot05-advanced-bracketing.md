# 5D Mark IV 1.3.3: SHOOT05 experimental bracket capture

This branch ports the isolated SHOOT05 AB131-R3 experiment from the original
development tree into the upstream-based fork. It is separate from upstream
`FEATURE_HDR_BRACKETING`, which remains disabled in the 5D4 feature list.
The menu entry is `Advanced Bracket [PREVIEW]`; its `Capture 3 [TEST]`
child can run one fixed sequence after closing the ML menu.

## Scope

- Manual photo mode, initial shutter Tv RAW 117 (1/200 s), fixed ISO and
  single drive; LiveView, movie, Bulb, Canon AEB, ML HDR and Intervalometer
  are excluded by the experiment's preflight gate.
- Three exposures in the fixed order 0, -1, +1 EV, with Tv RAW targets
  117, 125 and 109. The worker runs once per boot and makes no automatic
  retry. It restores Tv to RAW 117 when its readiness checks permit.
- The preview controls show more frame counts and EV spacings, but those
  settings do not extend the three-frame capture test.
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

## Fork provenance and validation

This fork adaptation starts from `5d4/port` at
`ec7ee6a8fc5f5eaf4faee05bdf49dad9eb8c6b92`. It inserts the
SHOOT05-specific functions and menu entries from the AB131-R3 source into
`src/shoot.c`; it does not transfer the experimental tree's unrelated
changes. The original source's hardware results do not validate this
adapted fork build. A fork build and physical camera retest remain pending.

The original evidence is in
`/home/corebee/5D4.133-WORKING/SHOOT05-ADVANCED-BRACKET/`, notably
`logs/SHOOT05-AB131-R2-HW2-HW5-20261117-213522-366881.txt` and
`logs/SHOOT05-AB131-R3-HW2-HW6-20261117-215554-369404.txt`.
The source patches are in the corresponding AB131-R2 and AB131-R3
artifact directories. No binary, Canon firmware or dump is included here.

AB132-R1 extends the sequence to five frames in a later isolated build.
It is outside this port and had no hardware result at the time of this
checkpoint.
