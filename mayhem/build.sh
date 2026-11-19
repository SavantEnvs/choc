#!/usr/bin/env bash
#
# mayhem/build.sh — build this repo's fuzz harness(es). EDIT per repo.
#
# Runs inside the commit image (mayhem/Dockerfile) as `mayhem` in /mayhem. The base image
# (ghcr.io/savantenvs/base) already exports the build contract — use these, don't redefine:
#   CC, CXX             stock clang / clang++
#   LIB_FUZZING_ENGINE  -fsanitize=fuzzer   (link into each harness that has a LLVMFuzzer entry)
#   SANITIZER_FLAGS     -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
#                       (ASan + UBSan, both set to HALT — so Mayhem catches memory AND UB defects)
#   DEBUG_FLAGS         -g -gdwarf-3   (DWARF debug info — always on for fuzz/standalone builds,
#                       independent of the sanitizer off-switch; DWARF version must be < 4)
#   RUST_DEBUG_FLAGS    -C debuginfo=2 -C force-frame-pointers=yes -C llvm-args=-gdwarf-3
#                       (thread through RUSTFLAGS on every cargo-fuzz build)
#   GO_DEBUG_FLAGS      -gcflags=all=-N -l
#                       (thread through go build / go-fuzz-build so the linked ELF keeps symbols)
#   SRC                 /mayhem (the repo source)
#
# Contract: build EVERYTHING here — one runnable binary per fuzz harness, AND the project's test
# suite (so mayhem/test.sh only has to RUN it, never compile). Keep it ADDITIVE (build upstream as
# upstream documents; don't edit upstream files). IMPORTANT: build the PROJECT ITSELF with
# $SANITIZER_FLAGS and $DEBUG_FLAGS (not just the harness) so the fuzzed code is instrumented
# AND carries DWARF < 4 symbols — otherwise ASan/UBSan only see the harness, not the library
# you're trying to find bugs in, and backtraces won't resolve project source lines. Build the TEST
# suite with the project's NORMAL flags (a clean, independent build) so test.sh stays an honest
# functional oracle and won't false-fail on benign UB. Leave the test binary/runner where test.sh
# expects it.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

# Build knobs come from the ENVIRONMENT (overridable), with sane defaults — no if-statements,
# just parameter-expansion fallbacks. Default sanitizers are the base's ASan+UBSan (halting); override
# per build via the Dockerfile's `--build-arg SANITIZER_FLAGS="..."`.
# NB: SANITIZER_FLAGS uses `=` (no colon) on purpose — `=` only fills when the var is UNSET, so an
# explicit EMPTY value (`--build-arg SANITIZER_FLAGS=`) is honored and builds with NO sanitizers
# (useful when you want the program's natural crash / full backtrace, not an ASan report). The other
# knobs use `:=` (default on empty too). MAYHEM_JOBS sets build parallelism (falls back to nproc).
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
# DEBUG_FLAGS carries DWARF debug info INDEPENDENTLY of the sanitizer off-switch (so an empty
# SANITIZER_FLAGS still yields DWARF symbols). DWARF MUST be < 4 (Mayhem triage can't read >=4); clang-19's
# plain `-g` emits DWARF-5, so `-gdwarf-3` is explicit. Apply $DEBUG_FLAGS to the fuzz/harness/standalone
# builds (NOT the test/oracle build). Rust/Go carry the same intent via their language flags below.
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${RUST_DEBUG_FLAGS:=-C debuginfo=2 -C force-frame-pointers=yes -C llvm-args=-gdwarf-3}"
: "${GO_DEBUG_FLAGS:=-gcflags=all=-N -l}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
# COVERAGE_FLAGS: empty by default → no effect on the normal oracle build. Set it via the Dockerfile's
# `--build-arg COVERAGE_FLAGS="-fprofile-instr-generate -fcoverage-mapping"` to instrument the TEST
# build for source-coverage measurement (how much of the project the test suite actually exercises —
# a quality signal for the oracle; complements the anti-reward-hack sabotage check). APPEND it to the
# test build's compile+link flags in step 3 (NOT the fuzz build); after `test.sh` runs, merge with
# `llvm-profdata` and report with `llvm-cov`. Empty value is honored (`=`, not `:=`).
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS RUST_DEBUG_FLAGS GO_DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# choc is header-only: the "project" is compiled into each harness TU, so compiling the harness with
# $SANITIZER_FLAGS instruments the library code it reaches. Each harness is compiled once (with
# -fsanitize=fuzzer-no-link for coverage) and linked twice: libFuzzer binary + standalone reproducer.
OUT="$SRC/mayhem/out"
mkdir -p "$OUT"
HARNESSES="json zip inflate deflate_roundtrip wav ogg mp3 midifile value utf8 quickjs duktape base64_text"
CXXFLAGS_FUZZ="-std=c++17 -O1 $SANITIZER_FLAGS $DEBUG_FLAGS -I$SRC -I$SRC/mayhem/harnesses -w"

$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o "$OUT/lsan_off.o"
$CC  $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

build_one() {
  local h="$1"
  $CXX $CXXFLAGS_FUZZ -fsanitize=fuzzer-no-link -c "$SRC/mayhem/harnesses/fuzz_$h.cpp" -o "$OUT/fuzz_$h.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "$OUT/fuzz_$h.o" "$OUT/lsan_off.o" -lpthread -lm -o "$OUT/fuzz_$h"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "$OUT/fuzz_$h.o" "$OUT/standalone_main.o" "$OUT/lsan_off.o" -lpthread -lm -o "$OUT/fuzz_$h-standalone"
}
export -f build_one
export OUT CXXFLAGS_FUZZ
printf '%s\n' $HARNESSES | xargs -P "$MAYHEM_JOBS" -I{} bash -c 'build_one {}'
for h in $HARNESSES; do test -x "$OUT/fuzz_$h" && test -x "$OUT/fuzz_$h-standalone"; done

# The oracle: upstream's unit tests (tests/choc_tests.h) driven by mayhem/choc_selftest.cpp, built
# with NORMAL flags (no sanitizers), linking gtk+-3.0/webkit2gtk-4.1 as upstream's Linux build does
# (choc_tests.h includes the WebView and MessageLoop headers).
# Compiled WITHOUT SOURCE_DATE_EPOCH: upstream's BuildDate test (testPlatform) asserts that
# __DATE__/__TIME__ is recent and not exactly midnight, and clang derives both from the epoch, so a
# build replayed with a past epoch would fail the oracle on correct code. Only this binary is exempt;
# the fuzz/standalone binaries above keep the epoch and stay reproducible.
env -u SOURCE_DATE_EPOCH $CXX -std=c++20 -O1 $COVERAGE_FLAGS -w "$SRC/mayhem/choc_selftest.cpp" \
     $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1) -lpthread -o "$OUT/choc_selftest"
test -x "$OUT/choc_selftest"
