#include "can.h"
#include "event_queue.h"
#include "gpio_it.h"
#include "interrupt.h"
#include "log.h"
#include "ms_test_helpers.h"
#include "pedal_can.h"
#include "pedal_events.h"
#include "status.h"
#include "test_helpers.h"
#include "unity.h"

#define CAN_DEVICE_ID 0x1

static CanStorage can_storage;

void setup_test(void) {
  const CanSettings can_settings = {
    .device_id = CAN_DEVICE_ID,
    .bitrate = CAN_HW_BITRATE_500KBPS,
    .rx_event = PEDAL_CAN_RX,
    .tx_event = PEDAL_CAN_TX,
    .fault_event = PEDAL_CAN_FAULT,
    .loopback = true,
    .tx = { GPIO_PORT_A, 12 },  // CHANGE
    .rx = { GPIO_PORT_A, 11 },  // CHANGE
  };
  gpio_init();
  interrupt_init();
  gpio_it_init();
  soft_timer_init();
  event_queue_init();
  pedal_can_init(&can_storage, &can_settings);
}

void teardown_test(void) {}

void test_assert_trivial(void) {
  TEST_ASSERT_TRUE(true);
}

// Transmit a pedal can state can message, and expect the correct event to be raised.
void test_pedal_can_rx_handler_brake_pressed(void) {
  // Transmit a pedal pressed state message.
  CanMessage msg = {
    .source_id = 0x1,
    .type = CAN_MSG_TYPE_DATA,
    .msg_id = 0,
    .data = 0,
    .dlc = 0,
  };
  TEST_ASSERT_OK(can_transmit(&msg, NULL));
  Event e = { 0 };

  MS_TEST_HELPER_CAN_TX_RX(PEDAL_CAN_TX, PEDAL_CAN_RX);
  TEST_ASSERT_OK(event_process(&e));
  TEST_ASSERT_EQUAL(PEDAL_CAN_EVENT_BRAKE_PRESSED, e.id);
}

void test_pedal_can_rx_handler_brake_released(void) {
  // Transmit a pedal released state message.
  CanMessage msg = {
    .source_id = 0x1,
    .type = CAN_MSG_TYPE_DATA,
    .msg_id = 1,
    .data = 0,
    .dlc = 0,
  };
  TEST_ASSERT_OK(can_transmit(&msg, NULL));
  Event e = { 0 };

  MS_TEST_HELPER_CAN_TX_RX(PEDAL_CAN_TX, PEDAL_CAN_RX);
  TEST_ASSERT_OK(event_process(&e));
  TEST_ASSERT_EQUAL(PEDAL_CAN_EVENT_BRAKE_RELEASED, e.id);
}

//
void test_pedal_can_brake_pressed_can(void) {
  Event e = {
    .id = PEDAL_CAN_EVENT_BRAKE_PRESSED,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}

void test_pedal_can_brake_released_can(void) {
  Event e = {
    .id = PEDAL_CAN_EVENT_BRAKE_RELEASED,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}

void test_pedal_can_rx_can(void) {
  Event e = {
    .id = PEDAL_CAN_RX,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}

void test_pedal_can_tx_can(void) {
  Event e = {
    .id = PEDAL_CAN_TX,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}

void test_pedal_can_fault_can(void) {
  Event e = {
    .id = PEDAL_CAN_FAULT,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}

void test_pedal_can_car_input_fault(void) {
  Event e = {
    .id = PEDAL_DRIVE_INPUT_EVENT_FAULT,
  };
  TEST_ASSERT_FALSE(pedal_can_process_event(&e));
}

void test_pedal_can_car_input_neutral(void) {
  Event e = {
    .id = PEDAL_DRIVE_INPUT_EVENT_NEUTRAL,
  };
  TEST_ASSERT_FALSE(pedal_can_process_event(&e));
}

void test_pedal_can_car_input_drive(void) {
  Event e = {
    .id = PEDAL_DRIVE_INPUT_EVENT_DRIVE,
  };
  TEST_ASSERT_FALSE(pedal_can_process_event(&e));
}

void test_pedal_can_brake_pressed(void) {
  Event e = {
    .id = PEDAL_BRAKE_FSM_EVENT_PRESSED,
  };
  TEST_ASSERT_FALSE(pedal_can_process_event(&e));
}

void test_pedal_can_brake_released(void) {
  Event e = {
    .id = PEDAL_BRAKE_FSM_EVENT_RELEASED,
  };
  TEST_ASSERT_FALSE(pedal_can_process_event(&e));
}

void test_pedal_can_throttle(void) {
  Event e = {
    .id = PEDAL_EVENT_THROTTLE_READING,
  };
  TEST_ASSERT_TRUE(pedal_can_process_event(&e));
}