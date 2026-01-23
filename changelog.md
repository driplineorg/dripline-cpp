# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Types of changes: Added, Changed, Deprecated, Removed, Fixed, Security

## [Unreleased]


## [2.10.11] -- 2026-01-23

### Changed

- Updated Scarab to v3.14.1


## [2.10.10] -- 2026-01-21

### Changed

- Updated Scarab to v3.14.0 and made downstream changes
- Updated the base Docker image to 3.14.2-slim-trixie


## [2.10.9] - 2025-12-02

### Fixed

- dl-agent was stuck in dry-run mode due to a change in configuration; this is fixed by changing the logic to set the agent in dry-run mode
- A long-standing memory leak in which messages were being kept by the receiver instead of being erased once processed is fixed


## [2.10.8] - 2025-11-04

### Changed

- Updated Scarab to v3.13.4


## [2.10.7] - 2025-10-16

### Changed

- Use rasmus-saks/release-a-changelog-action to create a GitHub release that incorporates the latest changelog information
- Updated Scarab to v3.13.2
- Updated minimum CMake version in the SimpleAmqpClient integration (external/CMakeLists.txt)


## [2.10.6] - 2025-07-10

### Fixed

- Switched integer types for message op and request type back to unsigned ints for compatibility with dl-cpp v2.10.3 and earlier
