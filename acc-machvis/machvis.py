#!/usr/bin/env python3

import sys
import threading
import cv2
import time
import numpy as np
from accvis import *

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
        self.nframes = nframes
        self.lock = threading.Lock()
        self.t = threading.Thread(target=self._reader)
        self.t.daemon = True
        self.t.start()
    # grab frames as soon as they are available
    def _reader(self):
        while True:
            with self.lock:
                ret = self.cap.grab()
            if not ret:
                print("Failed to grab a capture!")
                continue
    # retrieve the latest frame and the nframes-1 that come after.
    def read(self):
        with self.lock:
            ret, frame = self.cap.retrieve()
            if ret == False:
                return [False, None]
            acc = np.zeros_like(frame, dtype=np.float32) # empty Mat for average
            cv2.accumulate(frame, acc)
            for i in range (2, self.nframes):
                ret, frame = self.cap.read()
                if ret == False:
                    continue
                cv2.accumulate(frame, acc)
            avgframe = cv2.convertScaleAbs(acc / self.nframes)
            return [True, avgframe]
    def isOpened(self):
        return self.cap.isOpened()

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
        cap = AccCapture("udp://@:5000", cv2.CAP_FFMPEG, nframes=15)
    except:
        print("Could not open VideoCapture!")
        print(cap)

    while(cap.isOpened()):
        ret, frame = cap.read()
        if(ret == True):
            ai = AccImage(frame)
            normframe = ai.norm
            try:
                # Routine for desktop
                # Will throw exception on graphic-less production environment
                cv2.imshow("Live feed", frame)
                if normframe is not None:
                    parseFrame(normframe)
                    # Parse FIRST. The functions below alter normimage!!!
                    normframe = drawRectangles(normframe)
                    normframe = drawHSVText(normframe)
                    normframe = drawTruthText(normframe)
                    cv2.imshow("Normalized live feed", normframe)
                if cv2.waitKey(1) == ord('q'):
                    break
            except:
                if normframe is not None:
                    parseFrame(normframe)

    print("Capture was closed.")
    cap.release()
    cv2.destroyAllWindows()
    return 0

if __name__ == '__main__':
    sys.exit(main())

