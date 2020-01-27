#include "bts_7200_current_sense.h"
#include <stddef.h>

#define STM32_GPIO_STATE_SELECT_OUT_0 GPIO_STATE_LOW
#define STM32_GPIO_STATE_SELECT_OUT_1 GPIO_STATE_HIGH
#define MCP23008_GPIO_STATE_SELECT_OUT_0 MCP23008_GPIO_STATE_LOW
#define MCP23008_GPIO_STATE_SELECT_OUT_1 MCP23008_GPIO_STATE_HIGH

static void prv_measure_current(SoftTimerId timer_id, void *context) {
  Bts7200Storage *storage = context;
  bts_7200_get_measurement(storage, &storage->reading_out_0, &storage->reading_out_1);

  if (storage->callback) {
    storage->callback(storage->reading_out_0, storage->reading_out_1, storage->callback_context);
  }

  soft_timer_start(storage->interval_us, &prv_measure_current, storage, &storage->timer_id);
}

static StatusCode prv_init_common(Bts7200Storage *storage) {
  // initialize the sense pin
  GpioSettings sense_settings = {
    .direction = GPIO_DIR_IN,
    .alt_function = GPIO_ALTFN_ANALOG,
  };
  status_ok_or_return(gpio_init_pin(storage->sense_pin, &sense_settings));

  // initialize the sense pin as ADC
  AdcChannel sense_channel = NUM_ADC_CHANNELS;
  adc_get_channel(*storage->sense_pin, &sense_channel);
  adc_set_channel(sense_channel, true);
  return STATUS_CODE_OK;
}

StatusCode bts_7200_init_stm32(Bts7200Storage *storage, Bts7200Stm32Settings *settings) {
  storage->select_pin_stm32 = settings->select_pin;
  storage->select_pin_mcp23008 = NULL;
  storage->select_pin_type = BTS7200_SELECT_PIN_STM32;
  storage->sense_pin = settings->sense_pin;
  storage->interval_us = settings->interval_us;
  storage->callback = settings->callback;
  storage->callback_context = settings->callback_context;

  // initialize the select pin
  GpioSettings select_settings = {
    .direction = GPIO_DIR_OUT,
    .state = GPIO_STATE_LOW,
    .resistor = GPIO_RES_NONE,
    .alt_function = GPIO_ALTFN_NONE,
  };
  status_ok_or_return(gpio_init_pin(storage->select_pin_stm32, &select_settings));

  return prv_init_common(storage);
}

StatusCode bts_7200_init_mcp23008(Bts7200Storage *storage, Bts7200Mcp23008Settings *settings) {
  storage->select_pin_mcp23008 = settings->select_pin;
  storage->select_pin_stm32 = NULL;
  storage->select_pin_type = BTS7200_SELECT_PIN_MCP23008;
  storage->sense_pin = settings->sense_pin;
  storage->interval_us = settings->interval_us;
  storage->callback = settings->callback;
  storage->callback_context = settings->callback_context;

  // initialize the select pin
  Mcp23008GpioSettings select_settings = {
    .direction = MCP23008_GPIO_DIR_OUT,
    .state = MCP23008_GPIO_STATE_LOW,
  };
  status_ok_or_return(mcp23008_gpio_init_pin(storage->select_pin_mcp23008, &select_settings));

  return prv_init_common(storage);
}

StatusCode bts_7200_get_measurement(Bts7200Storage *storage, uint16_t *meas0, uint16_t *meas1) {
  AdcChannel sense_channel = NUM_ADC_CHANNELS;
  adc_get_channel(*storage->sense_pin, &sense_channel);

  if (storage->select_pin_type == BTS7200_SELECT_PIN_STM32) {
    gpio_set_state(storage->select_pin_stm32, STM32_GPIO_STATE_SELECT_OUT_0);
  } else {
    mcp23008_gpio_set_state(storage->select_pin_mcp23008, MCP23008_GPIO_STATE_SELECT_OUT_0);
  }
  adc_read_raw(sense_channel, meas0);

  if (storage->select_pin_type == BTS7200_SELECT_PIN_STM32) {
    gpio_set_state(storage->select_pin_stm32, STM32_GPIO_STATE_SELECT_OUT_1);
  } else {
    mcp23008_gpio_set_state(storage->select_pin_mcp23008, MCP23008_GPIO_STATE_SELECT_OUT_1);
  }
  adc_read_raw(sense_channel, meas1);

  return STATUS_CODE_OK;
}

StatusCode bts_7200_start(Bts7200Storage *storage) {
  // |prv_measure_current| will set up a soft timer to call itself repeatedly
  // by calling this now there's no period with invalid measurements
  prv_measure_current(SOFT_TIMER_INVALID_TIMER, storage);
  return STATUS_CODE_OK;
}

bool bts_7200_stop(Bts7200Storage *storage) {
  return soft_timer_cancel(storage->timer_id);
}
