#include "app.h"

#include "display.h"
#include "status_led.h"
#include "ultrasonic.h"

#define APP_ECHO_TIMEOUT_US    30000U
#define APP_DISTANCE_ERROR_CM  999U
#define APP_LOOP_DELAY_MS      33U

uint8_t app_init(TIM_HandleTypeDef *echo_tim, uint32_t echo_channel)
{
    ultrasonic_init(echo_tim, echo_channel);

    if (!display_init()) {
        return 0U;
    }

    display_show_boot();
    return 1U;
}

void app_step(void)
{
    uint32_t distance_cm = ultrasonic_read_distance_cm(APP_ECHO_TIMEOUT_US, APP_DISTANCE_ERROR_CM);

    status_led_set_for_distance(distance_cm);
    display_show_distance_cm(distance_cm);

    HAL_Delay(APP_LOOP_DELAY_MS);
}
