/* gpio.h is the library for control of the GPIO in the ac-cloudifier project.
 * Thanks to pigpio, multiple instances of this library can be used at the same.
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#ifndef _DESKTOP_BUILD_
#include <pigpiod_if2.h>
#endif

#define _GPIO_H_LED_RED         22
#define _GPIO_H_LED_BLUE        23
#define _GPIO_H_LED_IR          18

#define _GPIO_H_LED_RED_ON      PI_LOW
#define _GPIO_H_LED_BLUE_ON     PI_LOW
#define _GPIO_H_LED_IR_ON       PI_HIGH

#define _GPIO_H_LED_RED_OFF     PI_HIGH
#define _GPIO_H_LED_BLUE_OFF    PI_HIGH
#define _GPIO_H_LED_IR_OFF      PI_LOW

enum StatusLed {stat_off, stat_red, stat_blue, stat_purple};
enum InfraLed {ir_off, ir_on};

struct GPIO {
    int _fd;
    enum StatusLed _statusled;
    enum InfraLed _infraled;
};

/* Initializes the GPIO object. Must be called before any other GPIO functions.
 * A `struct GPIO` pointer must be passed to @param gpio. This structure is then
 * initialized, and the hardware is also initialized.
 * @returns 0 on success or a negative on failure.
 */
int GPIO_initialize(struct GPIO *gpio);

/* Finalizes the GPIO object. Must be called before exiting the program.
 * @returns 0 on success or a negative on failure.
 */
int GPIO_finalize(struct GPIO *gpio);

/* Sets the product's status LED. See the `StatusLed` enum for color options.
 * The function takes a `struct GPIO` @param gpio to hold the object's state. 
 * The struct must be already initialized. The @param color is used to specify 
 * the color. @returns 0 on success or a pigpiod_if2 error code on failure.
 */
int GPIO_set_StatusLED(struct GPIO *gpio, enum StatusLed color);

/* Gets the product's status LED. See the `StatusLed` enum for color options.
 * The function takes a `struct GPIO` on @param gpio and this is used to return
 * the status to the `statusled` member. The struct must be already initialized.
 * @returns 0 on success or a pigpiod_if2 error code on failure.
 */
int GPIO_get_StatusLED(struct GPIO *gpio);

/* Sets the product's infrared LED. See the `InfraLed` enum for color options.
 * The function takes a `struct GPIO` @param gpio to hold the object's state. 
 * The struct must be already initialized. The @param state is used to specify 
 * the color. @returns 0 on success or a pigpiod_if2 error code on failure.
 */
int GPIO_set_InfraLED(struct GPIO *gpio, enum InfraLed state);

/* Gets the product's infrared LED. See the `InfraLed` enum for options.
 * The function takes a `struct GPIO` on @param gpio and this is used to return
 * the status to the `infraled` member. The struct must be already initialized.
 * @returns 0 on success or a pigpiod_if2 error code on failure.
 */
int GPIO_get_InfraLED(struct GPIO *gpio);

#endif
