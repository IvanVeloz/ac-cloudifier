#!/usr/bin/env python3

import cv2
import time
import sys
import numpy as np
import numpy.typing as npt
from typing import Optional
from dataclasses import dataclass
from accvis import *

class FakePanelParser(AccPanelParser):
    pass
    def parse(self):
        self._panel.msdigit = self._sevenincrement(self._panel.msdigit)
        self._panel.lsdigit = self._sevenincrement(self._panel.lsdigit)
        
        tmp = self._genericincrement(self._panel.delay.value, 3)
        self._panel.fan = AccPanelFan(tmp)

        tmp = self._genericincrement(self._panel.delay.value, 2)
        self._panel.mode = AccPanelMode(tmp)

        tmp = self._genericincrement(self._panel.delay.value, 1)
        self._panel.delay = AccPanelDelay(tmp)

    def _sevenincrement(self, digit: int) -> int:
        r = self._genericincrement(digit, 9)
        return r
    
    def _genericincrement(self, value: int, limit: int) -> int:
        v = value
        if v >= limit:
            v = 0
        else:
            v = v + 1
        return v
    
def main() -> int:
    fakeparser = FakePanelParser(None, None)
    while(1):
        print(str(fakeparser._panel))
        fakeparser.transmit()
        time.sleep(1)
    return 0

if __name__ == '__main__':
    sys.exit(main())
