#include "input/encoder_input.h"

#include "input/input_utils.h"
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
#define ENCODER_STEPS_PER_DETENT  4U
#define ENCODER_EVENT_QUEUE_SIZE  8U

typedef struct
{
    uint8_t stable_level;
    uint8_t candidate_level;
    uint8_t candidate_ticks;
} sw_state_t;

static sw_state_t s_sw_state;
static uint32_t s_last_sw_sample_ms = 0U;
static uint8_t s_last_ab_state = 0U;
static int8_t s_step_accum = 0;

static encoder_event_t s_event_queue[ENCODER_EVENT_QUEUE_SIZE];
static uint8_t s_queue_head = 0U;
static uint8_t s_queue_tail = 0U;
static uint8_t s_queue_count = 0U;

/* Reject invalid quadrature transitions to suppress bounce-induced reversals. */
static const int8_t s_quadrature_lut[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

static uint8_t encoder_read_ab_state(void)
{
    uint8_t clk_level = input_gpio_level(ENCODER_CLK_EXTI1_GPIO_Port, ENCODER_CLK_EXTI1_Pin);
    uint8_t dt_level = input_gpio_level(ENCODER_DT_EXTI10_GPIO_Port, ENCODER_DT_EXTI10_Pin);

    return (uint8_t)((clk_level << 1) | dt_level);
}

static void queue_push(encoder_event_type_t type, uint32_t now_ms, uint8_t sw_level)
{
    uint32_t primask = input_irq_lock();

    if (s_queue_count < ENCODER_EVENT_QUEUE_SIZE) {
        encoder_event_t event;
        event.type = type;
        event.timestamp_ms = now_ms;
        event.sw_level = sw_level;

        s_event_queue[s_queue_tail] = event;
        s_queue_tail = (uint8_t)((s_queue_tail + 1U) % ENCODER_EVENT_QUEUE_SIZE);
        s_queue_count++;
    }

    input_irq_unlock(primask);
}

void encoder_input_init(void)
{
    uint32_t primask = input_irq_lock();
    uint8_t sw_level = input_gpio_level(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin);

    s_sw_state.stable_level = sw_level;
    s_sw_state.candidate_level = sw_level;
    s_sw_state.candidate_ticks = 0U;

    s_last_sw_sample_ms = HAL_GetTick();
    s_last_ab_state = encoder_read_ab_state();
    s_step_accum = 0;

    s_queue_head = 0U;
    s_queue_tail = 0U;
    s_queue_count = 0U;

    input_irq_unlock(primask);
}

void encoder_input_tick(uint32_t now_ms)
{
    uint8_t sw_level;

    if (input_has_elapsed_ms(now_ms, s_last_sw_sample_ms, ENCODER_SW_SAMPLE_MS) == 0U) {
        return;
    }
    s_last_sw_sample_ms = now_ms;

    sw_level = input_gpio_level(ENCODER_SW_GPIO_Port, ENCODER_SW_Pin);
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
    uint8_t ab_state;
    uint8_t lut_index;
    int8_t step_delta;
    uint8_t sw_level;
    uint32_t now_ms = HAL_GetTick();

    ab_state = encoder_read_ab_state();
    lut_index = (uint8_t)((s_last_ab_state << 2) | ab_state);
    step_delta = s_quadrature_lut[lut_index];
    s_last_ab_state = ab_state;

    if (step_delta == 0) {
        return;
    }

    s_step_accum = (int8_t)(s_step_accum + step_delta);
    sw_level = s_sw_state.stable_level;

    if (s_step_accum >= (int8_t)ENCODER_STEPS_PER_DETENT) {
        s_step_accum = 0;
        queue_push(ENCODER_EVENT_CW, now_ms, sw_level);
    } else if (s_step_accum <= -(int8_t)ENCODER_STEPS_PER_DETENT) {
        s_step_accum = 0;
        queue_push(ENCODER_EVENT_CCW, now_ms, sw_level);
    }
}

uint8_t encoder_input_pop_event(encoder_event_t *out_event)
{
    uint32_t primask;

    if (out_event == NULL) {
        return 0U;
    }

    primask = input_irq_lock();
    if (s_queue_count == 0U) {
        input_irq_unlock(primask);
        return 0U;
    }

    *out_event = s_event_queue[s_queue_head];
    s_queue_head = (uint8_t)((s_queue_head + 1U) % ENCODER_EVENT_QUEUE_SIZE);
    s_queue_count--;
    input_irq_unlock(primask);
    return 1U;
}
