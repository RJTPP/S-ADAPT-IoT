#include "switch_input.h"

#include "main.h"

#define SWITCH_SAMPLE_PERIOD_MS 10U
#define SWITCH_DEBOUNCE_TICKS   3U
#define SWITCH_EVENT_QUEUE_SIZE 8U

typedef struct
{
    uint8_t stable_level;
    uint8_t candidate_level;
    uint8_t candidate_ticks;
} switch_state_t;

static switch_state_t s_button_state;
static switch_state_t s_sw2_state;
static uint32_t s_last_sample_ms = 0U;

static switch_input_event_t s_event_queue[SWITCH_EVENT_QUEUE_SIZE];
static uint8_t s_queue_head = 0U;
static uint8_t s_queue_tail = 0U;
static uint8_t s_queue_count = 0U;

static uint8_t gpio_level(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(port, pin);
    return (state == GPIO_PIN_SET) ? 1U : 0U;
}

static void init_switch_state(switch_state_t *state, GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t level = gpio_level(port, pin);
    state->stable_level = level;
    state->candidate_level = level;
    state->candidate_ticks = 0U;
}

static void queue_push(switch_input_id_t input, uint8_t level)
{
    switch_input_event_t event;

    if (s_queue_count >= SWITCH_EVENT_QUEUE_SIZE) {
        return;
    }

    event.input = input;
    event.level = level;
    event.pressed = (level == 0U) ? 1U : 0U;

    s_event_queue[s_queue_tail] = event;
    s_queue_tail = (uint8_t)((s_queue_tail + 1U) % SWITCH_EVENT_QUEUE_SIZE);
    s_queue_count++;
}

static void process_switch(switch_state_t *state, GPIO_TypeDef *port, uint16_t pin, switch_input_id_t input)
{
    uint8_t raw_level = gpio_level(port, pin);

    if (raw_level != state->candidate_level) {
        state->candidate_level = raw_level;
        state->candidate_ticks = 1U;
        return;
    }

    if (state->candidate_ticks < SWITCH_DEBOUNCE_TICKS) {
        state->candidate_ticks++;
    }

    if ((state->candidate_ticks >= SWITCH_DEBOUNCE_TICKS) &&
        (state->stable_level != state->candidate_level)) {
        state->stable_level = state->candidate_level;
        queue_push(input, state->stable_level);
    }
}

void switch_input_init(void)
{
    init_switch_state(&s_button_state, BUTTON_GPIO_Port, BUTTON_Pin);
    init_switch_state(&s_sw2_state, SW_GPIO_Port, SW_Pin);
    s_last_sample_ms = HAL_GetTick();

    s_queue_head = 0U;
    s_queue_tail = 0U;
    s_queue_count = 0U;
}

void switch_input_tick(uint32_t now_ms)
{
    if ((uint32_t)(now_ms - s_last_sample_ms) < SWITCH_SAMPLE_PERIOD_MS) {
        return;
    }

    s_last_sample_ms = now_ms;
    process_switch(&s_button_state, BUTTON_GPIO_Port, BUTTON_Pin, SWITCH_INPUT_BUTTON);
    process_switch(&s_sw2_state, SW_GPIO_Port, SW_Pin, SWITCH_INPUT_SW2);
}

uint8_t switch_input_pop_event(switch_input_event_t *out_event)
{
    if ((out_event == NULL) || (s_queue_count == 0U)) {
        return 0U;
    }

    *out_event = s_event_queue[s_queue_head];
    s_queue_head = (uint8_t)((s_queue_head + 1U) % SWITCH_EVENT_QUEUE_SIZE);
    s_queue_count--;
    return 1U;
}
