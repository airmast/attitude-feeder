[![Travis CI Build Status](https://travis-ci.org/airmast/attitude-feeder.svg?branch=master)](https://travis-ci.org/airmast/attitude-feeder)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/b10a9rd1vj0n5fk6?svg=true)](https://ci.appveyor.com/project/mkrutyakov-ugcs/attitude-feeder/)

Attitude Feeder
===============

MAVLink-compatible tool which listens for AHRS data on a serial interface and transmits it to the Camera Adapter via HTTP.

Requirements
------------

* MAVLink-based AHRS device (like [Pixhawk](https://pixhawk.org/modules/pixhawk), for example)
* Qt5 and Qt Creator 4.3+
* OS: **Linux** (tested on Ubuntu 16.04, 14.04) or **Windows** (tested on Windows 10)

Building
--------

Open `attitude-feeder.qbs` in Qt Creator and click to **Build Project** button.

Usage
-----

```shell
attfeeder [options] device
```

Arguments:

* `device`

    MAVLink serial device (e.g. **/dev/ttyACM0** for Linux or **COM10** in case of Windows).

Options:

* `-h`, `--help`

    Displays this help.

* `-v`, `--version`

    Displays version information.

License
-------

Licensed under the [3-Clause BSD License](./LICENSE).
