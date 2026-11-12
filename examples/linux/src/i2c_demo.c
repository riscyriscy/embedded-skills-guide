/* Reads register 0 of an I2C device through /dev/i2c-N with the SMBus ioctl, as a temperature in half degrees.
 * Usage: i2c_demo [bus device] [7-bit address]    default: /dev/i2c-0 0x48 */
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int smbus_read_byte(int fd, unsigned char reg)
{
    union i2c_smbus_data data;
    struct i2c_smbus_ioctl_data args = {
        .read_write = I2C_SMBUS_READ, .command = reg, .size = I2C_SMBUS_BYTE_DATA, .data = &data,
    };
    if (ioctl(fd, I2C_SMBUS, &args) < 0)
        return -1;
    return data.byte;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/dev/i2c-0";
    long address = argc > 2 ? strtol(argv[2], NULL, 0) : 0x48;
    int fd = open(path, O_RDWR);
    if (fd < 0) { perror(path); return 1; }
    if (ioctl(fd, I2C_SLAVE, address) < 0) { perror("I2C_SLAVE"); return 1; }
    int raw = smbus_read_byte(fd, 0x00);
    if (raw < 0) { perror("read register 0"); return 1; }
    printf("device 0x%02lx register 0: %d, which is %.1f degrees\n", address, raw, raw / 2.0);
    close(fd);
    return 0;
}
