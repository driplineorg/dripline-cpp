# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Types of changes: Added, Changed, Deprecated, Removed, Fixed, Security

## [Unreleased]

### Changed

- Use rasmus-saks/release-a-changelog-action to create a GitHub release that incorporates the latest changelog information
- Updated Scarab to v3.13.2
- Updated minimum CMake version in the SimpleAmqpClient integration (external/CMakeLists.txt)

## [2.10.6] - 2025-07-10

### Fixed

- Switched integer types for message op and request type back to unsigned ints for compatibility with dl-cpp v2.10.3 and earlier
