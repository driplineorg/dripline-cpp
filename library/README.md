# dripline-cpp/library

This directory contains all of the source files for the dripline-cpp library.

## Key Classes

| Class | Files | Description |
|---|---|---|
| `core` | `core.hh` / `core.cc` | AMQP connectivity, send primitives, VHost/Topology management |
| `message`, `msg_request`, `msg_reply`, `msg_alert` | `message.hh` / `message.cc` | Protocol objects, serialization, chunking |
| `receiver` | `receiver.hh` / `receiver.cc` | Chunk assembly, incoming-message map, stale-entry eviction, `wait_for_reply()` |
| `message_dispatcher` | `message_dispatcher.hh` / `message_dispatcher.cc` | rmqcpp Consumer lifecycle (`start_listening` / `stop_listening`), dispatches assembled messages to `submit_message()`. Formerly named `concurrent_receiver`. |
| `endpoint` | `endpoint.hh` / `endpoint.cc` | Request dispatch, lockout semantics |
| `endpoint_listener_receiver` | `endpoint.hh` / `endpoint.cc` | Decorator: wraps an `endpoint` with `message_dispatcher` capabilities for async child use |
| `service` | `service.hh` / `service.cc` | Composes `core` + `endpoint` + `message_dispatcher` + `heartbeater` + `scheduler` |
| `monitor` | `monitor.hh` / `monitor.cc` | Composes `core` + `message_dispatcher`; prints received messages |
| `hub` | `hub.hh` / `hub.cc` | Maps specifiers to user-registered handler functions |
| `agent` | `agent.hh` / `agent.cc` | CLI-oriented one-shot message sender |
| `heartbeater` | `heartbeater.hh` / `heartbeater.cc` | Periodic heartbeat sender |
| `scheduler` | `scheduler.hh` | Event scheduler |
| `relayer` | `relayer.hh` / `relayer.cc` | Forwards messages between services |
| `reply_cache` | `reply_cache.hh` / `reply_cache.cc` | Python-binding reply cache (built only with `Scarab_BUILD_PYTHON`) |
