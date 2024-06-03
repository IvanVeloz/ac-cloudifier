#!/usr/bin/env python3

import cv2
import numpy as np
import numpy.typing as npt
from typing import Optional
from dataclasses import dataclass
from accvis import *


def drawRectangles(frame: cv2.Mat):
    for feature in AccKeyFeatures.FeatureDict.values():
        color = (0,0,255)
        cv2.rectangle(frame, feature.start_point, feature.end_point, color, 1)
    return frame

def drawHSVText(frame: cv2.Mat):
    parser = FeatureParser(sourceImage=frame)
    for feature in AccKeyFeatures.FeatureDict.values():
        color = (0,0,255)
        font = cv2.FONT_HERSHEY_SIMPLEX
        scale = 0.4
        parser.feature = feature
        text = str(parser.avgHSV)
        cv2.putText(frame, text, feature.end_point, 
                    font, scale, color, 1, cv2.LINE_AA)
    return frame

def drawTruthText(frame: cv2.Mat):
    parser = FeatureParser(sourceImage=frame)
    for feature in AccKeyFeatures.FeatureDict.values():
        color = (0,0,255)
        font = cv2.FONT_HERSHEY_SIMPLEX
        scale = 0.4
        parser.feature = feature
        text = str(parser.evaluateThreshold())
        position = (feature.end_point[0] + 0, feature.end_point[1] + 10)
        cv2.putText(frame, text, position, 
                    font, scale, color, 1, cv2.LINE_AA)
    return frame

cap = cv2.VideoCapture("udp://@:5000", cv2.CAP_FFMPEG)
while(cap.isOpened()):
    ret, frame = cap.read()
    if(ret == True):
        ai = AccImage(frame)
        normframe = ai.norm
        cv2.imshow("Live feed", frame)
        if normframe is not None:
            normframe = drawRectangles(normframe)
            normframe = drawHSVText(normframe)
            normframe = drawTruthText(normframe)
            cv2.imshow("Normalized live feed", normframe)
        if cv2.waitKey(1) == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()
exit(0)
