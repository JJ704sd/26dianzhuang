#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "scope_metrics.h"

static void test_inverting_front_end_duty(void)
{
    const uint16_t samples[8] = {
        1800U, 1800U, 1800U, 1800U, 3000U, 3000U, 3000U, 3000U
    };
    scope_metrics_t metrics =
        ScopeMetrics_Analyze(samples, 8U, 1800U, 3000U, 16U);

    assert(metrics.valid == 1U);
    assert(metrics.input_duty_percent == 50U);
}

static void test_asymmetric_pulse_duty(void)
{
    const uint16_t samples[8] = {
        1800U, 1800U, 3000U, 3000U, 3000U, 3000U, 3000U, 3000U
    };
    scope_metrics_t metrics =
        ScopeMetrics_Analyze(samples, 8U, 1800U, 3000U, 16U);

    assert(metrics.valid == 1U);
    assert(metrics.input_duty_percent == 25U);
}

static void test_flat_trace_is_invalid(void)
{
    const uint16_t samples[4] = {2048U, 2049U, 2048U, 2049U};
    scope_metrics_t metrics =
        ScopeMetrics_Analyze(samples, 4U, 2048U, 2049U, 16U);

    assert(metrics.valid == 0U);
    assert(metrics.input_duty_percent == 0U);
}

static void test_invalid_arguments(void)
{
    const uint16_t sample = 1000U;

    assert(ScopeMetrics_Analyze(0, 1U, 0U, 100U, 16U).valid == 0U);
    assert(ScopeMetrics_Analyze(&sample, 0U, 0U, 100U, 16U).valid == 0U);
    assert(ScopeMetrics_Analyze(&sample, 1U, 100U, 0U, 16U).valid == 0U);
}

int main(void)
{
    test_inverting_front_end_duty();
    test_asymmetric_pulse_duty();
    test_flat_trace_is_invalid();
    test_invalid_arguments();
    puts("scope metrics tests passed");
    return 0;
}
