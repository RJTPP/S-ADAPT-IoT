#include "encoder_input.h"

#include "main.h"

#if defined(ENCODER_PRESS_GPIO_Port) && defined(ENCODER_PRESS_Pin)
#define ENCODER_SW_GPIO_Port ENCODER_PRESS_GPIO_Port
#define ENCODER_SW_Pin ENCODER_PRESS_Pin
#elif defined(SW_GPIO_Port) && defined(SW_Pin)
#define ENCODER_SW_GPIO_Port SW_GPIO_Port
#define ENCODER_SW_Pin SW_Pin
#else
#error "Encoder switch pin macros are not defined in main.h"
#endif

#define ENCODER_SW_SAMPLE_MS      10U
#define ENCODER_SW_DEBOUNCE_TICKS 3U
#define ENCODER_EDGE_GUARD_MS     2U
#define ENCODER_EVENT_QUEUE_SIZE  8U

typedef struct
{
    uint8_t stable_level;
    uint8_t candidate_level;
    uint8_t candidate_ticks;
} sw_state_t;

static sw_state_t s_sw_state;
static uint32_t s_last_sw_sample_ms = 0U;
static uint32_t s_last_clk_edge_ms = 0U;

static encoder_event_t s_event_queue[ENCODER_EVENT_QUEUE_SIZE];
static uint8_t s_queue_head = 0U;
static uint8_t s_queue_tail = 0U;
static uint8_t s_queue_count = 0U;

static GPIO_PinState read_pin(GPIO_TypeDef *port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port, pin);
}

static uint8_t read_pin_level(GPIO_TypeDef *port, uint16_t pin)
{
    return (read_pin(port, pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static uint32_t critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void critical_exit(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

static void queue_push(encoder_event_type_t type, uint32_t now_ms, uint8_t sw_level)
{
    uint32_t primask = critical_enter();

    if (s_queue_count < ENCODER_EVENT_QUEUE_SIZE) {
        encoder_event_t event;
        event.type = type;
        event.timestamp_ms = now_ms;
        event.sw_level = sw_level;

        s_event_queue[s_queue_tail] = event;
        s_queue_tail = (uint8_t)((s_queue_tail + 1U) % ENCODER_EVENT_QUEUE_SIZE);
        s_queue_count++;
    }

    critical_exit(primask);
}

void encoder_input_init(void)
{
    uint8_t sw_level = read_pin_level(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin);

    s_sw_state.stable_level = sw_level;
    s_sw_state.candidate_level = sw_level;
    s_sw_state.candidate_ticks = 0U;

    s_last_sw_sample_ms = HAL_GetTick();
    s_last_clk_edge_ms = 0U;

    s_queue_head = 0U;
    s_queue_tail = 0U;
    s_queue_count = 0U;
}

void encoder_input_tick(uint32_t now_ms)
{
    uint8_t sw_level;

    if ((uint32_t)(now_ms - s_last_sw_sample_ms) < ENCODER_SW_SAMPLE_MS) {
        return;
    }
    s_last_sw_sample_ms = now_ms;

    sw_level = read_pin_level(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin);
    if (sw_level != s_sw_state.candidate_level) {
        s_sw_state.candidate_level = sw_level;
        s_sw_state.candidate_ticks = 1U;
        return;
    }

    if (s_sw_state.candidate_ticks < ENCODER_SW_DEBOUNCE_TICKS) {
        s_sw_state.candidate_ticks++;
    }

    if ((s_sw_state.candidate_ticks >= ENCODER_SW_DEBOUNCE_TICKS) &&
        (s_sw_state.stable_level != s_sw_state.candidate_level)) {
        s_sw_state.stable_level = s_sw_state.candidate_level;
        queue_push((s_sw_state.stable_level == 0U) ? ENCODER_EVENT_SW_PRESSED : ENCODER_EVENT_SW_RELEASED,
                   now_ms,
                   s_sw_state.stable_level);
    }
}

void encoder_input_on_clk_edge_isr(void)
{
    uint8_t dt_level;
    uint8_t sw_level;
    uint32_t now_ms = HAL_GetTick();

    if ((uint32_t)(now_ms - s_last_clk_edge_ms) < ENCODER_EDGE_GUARD_MS) {
        return;
    }
    s_last_clk_edge_ms = now_ms;

    dt_level = read_pin_level(ENCODER_DT_EXTI10_GPIO_Port, ENCODER_DT_EXTI10_Pin);
    sw_level = read_pin_level(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin);

    queue_push((dt_level == 0U) ? ENCODER_EVENT_CW : ENCODER_EVENT_CCW, now_ms, sw_level);
}

uint8_t encoder_input_pop_event(encoder_event_t *out_event)
{
    uint32_t primask;

    if (out_event == NULL) {
        return 0U;
    }

    primask = critical_enter();
    if (s_queue_count == 0U) {
        critical_exit(primask);
        return 0U;
    }

    *out_event = s_event_queue[s_queue_head];
    s_queue_head = (uint8_t)((s_queue_head + 1U) % ENCODER_EVENT_QUEUE_SIZE);
    s_queue_count--;
    critical_exit(primask);
    return 1U;
}
