#!/usr/bin/env python3

from collections import namedtuple
import glob
import cv2
import numpy as np
from dataclasses import dataclass

imgpath = glob.glob(
    '/home/ivan/Sources/ac-cloudifier/acc-machvis/imgs/with_four_aruco/*.jpg')
images = []

class AccImage:
    def __init__ (self, srcimg: cv2.Mat):
        self.src = srcimg
        self._magicmarkers = (100, 101, 102, 103) 
        # These are the codes stuck to the AC unit, clockwise from top left

    def _detectMarkers(self):
        # Aruco detection
        magicmarkers = (100, 101, 102, 103) # markers in OpenCV clockwise order
        dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_4X4_250)
        detectorparams = cv2.aruco.DetectorParameters()
        detector = cv2.aruco.ArucoDetector(dict, detectorparams)

        (cornersarray, idarray, rejectedImgPointsarray) = \
            detector.detectMarkers(self._src)
        self._markers = (cornersarray, idarray, rejectedImgPointsarray)
        

    def _detectCorners(self):
        (cornersarray, idarray, rejectedImgPointsarray) = self._markers        
        fourcorners = {}
        if idarray is not None:
            for n,id in enumerate(idarray):
                for m in self._magicmarkers:
                    if(id == m):
                        fourcorners[m] = cornersarray[n]
            if len(fourcorners) != 4:
                print('Could not find all aruco markers! Found:')
                print(fourcorners.keys())
                self._corners = {}
                return
            print('Found all aruco markers. Found:')
            print(fourcorners.keys())
            self._corners = fourcorners
        else:
            print('Could not find aruco')
            self._corners = {}

    def _normalizePerspective(self):
        # Perspective transformation
        if len(self._corners) == 4:
            points1 = np.float32([
                self._corners[self._magicmarkers[0]][0][0],
                self._corners[self._magicmarkers[1]][0][1],
                self._corners[self._magicmarkers[2]][0][2],
                self._corners[self._magicmarkers[3]][0][3],
                ])
            points2 = np.float32([
                [0,0],
                [530,0],
                [530,1150],
                [0,1150]])
            dimensions = (530,1150)
            # After the transform, one pixel = 0.1mm
            ap1 = points1#[0:3]
            ap2 = points2#[0:3]
            M = cv2.getPerspectiveTransform(ap1, ap2)
            rows,cols,ch = self._src.shape
            normalized = cv2.warpPerspective(self._src, M, (dimensions))
            self._norm = normalized
        else:
            self._norm = None

    def _processNorm(self):
        self._detectMarkers()
        self._detectCorners()
        self._normalizePerspective()

    def _setsrc(self, srcimg):
        self._src = cv2.rotate(srcimg, cv2.ROTATE_90_CLOCKWISE)
        self._normIsProcessed = False
    
    def _getsrc(self):
        return self._src
    
    def _getnorm(self):
        if not self._normIsProcessed:
            self._processNorm()
            self._normIsProcessed = True
        return self._norm

    @classmethod
    def getnorm(srcimg: cv2.Mat):
        ai = AccImage(srcimg)
        return ai.norm

    src = property(fget=_getsrc, fset=_setsrc)
    norm = property(fget=_getnorm)

class AccParser:
    def __init__(self, accimage: AccImage):
        self._src = accimage.norm

@dataclass 
class FeatureSize:
    width: int
    height: int

# Coordinates of a feature in pixels. Always use the top left corner of the 
# feature.
class FeatureCoords:
    x: int
    y: int
    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y
    def __add__(self, val2):
        return FeatureCoords(x=(self.x + val2.x), y=(self.y + val2.y))

@dataclass
class Feature:
    size: FeatureSize
    location: FeatureCoords
    def get_start_point(self):
        return (self.location.x, 
                self.location.y)
    def get_end_point(self):
        return (self.location.x + self.size.width,
                self.location.y + self.size.height)
    start_point = property(fget=get_start_point)
    end_point = property(fget=get_end_point)    

@dataclass
class AccKeyFeatures:
    """ Class which contains key image features for the AccParser class.
        All of the locations are defined in ratios to the normalized dimensions.
    """

    # This is the size of the normalized, perspective-corrected image. All other
    # dimensions are based on this. In pixels.
    normalizedImageDimensions = np.float32([
        [0,0],
        [530,0],
        [530,1150],
        [0,1150]])
    
    ledSize = FeatureSize(width=20, height=20)
    verticalSegmentSize = FeatureSize(width=10, height=20)
    horizontalSegmentSize = FeatureSize(width=20, height=15)
    displayBackgroundSize = FeatureSize(width=100, height=50)

    FanAuto  = Feature(size=ledSize, location=FeatureCoords(x=43, y=593))
    FanHigh  = Feature(size=ledSize, location=FeatureCoords(x=43, y=665))
    FanMed   = Feature(size=ledSize, location=FeatureCoords(x=43, y=734))
    FanLow   = Feature(size=ledSize, location=FeatureCoords(x=43, y=804))

    ModeCool = Feature(size=ledSize, location=FeatureCoords(x=215, y=593))
    ModeFan  = Feature(size=ledSize, location=FeatureCoords(x=215, y=665))
    ModeEco  = Feature(size=ledSize, location=FeatureCoords(x=215, y=734))

    DelayOn  = Feature(size=ledSize, location=FeatureCoords(x=392, y=593))
    DelayOff = Feature(size=ledSize, location=FeatureCoords(x=392, y=665))

    # Display background. Used for comparison against the display segments.
    displayBackground = Feature(size=displayBackgroundSize, location=FeatureCoords(x=215,y=235))

    # Relative positions of display segments
    RelA = FeatureCoords(x=216,y=51)
    RelB = FeatureCoords(x=241,y=70)
    RelC = FeatureCoords(x=236,y=111)
    RelD = FeatureCoords(x=210,y=135)
    RelE = FeatureCoords(x=195,y=111)
    RelF = FeatureCoords(x=200,y=70)
    RelG = FeatureCoords(x=213,y=93)

    # Offsets for 7 segment displays
    OffMSD = FeatureCoords(x=0, y=0)
    OffLSD = FeatureCoords(x=75, y=0)

    MSDA = Feature(size=horizontalSegmentSize, location = RelA+OffMSD)
    MSDB = Feature(size=verticalSegmentSize,   location = RelB+OffMSD)
    MSDC = Feature(size=verticalSegmentSize,   location = RelC+OffMSD)
    MSDD = Feature(size=horizontalSegmentSize, location = RelD+OffMSD)
    MSDE = Feature(size=verticalSegmentSize,   location = RelE+OffMSD)
    MSDF = Feature(size=verticalSegmentSize,   location = RelF+OffMSD)
    MSDG = Feature(size=horizontalSegmentSize, location = RelG+OffMSD)

    LSDA = Feature(size=horizontalSegmentSize, location = RelA+OffLSD)
    LSDB = Feature(size=verticalSegmentSize,   location = RelB+OffLSD)
    LSDC = Feature(size=verticalSegmentSize,   location = RelC+OffLSD)
    LSDD = Feature(size=horizontalSegmentSize, location = RelD+OffLSD)
    LSDE = Feature(size=verticalSegmentSize,   location = RelE+OffLSD)
    LSDF = Feature(size=verticalSegmentSize,   location = RelF+OffLSD)
    LSDG = Feature(size=horizontalSegmentSize, location = RelG+OffLSD)

    FeatureDict = {
        "FanAuto"               :   FanAuto,
        "FanHigh"               :   FanHigh,
        "FanMed"                :   FanMed,
        "FanLow"                :   FanLow,
        "ModeCool"              :   ModeCool,
        "ModeFan"               :   ModeFan,
        "ModeEco"               :   ModeEco,
        "DelayOn"               :   DelayOn,
        "DelayOff"              :   DelayOff,
        "displayBackground"     :   displayBackground,
        "MSDA"                  :   MSDA,
        "MSDB"                  :   MSDB,
        "MSDC"                  :   MSDC,
        "MSDD"                  :   MSDD,
        "MSDE"                  :   MSDE,
        "MSDF"                  :   MSDF,
        "MSDG"                  :   MSDG,
        "LSDA"                  :   LSDA,
        "LSDB"                  :   LSDB,
        "LSDC"                  :   LSDC,
        "LSDD"                  :   LSDD,
        "LSDE"                  :   LSDE,
        "LSDF"                  :   LSDF,
        "LSDG"                  :   LSDG
    }

def drawRectangles(frame: cv2.Mat):
    for feature in AccKeyFeatures.FeatureDict.values():
        color = (0,0,255)
        cv2.rectangle(frame, feature.start_point, feature.end_point, color, 1)
    return frame

cap = cv2.VideoCapture("udp://@:5000", cv2.CAP_FFMPEG)
while(cap.isOpened()):
    ret, frame = cap.read()
    if(ret == True):
        ai = AccImage(frame)
        normframe = ai.norm
        cv2.imshow("Live feed", frame)
        #if normframe is None:
        #    normframe = np.zeros(shape=[512,512,3], dtype=np.uint8)
        if normframe is not None:
            normframe = drawRectangles(normframe)
            cv2.imshow("Normalized live feed", normframe)
        if cv2.waitKey(1) == ord('q'):
            break

cap.release()
cv2.destroyAllWindows()
exit(0)

images = []

for img in imgpath:
    i = cv2.imread(img)
    ai = AccImage(i)
    images.append(ai)

# Now do a slideshow
for n,img in enumerate(images):
    img.showall("Image {} - ".format(n))
    cv2.waitKey()
    img.destroyall()

cv2.destroyAllWindows()
