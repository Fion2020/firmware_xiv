# Defines $(T)_SRC, $(T)_INC, $(T)_DEPS, and $(T)_CFLAGS for the build makefile.
# Tests can be excluded by defining $(T)_EXCLUDE_TESTS.
# Pre-defined:
# $(T)_SRC_ROOT: $(T)_DIR/src
# $(T)_INC_DIRS: $(T)_DIR/inc{/$(PLATFORM)}
# $(T)_SRC: $(T)_DIR/src{/$(PLATFORM)}/*.{c,s}

# Specify the libraries you want to include
$(T)_DEPS := ms-common ms-helper ms-drivers

ifeq (x86,$(PLATFORM))
$(T)_test_killswitch_MOCKS := gpio_get_state fault_bps_set fault_bps_clear
$(T)_test_bps_heartbeat_MOCKS := fault_bps_set fault_bps_claer
$(T)_test_relay_sequence_MOCKS := fault_bps_set fault_bps_clear gpio_set_state mcp23008_gpio_get_state
$(T)_test_cell_sense_MOCKS := current_sense_is_charging ltc_afe_process_event
$(T)_test_passive_balance_MOCKS := ltc_afe_toggle_cell_discharge
$(T)_test_fault_bps_MOCKS := relay_fault
$(T)_test_current_sense_MOCKS := fault_bps_set fault_bps_clear ads1259_get_conversion_data ads1259_init
endif
# Uses mocked fault handling to verify internal logic
# TODO(SOFT-61): Should allow modules to hook into internal fault state (or read can messages) so this isn't needed
$(T)_test_cell_sense_MOCKS += fault_bps_set fault_bps_clear
