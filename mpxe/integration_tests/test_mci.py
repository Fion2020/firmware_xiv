import sys
import os
from os.path import dirname
sys.path.append(dirname(sys.path[0]))

import unittest
import time

import int_test

class TestMci(int_test.IntTest):
    def setUp(self):
        super(TestMci, self).setUp()
        self.mci = self.manager.start('mci')
    
    def test_mci(self):
        time.sleep(1)
        self.assertNotEqual(self.mci.sim.tx_id, 0)
        self.assertEqual(self.mci.sim.tx_data, 0)

        self.manager.can.send('BEGIN_PRECHARGE', None)
        time.sleep(1)
        self.assert_can_got('PRECHARGE_COMPLETED')

        self.can_send('PEDAL_OUTPUT', {'throttle_output': 50, 'brake_output': 0})
        time.sleep(1)
        self.assertEqual(self.mci.sim.tx_data, 0)

        self.can_send('DRIVE_OUTPUT', {'drive_output': 1})
        time.sleep(1)
        self.can_send('PEDAL_OUTPUT', {'throttle_output': 50, 'brake_output': 0})
        time.sleep(1)
        self.assertNotEqual(self.mci.sim.tx_data, 0)

if __name__ == '__main__':
    unittest.main()
