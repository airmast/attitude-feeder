Attitude Feeder
===============

MAVLink-compatible tool which listens for AHRS data on a serial or network interface and transmits it to the Camera Adapter via HTTP.

Requirements
------------

* MAVLink-based AHRS device (like [Pixhawk](https://pixhawk.org/modules/pixhawk), for example)
* OS: **Linux** (tested on Ubuntu 16.04, 14.04) or **Windows** (tested on Windows 10)

Installation
------------

### Desktop and Embedded Linux ###

Using snap (**i386**, **amd64**, **armhf** and **arm64** platforms are supported):

    sudo snap install attfeeder

### Windows, Desktop Linux ###

Download the package from [releases section on GitHub](https://github.com/airmast/attitude-feeder/releases), and unpack it.

Building
--------

Prerequisites:
* Qt5
* Qt Creator 4.3+ with QBS 1.7+

Open `attitude-feeder.qbs` in Qt Creator and click to **Build Project** button.

Usage
-----

```shell
attfeeder [options]
```

Options:

* `-h`, `--help`

    Displays this help.

* `-v`, `--version`

    Displays version information.

* `-s`, `--serial` `<serial>`

    MAVLink serial device (e.g. '/dev/ttyACM0' or 'COM10' or regex like '/dev/ttyUSB\\\\d')

* `-n`, `--network` `<network>`

    MAVLink network device address (e.g. localhost or 127.0.0.1).

* `-p`, `--port` `<port>`

    MAVLink network device port.

License
-------

Licensed under the [3-Clause BSD License](./LICENSE).
