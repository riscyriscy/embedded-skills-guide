# sensorfw

Firmware for the sensor board. The library in `lib/` keeps the last samples of a sensor in a ring buffer and
computes their mean. The application in `app/` runs on a Cortex-M4 and prints its version.

* [Architecture](architecture.md): the parts and how they depend on each other.
* [Building](building.md): from a fresh clone to a tested image.
* [Releasing](releasing.md): how a tag becomes a signed release.
* [Decisions](decisions/index.md): why the code is the way it is.
* [API reference](api/index.html): generated from the comments in the headers.
