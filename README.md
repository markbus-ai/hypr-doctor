# Hyprland Doctor 🩺

**Hyprland Doctor** is a lightweight, C-based diagnostic tool designed to help [Hyprland](https://github.com/hyprwm/Hyprland) users troubleshoot their setup. It scans your system for common configuration errors, missing dependencies, and environment variable issues, ensuring a stable Wayland experience.

> **Note:** The CLI output is currently localized in **Spanish**.

## 🚀 Features

### 1. GPU Diagnostics
- **NVIDIA Support**: Checks for `nvidia`, `nvidia_drm`, and `nvidia_modeset` modules.
- **Wayland Readiness**: Verifies if `nvidia_drm.modeset=1` is active (essential for Hyprland on NVIDIA).
- **Conflict Detection**: Warns if `nouveau` is loaded alongside proprietary drivers.
- **Multi-GPU**: Detects hybrid setups (Intel/AMD + NVIDIA).

### 2. Environment Verification (`ENV`)
- **Session Variables**: Ensures `XDG_SESSION_TYPE` and `XDG_CURRENT_DESKTOP` are correctly set.
- **XDG Portals**: Checks for the presence of `xdg-desktop-portal-hyprland` and warns about conflicting portals (GNOME, KDE, WLR).
- **App Compatibility**: Verifies hints for Electron, Qt, and SDL applications to run natively on Wayland.

### 3. Configuration Check
- Parses `~/.config/hypr/hyprland.conf`.
- **Deprecation Scanner**: Identifies old syntax (e.g., `windowrulev2` vs `windowrule`, boolean syntax).
- **Auto-Fix**: Generates a corrected version of your config at `hyprland.conf.tmp` with updated syntax.

### 4. Process & Service Monitor
- Checks if critical services are running:
  - `Pipewire` & `Wireplumber` (Audio/Screen sharing)
  - `DBus`
  - `XDG Desktop Portals`
- Warns about missing services that could break screen sharing or file pickers.

### 5. Log Analysis
- Automatically finds the latest Hyprland log in `/tmp/hypr/`.
- Tails the last 50 lines.
- Highlights **Errors**, **Warnings**, **Crashes** (Segfaults), and permission issues in real-time.

## 📦 Installation

### Prerequisites
- GCC (GNU Compiler Collection)
- Make (optional)

### Build from Source

```bash
git clone https://github.com/markbus-ai/hypr-doctor
cd hypr-doctor
make
```

Or manually without Make:
```bash
gcc -o hypr-doctor main.c src/*.c -Iinclude
```

## 🛠 Usage

Simply run the generated binary:

```bash
./hypr-doctor
```

### Understanding the Output
- 🟢 **[OK]**: The check passed successfully.
- 🟡 **[WARNING]**: Something is not ideal (e.g., a deprecated config option) but won't break the session.
- 🔴 **[ERROR]**: A critical issue that needs attention (e.g., missing drivers, conflicting portals).

## 🤝 Contributing

Contributions are welcome! Whether it's translating the tool to English/other languages, adding new checks, or fixing bugs.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/NewFeature`)
3. Commit your Changes (`git commit -m 'Add some NewFeature'`)
4. Push to the Branch (`git push origin feature/NewFeature`)
5. Open a Pull Request

## 📄 License

[MIT](LICENSE) (Assuming standard open source license, please verify)
