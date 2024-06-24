#!/usr/bin/env python3

import sys
import threading
import cv2
import time
import numpy as np
from accvis import *

try:
    import pigpio
except ImportError:
    pigpio = None

def setledstatus(okay: bool):
    ledblue = 23
    ledred  = 22
    ledon  = 0
    ledoff = 1
    if pigpio is not None:
        pi = pigpio.pi()
        if okay:
            pi.write(ledred, ledoff)
            pi.write(ledblue, ledoff)
        else:
            pi.write(ledred, ledon)
            pi.write(ledblue, ledoff)


# This class was based on content at https://stackoverflow.com/a/69141497
# with modifications to average the last n frames.

class AccCapture:
    def __init__(self, name, backend=None, nframes=15):
        try:
            if backend is None:
                self.cap = cv2.VideoCapture(name)
            else:
                self.cap = cv2.VideoCapture(name, backend)
        except:
            print("Could not open VideoCapture!")
            return
        self._name = name
        self._backend = backend
        self.nframes = nframes
        self.frame = []
        self.lock = threading.Lock()
        self.t = threading.Thread(target=self._reader)
        self.t.daemon = True
        self.t.start()
    # grab frames as soon as they are available
    def _reader(self):
        fc = 0
        while True:
            with self.lock:
                ret = self.cap.grab()
            if not ret:
                print("Failed to grab a frame!")
                time.sleep(1/self.nframes)
                fc = fc + 1
                if(fc < self.nframes*5):
                    continue
                else:
                    print("Automatically restarting video capture")
                    with self.lock:
                        self.cap.release()
                        if self._backend is None:
                            self.cap = cv2.VideoCapture(self._name)
                        else:
                            self.cap = cv2.VideoCapture(
                                self._name, self._backend)
    # retrieve the latest frame and the nframes-1 that come after.
    def read(self):
        with self.lock:
            ret, frame = self.cap.retrieve()
            if ret == False:
                return [False, None]
            acc = np.zeros_like(frame, dtype=np.float32) # empty Mat for average
            cv2.accumulate(frame, acc)
            j = 1
            for i in range (2, self.nframes):
                ret, frame = self.cap.read()
                if ret == False:
                    print("Failed to grab a frame!")
                    continue
                cv2.accumulate(frame, acc)
                j = j+1
            avgframe = cv2.convertScaleAbs(acc / j)
            return [True, avgframe]
    def isOpened(self):
        with self.lock:
            return self.cap.isOpened()
    def release(self):
        self.cap.release()

def drawRectangles(frame: cv2.Mat):
    for feature in AccKeyFeatures.FeatureDict.values():
        color = (0,0,255)
        cv2.rectangle(frame, feature.start_point, feature.end_point, color, 1)
    return frame

def drawHSVText(frame: cv2.Mat):
    parser = FeatureParser(sourceImage=frame)
    for key,feature in AccKeyFeatures.FeatureDict.items():
        color = (0,0,255)
        font = cv2.FONT_HERSHEY_SIMPLEX
        scale = 0.2
        parser.feature = feature
        text = str(parser.avgHSV)
        position = (feature.end_point[0] + 0, feature.end_point[1] + 0)
        if(any( x == key for x in ['MSDE', 'MSDF', 'LSDE', 'LSDF'])):
            position = (feature.end_point[0] + 0, feature.end_point[1] - 7)
        cv2.putText(frame, text, position, 
                    font, scale, color, 1, cv2.LINE_AA)
    return frame

def drawTruthText(frame: cv2.Mat):
    parser = FeatureParser(sourceImage=frame)
    for key,feature in AccKeyFeatures.FeatureDict.items():
        color = (0,0,255)
        font = cv2.FONT_HERSHEY_SIMPLEX
        scale = 0.2
        parser.feature = feature
        text = str(parser.evaluateThreshold())
        position = (feature.end_point[0] + 0, feature.end_point[1] + 10)
        if(any( x == key for x in ['MSDE', 'MSDF', 'LSDE', 'LSDF'])):
            position = (feature.end_point[0] + 0, feature.end_point[1] + 3)
        cv2.putText(frame, text, position, 
                    font, scale, color, 1, cv2.LINE_AA)
    return frame

def parseFrame(frame: cv2.Mat):
    panelparser = AccPanelParser(sourceImage=frame)
    panelparser.parse()
    print(repr(panelparser._panel))
    print(str(panelparser._panel))
    panelparser.transmit()

def main() -> int:
    try:
        cap = AccCapture("udp://@:5000", cv2.CAP_FFMPEG, nframes=5)
    except:
        print("Could not open VideoCapture!")
        print(cap)

    while(cap.isOpened()):
        ret, frame = cap.read()
        if(ret == True):
            ai = AccImage(frame)
            normframe = ai.norm
            try:
                cv2.imshow("Live feed", frame)
                skipdrawing = false
            except:
                skipdrawing = True
                pass
            if normframe is not None:
                parseFrame(normframe)
                # Parse FIRST. The functions below alter normimage!!!
                if not skipdrawing:
                    normframe = drawRectangles(normframe)
                    normframe = drawHSVText(normframe)
                    normframe = drawTruthText(normframe)
                    cv2.imshow("Normalized live feed", normframe)
                setledstatus(okay=True)
            else:
                setledstatus(okay=False)
            if cv2.waitKey(1) == ord('q'):
                break

    print("Capture was closed.")
    cap.release()
    cv2.destroyAllWindows()
    return 0

if __name__ == '__main__':
    sys.exit(main())

