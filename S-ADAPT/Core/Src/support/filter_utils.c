#include "support/filter_utils.h"

static uint32_t median3_u32(uint32_t a, uint32_t b, uint32_t c)
{
    if (a > b) {
        uint32_t t = a;
        a = b;
        b = t;
    }
    if (b > c) {
        uint32_t t = b;
        b = c;
        c = t;
    }
    if (a > b) {
        uint32_t t = a;
        a = b;
        b = t;
    }
    return b;
}

void filter_moving_average_u16_init(filter_moving_average_u16_t *f, uint8_t window_size)
{
    uint8_t i;

    if (f == NULL) {
        return;
    }

    if ((window_size == 0U) || (window_size > FILTER_MA_U16_MAX_WINDOW)) {
        window_size = 1U;
    }

    f->sum = 0U;
    f->window_size = window_size;
    f->count = 0U;
    f->index = 0U;

    for (i = 0U; i < FILTER_MA_U16_MAX_WINDOW; i++) {
        f->buffer[i] = 0U;
    }
}

uint16_t filter_moving_average_u16_push(filter_moving_average_u16_t *f, uint16_t sample)
{
    if (f == NULL) {
        return sample;
    }

    if (f->count < f->window_size) {
        f->buffer[f->index] = sample;
        f->sum += sample;
        f->count++;
        f->index = (uint8_t)((f->index + 1U) % f->window_size);
    } else {
        f->sum -= f->buffer[f->index];
        f->buffer[f->index] = sample;
        f->sum += sample;
        f->index = (uint8_t)((f->index + 1U) % f->window_size);
    }

    return (uint16_t)(f->sum / f->count);
}

uint16_t filter_moving_average_u16_get(const filter_moving_average_u16_t *f)
{
    if ((f == NULL) || (f->count == 0U)) {
        return 0U;
    }

    return (uint16_t)(f->sum / f->count);
}

uint8_t filter_moving_average_u16_is_ready(const filter_moving_average_u16_t *f)
{
    if (f == NULL) {
        return 0U;
    }

    return (f->count >= f->window_size) ? 1U : 0U;
}

void filter_median3_u32_init(filter_median3_u32_t *f)
{
    if (f == NULL) {
        return;
    }

    f->samples[0] = 0U;
    f->samples[1] = 0U;
    f->samples[2] = 0U;
    f->count = 0U;
    f->index = 0U;
}

void filter_median3_u32_push(filter_median3_u32_t *f, uint32_t sample)
{
    if (f == NULL) {
        return;
    }

    f->samples[f->index] = sample;
    f->index = (uint8_t)((f->index + 1U) % 3U);
    if (f->count < 3U) {
        f->count++;
    }
}

uint32_t filter_median3_u32_get(const filter_median3_u32_t *f)
{
    if (f == NULL || f->count == 0U) {
        return 0U;
    }

    if (f->count == 1U) {
        return f->samples[0];
    }

    if (f->count == 2U) {
        return (f->samples[0] + f->samples[1]) / 2U;
    }

    return median3_u32(f->samples[0], f->samples[1], f->samples[2]);
}

uint8_t filter_median3_u32_is_ready(const filter_median3_u32_t *f)
{
    if (f == NULL) {
        return 0U;
    }

    return (f->count >= 3U) ? 1U : 0U;
}
