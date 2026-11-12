/* Drives one line of a GPIO controller through libgpiod 2.x and reads the value back.
 * Usage: gpio_demo [chip device] [line offset]    default: /dev/gpiochip0 line 3 */
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "/dev/gpiochip0";
    unsigned int offset = argc > 2 ? (unsigned int)atoi(argv[2]) : 3;
    int status = 1;

    struct gpiod_chip *chip = gpiod_chip_open(path);
    if (!chip) { perror("gpiod_chip_open"); return 1; }

    /* a request names the lines, their direction and a consumer name that gpioinfo shows */
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *lines = gpiod_line_config_new();
    struct gpiod_request_config *request = gpiod_request_config_new();
    if (!settings || !lines || !request) { perror("allocation"); goto out; }
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_config_add_line_settings(lines, &offset, 1, settings);
    gpiod_request_config_set_consumer(request, "gpio_demo");

    struct gpiod_line_request *held = gpiod_chip_request_lines(chip, request, lines);
    if (!held) { perror("gpiod_chip_request_lines"); goto out; }

    for (int i = 0; i < 4; i++) {
        enum gpiod_line_value want = (i & 1) ? GPIOD_LINE_VALUE_INACTIVE : GPIOD_LINE_VALUE_ACTIVE;
        if (gpiod_line_request_set_value(held, offset, want) < 0) { perror("set_value"); break; }
        printf("line %u set to %d, read back %d\n", offset, (int)want,
               (int)gpiod_line_request_get_value(held, offset));
        status = 0;
    }
    gpiod_line_request_release(held);
out:
    gpiod_request_config_free(request);
    gpiod_line_config_free(lines);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    return status;
}
