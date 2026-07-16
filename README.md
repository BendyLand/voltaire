# Voltaire Engine

## 2D and 3D cross-platform game engine

**Voltaire** is a lightweight, cross-platform game engine designed to create 2D and 3D games from a unified interface. It provides a comprehensive set of development tools, allowing you to focus on making games without having to reinvent the wheel. Games can be compiled and exported to major desktop platforms (Linux, macOS, Windows), mobile platforms (Android, iOS), as well as Web-based platforms.

## Free and Open Source

Voltaire is completely free and open source under the permissive [MIT License](LICENSE.txt). No strings attached, no royalties, and no licensing fees. Your games are entirely yours, down to the last line of engine code. Voltaire's development is fully independent, empowering you to shape the engine to match your exact expectations.

## Compiling from Source

Voltaire is compiled directly from source to ensure a lean, highly optimized binary tailored to your development environment.

### Prerequisites

You will need:
* **SCons** (Python-based build tool)
* A C++17 compatible compiler (GCC, Clang, or MSVC)

### Compilation

To build the editor binary for your current desktop platform, run the following command in the root directory:

```bash
scons target=editor
```

For detailed compilation instructions on every supported platform, please refer to the build guides included in the repository.

## Contributing

If you are interested in contributing to Voltaire, please see the [contributing guide](CONTRIBUTING.md). This document includes guidelines for reporting bugs, proposing features, and submitting pull requests.

## Documentation

The class reference is fully accessible directly from the built-in editor. For general engine architecture and API workflows, refer to the local documentation files or standard markdown guides provided in this repository.
