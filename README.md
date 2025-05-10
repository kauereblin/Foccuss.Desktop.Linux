# Foccuss - Application Blocker for Linux

Foccuss is a productivity tool that helps you focus by blocking distracting applications during scheduled times.

## Features

- Block distracting applications during scheduled times
- Schedule blocking for specific days of the week
- Automatic background service that runs even when the main app is closed
- Simple and intuitive user interface

## Building from Source

### Prerequisites

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential cmake qt6-base-dev libsqlite3-dev
```

On Fedora:
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel sqlite-devel
```

On Arch Linux:
```bash
sudo pacman -S base-devel cmake qt6-base sqlite
```

### Build Instructions

1. Clone the repository:
```bash
git clone https://github.com/yourusername/foccuss.git
cd foccuss
```

2. Create a build directory:
```bash
mkdir build
cd build
```

3. Run CMake and build:
```bash
cmake ..
make
```

4. Install (optional):
```bash
sudo make install
```

## Usage

1. Launch Foccuss from your application menu or by running `Foccuss` in terminal
2. Add applications to the blocked list
3. Configure blocking schedules in the Settings tab
4. Enable the service to start blocking applications

## Service Configuration

Foccuss uses a systemd user service to monitor and block applications. You can manage the service manually:

Enable the service:
```bash
systemctl --user enable foccuss.service
```

Start the service:
```bash
systemctl --user start foccuss.service
```

Check service status:
```bash
systemctl --user status foccuss.service
```

## Troubleshooting

If applications are not being blocked:
1. Make sure the service is running
2. Check that the current time falls within the scheduled blocking period
3. Look at the logs in `~/.local/share/Foccuss/foccuss_service.log`

## License

This software is released under the MIT License.
