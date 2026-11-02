# Architecture

![Dependencies between the parts](img/architecture.svg)

The application depends on the sensor library and on the platform layer. The platform layer has two
implementations, one for the host and one for the Cortex-M4. The library never calls the platform, so
the unit tests run on the host without a board.

The diagram is generated from `architecture.dot` by `tools/docs.sh`. Change the file, not the picture.
