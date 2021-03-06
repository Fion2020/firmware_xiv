#include <string.h>
#include "can.h"
#include "can_msg_defs.h"
#include "can_transmit.h"
#include "can_tx_retry_wrapper.h"
#include "delay.h"
#include "event_queue.h"
#include "exported_enums.h"
#include "gpio.h"
#include "interrupt.h"
#include "log.h"
#include "mci_output_tx.h"
#include "ms_test_helpers.h"
#include "status.h"
#include "test_helpers.h"
#include "unity.h"

static MciOutputTxStorage s_storage = { 0 };
static CanStorage s_can_storage;
static CanAckStatus s_ack_status = CAN_ACK_STATUS_OK;
static StatusCode s_ack_cb_status;

typedef enum {
  TEST_MCI_OUTPUT_TX_EVENT_CAN_TX = 0,
  TEST_MCI_OUTPUT_TX_EVENT_CAN_RX,
  TEST_MCI_OUTPUT_TX_EVENT_CAN_FAULT,
  TEST_MCI_OUTPUT_TX_EVENT_SUCCESS,
  TEST_MCI_OUTPUT_TX_EVENT_FAIL
} TestEBrakeTxEvent;

static StatusCode prv_rx_callback(const CanMessage *msg, void *context, CanAckStatus *ack_reply) {
  *ack_reply = s_ack_status;
  return s_ack_cb_status;
}

void setup_test(void) {
  gpio_init();
  event_queue_init();
  interrupt_init();
  soft_timer_init();
  const CanSettings s_can_settings = { .device_id = SYSTEM_CAN_DEVICE_CENTRE_CONSOLE,
                                       .loopback = true,
                                       .bitrate = CAN_HW_BITRATE_500KBPS,
                                       .rx = { .port = GPIO_PORT_A, .pin = 11 },
                                       .tx = { .port = GPIO_PORT_A, .pin = 12 },
                                       .rx_event = TEST_MCI_OUTPUT_TX_EVENT_CAN_RX,
                                       .tx_event = TEST_MCI_OUTPUT_TX_EVENT_CAN_TX,
                                       .fault_event = TEST_MCI_OUTPUT_TX_EVENT_CAN_FAULT };
  TEST_ASSERT_OK(can_init(&s_can_storage, &s_can_settings));
  TEST_ASSERT_OK(can_register_rx_handler(SYSTEM_CAN_MESSAGE_DRIVE_OUTPUT, prv_rx_callback, NULL));
  TEST_ASSERT_OK(mci_output_init(&s_storage));
}

void teardown_test(void) {}

void test_mci_output_retries_then_raises_fail_event(void) {
  for (EEDriveOutput drive_output = EE_DRIVE_OUTPUT_OFF; drive_output < NUM_EE_DRIVE_OUTPUTS;
       drive_output++) {
    uint16_t fault_event_data = 0x1234;
    uint16_t success_event_data = 0x5678;
    RetryTxRequest req = {
      .completion_event_id = TEST_MCI_OUTPUT_TX_EVENT_SUCCESS,
      .completion_event_data = success_event_data,
      .fault_event_id = TEST_MCI_OUTPUT_TX_EVENT_FAIL,
      .fault_event_data = fault_event_data,
    };
    TEST_ASSERT_OK(mci_output_tx_drive_output(&s_storage, &req, drive_output));
    for (uint8_t i = 0; i < NUM_MCI_OUTPUT_TX_RETRIES; i++) {
      MS_TEST_HELPER_CAN_TX_RX_WITH_ACK(TEST_MCI_OUTPUT_TX_EVENT_CAN_TX,
                                        TEST_MCI_OUTPUT_TX_EVENT_CAN_RX);
      MS_TEST_HELPER_ACK_MESSAGE_WITH_STATUS(s_can_storage, SYSTEM_CAN_MESSAGE_DRIVE_OUTPUT,
                                             SYSTEM_CAN_DEVICE_MOTOR_CONTROLLER,
                                             CAN_ACK_STATUS_INVALID);
    }
    Event e = { 0 };
    MS_TEST_HELPER_ASSERT_NEXT_EVENT(e, TEST_MCI_OUTPUT_TX_EVENT_FAIL, fault_event_data);
    // No further events must be raised
    MS_TEST_HELPER_ASSERT_NO_EVENT_RAISED();
  }
}

void test_mci_output_retries_then_success_event(void) {
  for (EEDriveOutput drive_output = EE_DRIVE_OUTPUT_OFF; drive_output < NUM_EE_DRIVE_OUTPUTS;
       drive_output++) {
    uint16_t fault_event_data = 0x1234;
    uint16_t success_event_data = 0x5678;
    RetryTxRequest req = {
      .completion_event_id = TEST_MCI_OUTPUT_TX_EVENT_SUCCESS,
      .completion_event_data = success_event_data,
      .fault_event_id = TEST_MCI_OUTPUT_TX_EVENT_FAIL,
      .fault_event_data = fault_event_data,
    };
    TEST_ASSERT_OK(mci_output_tx_drive_output(&s_storage, &req, drive_output));
    for (uint8_t i = 0; i < NUM_MCI_OUTPUT_TX_RETRIES - 1; i++) {
      MS_TEST_HELPER_CAN_TX_RX_WITH_ACK(TEST_MCI_OUTPUT_TX_EVENT_CAN_TX,
                                        TEST_MCI_OUTPUT_TX_EVENT_CAN_RX);
      MS_TEST_HELPER_ACK_MESSAGE_WITH_STATUS(s_can_storage, SYSTEM_CAN_MESSAGE_DRIVE_OUTPUT,
                                             SYSTEM_CAN_DEVICE_MOTOR_CONTROLLER,
                                             CAN_ACK_STATUS_INVALID);
    }

    MS_TEST_HELPER_CAN_TX_RX_WITH_ACK(TEST_MCI_OUTPUT_TX_EVENT_CAN_TX,
                                      TEST_MCI_OUTPUT_TX_EVENT_CAN_RX);
    MS_TEST_HELPER_ACK_MESSAGE_WITH_STATUS(s_can_storage, SYSTEM_CAN_MESSAGE_DRIVE_OUTPUT,
                                           SYSTEM_CAN_DEVICE_MOTOR_CONTROLLER, CAN_ACK_STATUS_OK);

    Event e = { 0 };
    MS_TEST_HELPER_ASSERT_NEXT_EVENT(e, TEST_MCI_OUTPUT_TX_EVENT_SUCCESS, success_event_data);
    // no further events must be raised
    MS_TEST_HELPER_ASSERT_NO_EVENT_RAISED();
  }
}
