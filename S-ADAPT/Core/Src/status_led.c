#include "status_led.h"

#include "main.h"

#define DISTANCE_THRESHOLD_NEAR_CM 10U
#define DISTANCE_THRESHOLD_MID_CM  20U

void status_led_set_for_distance(uint32_t distance_cm)
{
    if (distance_cm < DISTANCE_THRESHOLD_NEAR_CM)
    {
        HAL_GPIO_WritePin(LED_Status_R_GPIO_Port, LED_Status_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_Status_G_GPIO_Port, LED_Status_G_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_Status_B_GPIO_Port, LED_Status_B_Pin, GPIO_PIN_RESET);
    }
    else if (distance_cm < DISTANCE_THRESHOLD_MID_CM)
    {
        HAL_GPIO_WritePin(LED_Status_R_GPIO_Port, LED_Status_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_Status_G_GPIO_Port, LED_Status_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_Status_B_GPIO_Port, LED_Status_B_Pin, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(LED_Status_R_GPIO_Port, LED_Status_R_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_Status_G_GPIO_Port, LED_Status_G_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_Status_B_GPIO_Port, LED_Status_B_Pin, GPIO_PIN_SET);
    }
}

void status_led_blink_error(uint32_t period_ms)
{
    HAL_GPIO_WritePin(LED_Status_G_GPIO_Port, LED_Status_G_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_Status_B_GPIO_Port, LED_Status_B_Pin, GPIO_PIN_RESET);
    HAL_GPIO_TogglePin(LED_Status_R_GPIO_Port, LED_Status_R_Pin);
    HAL_Delay(period_ms);
}
