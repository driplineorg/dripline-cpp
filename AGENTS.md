# AGENTS.md

## Purpose

This file defines guidance for coding agents acting as C++ developers in this repository.
The goal is to make safe, minimal, style-consistent changes to dripline-cpp.

## Repository Scope

- Main library code: `library/`
- CLI executables: `executables/` (`dl-agent`, `dl-mon`)
- Example services/endpoints: `examples/`
- Unit/integration tests: `testing/`
- Docs sources: `documentation/source/`
- Bundled dependency and build framework: `scarab/`

## Architecture At A Glance

- `core` owns AMQP connectivity and send/listen primitives.
- `message` and derived types (`msg_request`, `msg_reply`, `msg_alert`) implement protocol objects and chunking.
- `receiver` manages chunk assembly; `message_dispatcher` (formerly `concurrent_receiver`) holds the rmqcpp Consumer and dispatches assembled messages.
- `endpoint` implements request dispatch and lockout semantics.
- `service` composes endpoint + message_dispatcher + heartbeater + scheduler.
- `hub` maps message specifiers to user-registered handlers.
- `agent` and `monitor` provide CLI-oriented message send/observe tooling.

## Build And Test Workflow

Preferred local workflow (from repo root):

1. Configure
   - `cmake -S . -B build`
2. Build
   - `cmake --build build -j`
3. Run tests
   - `./build/testing/run_dl_tests`

Common options:

- `-DDripline_ENABLE_TESTING=ON`
- `-DDripline_ENABLE_EXECUTABLES=ON`
- `-DDripline_BUILD_EXAMPLES=ON`
- `-DDripline_BUILD_PYTHON=ON` (only when needed)

If you add new source files, update the corresponding `CMakeLists.txt` target lists.

## Coding Style (Observed In This Codebase)

Follow existing style in the touched file. Do not reformat unrelated code.

### Formatting

- Use 4-space indentation; no tabs.
- Put opening braces on the next line for classes/functions/control blocks.
- Use the project's spacing pattern, e.g. `if( condition )`, `catch( const std::exception& e )`.
- Keep lines reasonably readable; avoid large-scale wrapping churn.

### File Structure

- Header/source pairs use `.hh` and `.cc`.
- Header guards are uppercase with `_HH_` suffix (example pattern: `DRIPLINE_FOO_HH_`).
- Most files include a top block comment with file name, date, author; preserve existing header blocks.

### Includes

- In `.cc` files, include the matching local header first.
- Then include project headers, then external/library headers, then standard headers.
- Preserve the local ordering conventions in each file when editing.

### Namespaces And Types

- Core namespace is `dripline`.
- Prefer existing alias style in a file (`using`, `typedef`) instead of forcing one style.
- Keep API/export macros where used (`DRIPLINE_API`, `DRIPLINE_API_EXPORTS`).

### Class And Member Conventions

- Member fields commonly use `f_` prefix (`f_status`, `f_channel`, etc.).
- Accessor macros from Scarab are widely used (`mv_accessible`, `mv_referrable`, etc.); use them consistently in nearby code.
- Keep move/copy semantics explicit where already established.

### Error Handling And Logging

- Prefer explicit exception types used by this project (`dripline_error`, `connection_error`, AMQP exceptions).
- Preserve message-rich error text using stream-style construction.
- Use logger macros already present in the file (`LOGGER`, `LDEBUG`, `LINFO`, `LWARN`, `LERROR`).

### Const And Parameter Passing

- Prefer `const` correctness and pass heavy objects by `const &`.
- Follow existing pointer ownership style (`std::shared_ptr`, project typedefs).

## Testing Expectations For Changes

- Add or update tests in `testing/` when behavior changes.
- Prefer focused tests near related existing suites (agent/core/service/message/etc.).
- Do not weaken existing assertions to make tests pass.

## Agent Working Rules

- Make minimal, targeted edits.
- Preserve public behavior unless the task explicitly changes behavior.
- Avoid speculative refactors during bug fixes.
- Update docs/comments when behavior or configuration changes.
- Keep cross-component compatibility in mind (`dripline-python`, protocol constants, and wire expectations).

## When Unsure

- Prefer consistency with nearest surrounding code over generic modern C++ style advice.
- If patterns conflict across files, match the pattern used in the file you are editing.