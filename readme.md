# Text Mode Desktop

<a href="https://www.youtube.com/watch?v=kofkoxGjFWQ">
  <img width="400" alt="Demo on YouTube" src="https://user-images.githubusercontent.com/11535558/146906370-c9705579-1bbb-4e9e-8977-47312f551cc8.gif">
</a>

---

vtm is a text-based desktop environment allowing remote access over tunneling protocols such as SSH. Along with plain-text console I/O, vtm uses binary DirectVT I/O mode to maximize efficiency and minimize cross-platform issues. For remote desktop access between two hosts with vtm and ssh installed on both, all you need is the command: `vtm ssh user@host vtm`.

# Supported Platforms

- Windows
  - Core/Desktop
- Unix
  - Linux
  - Android <sup><sup>Linux kernel</sup></sup>
  - macOS
  - FreeBSD
  - NetBSD
  - OpenBSD
  - [`...`](https://en.wikipedia.org/wiki/POSIX#POSIX-oriented_operating_systems)

[Tested Terminals](https://github.com/directvt/vtm/discussions/72)

# Binary Downloads

![Linux](.resources/status/linux.svg)     [![Intel 64-bit](.resources/status/arch_x86_64.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_linux_x86_64.zip) [![Intel 32-bit](.resources/status/arch_x86.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_linux_x86.zip) [![ARM 64-bit](.resources/status/arch_arm64.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_linux_arm64.zip) [![ARM 32-bit](.resources/status/arch_arm32.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_linux_arm32.zip)  
![Windows](.resources/status/windows.svg) [![Intel 64-bit](.resources/status/arch_x86_64.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_windows_x86_64.zip)  [![Intel 32-bit](.resources/status/arch_x86.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_windows_x86.zip)  [![ARM 64-bit](.resources/status/arch_arm64.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_windows_arm64.zip)  [![ARM 32-bit](.resources/status/arch_arm32.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_windows_arm32.zip)  
![macOS](.resources/status/macos.svg)     [![Universal](.resources/status/arch_any.svg)](https://github.com/directvt/vtm/releases/latest/download/vtm_macos_any.zip)  

# Documentation

- [Architecture and usage](doc/architecture.md)
- [Building from source](doc/build.md)
- [Command-line options](doc/command-line-options.md)
- [User interface](doc/user-interface.md)
- [Settings](doc/settings.md)
- [Desktop Live Panel](doc/panel.md)
- [Built-in applications](doc/apps.md)
- Draft: [VT Input Mode](doc/vt-input-mode.md)
