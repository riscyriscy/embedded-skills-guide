#ifndef SENSOR_VERSION_H
#define SENSOR_VERSION_H

/**
 * @file version.h
 * @brief The version stamp of the build.
 */

/**
 * @brief Gives the version of the firmware.
 * @return The output of `git describe --tags --always --dirty` taken at build time, for example
 *         "v1.2.0-3-gabc1234". The string is static and must not be freed.
 */
const char *sensor_version(void);

#endif
