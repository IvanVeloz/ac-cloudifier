import cv2
import numpy as np
import json
import socket
from typing import Optional
from dataclasses import dataclass
from enum import Enum
from collections import namedtuple

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
class FeatureValueTriad:
    hue: np.uint8 = 0
    sat: np.uint8 = 0
    val: np.uint8 = 255

class FeatureThreshold(FeatureValueTriad):
    def evaluate(self, triad: FeatureValueTriad) -> bool:
        return triad.val > self.val

class SevenThreshold(FeatureThreshold): 
    def evaluate(self, triad: FeatureValueTriad) -> bool:
        if (self.hue - 30) <= triad.hue <= (self.hue + 20):
            if triad.val > self.val:
                return True
        return False


@dataclass
class Feature:
    size: FeatureSize
    location: FeatureCoords
    threshold: Optional[FeatureThreshold] = None
    def get_start_point(self):
        return (self.location.x, 
                self.location.y)
    def get_end_point(self):
        return (self.location.x + self.size.width,
                self.location.y + self.size.height)
    start_point = property(fget=get_start_point)
    end_point = property(fget=get_end_point)    

# Determines the truth value of a feature
class FeatureParser:
    def __init__(self, feature: Feature = None, sourceImage: cv2.Mat = None):
        self.feature = feature
        self.sourceImage = sourceImage
    def getSourceImage(self):
        return self._image
    def setSourceImage(self, sourceImage: cv2.Mat = None):
        self._image = sourceImage
        self._HSVIsUpdated = False
        self._RegionOfInterestIsUpdated = False
    def getFeature(self):
        return self._feature
    def setFeature(self, feature: Feature = None):
        self._feature = feature
        self._RegionOfInterestIsUpdated = False
        self._AvgHSVIsUpdated = False
    def _getHSVImage(self):
        if self._HSVIsUpdated == False:
            if(self._image is not None):
                self._HSVImage = cv2.cvtColor(self._image, cv2.COLOR_BGR2HSV)
            else:
                self._HSVImage = None
            self._HSVIsUpdated = True
        return self._HSVImage
    def _getRegionOfInterest(self):
        if self._RegionOfInterestIsUpdated == False:
            if(self._feature is not None and self.HSVImage is not None):
                sx, sy = self._feature.start_point
                fx, fy = self._feature.end_point
                self._ROI = self.HSVImage[sy:fy, sx:fx]
            else:
                self._ROI = None
            self._RegionOfInterestIsUpdated = True
        return self._ROI
    def _getAvgHSV(self):
        if self._AvgHSVIsUpdated == False:
            if self.ROI is not None:
                avgRow = np.average(self.ROI, axis=0)
                avg    = np.average(avgRow, axis=0)
            else:
                avg = None
            self._AvgHSV = avg.astype(np.uint8)
            self._AvgHSVIsUpdated = True
        return self._AvgHSV
    def getAvgHue(self):
        return self.avgHSV[0]
    def getAvgSat(self):
        return self.avgHSV[1]
    def getAvgVal(self):
        return self.avgHSV[2]
    def evaluateThreshold(self) -> bool:
        if self.feature.threshold is not None:
            hue = self.getAvgHue()
            sat = self.getAvgSat()
            val = self.getAvgVal()
            return self.feature.threshold.evaluate(FeatureValueTriad(hue,sat,val))
        else:
            return False
    feature = property(fset=setFeature, fget=getFeature)
    sourceImage = property(fset=setSourceImage, fget=getSourceImage)
    HSVImage = property(fget=_getHSVImage)
    ROI = property(fget=_getRegionOfInterest)
    avgHSV = property(fget=_getAvgHSV)
    avgHue = property(fget=getAvgHue)
    avgSat = property(fget=getAvgSat)
    avgVal = property(fget=getAvgVal)

# Data structure containing the constants specific to the GE air conditioner
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

    ledThreshold = FeatureThreshold(val = 130)
    sevenThreshold = SevenThreshold(hue=60, val=80)

    FanAuto  = Feature(size=ledSize, location=FeatureCoords(x=43, y=593), threshold=ledThreshold)
    FanHigh  = Feature(size=ledSize, location=FeatureCoords(x=43, y=665), threshold=ledThreshold)
    FanMed   = Feature(size=ledSize, location=FeatureCoords(x=43, y=734), threshold=ledThreshold)
    FanLow   = Feature(size=ledSize, location=FeatureCoords(x=43, y=804), threshold=ledThreshold)

    ModeCool = Feature(size=ledSize, location=FeatureCoords(x=215, y=593), threshold=ledThreshold)
    ModeFan  = Feature(size=ledSize, location=FeatureCoords(x=215, y=665), threshold=ledThreshold)
    ModeEco  = Feature(size=ledSize, location=FeatureCoords(x=215, y=734), threshold=ledThreshold)

    DelayOn  = Feature(size=ledSize, location=FeatureCoords(x=392, y=593), threshold=ledThreshold)
    DelayOff = Feature(size=ledSize, location=FeatureCoords(x=392, y=665), threshold=ledThreshold)

    Filter = Feature(size=ledSize, location=FeatureCoords(x=251, y=1065), threshold=ledThreshold)

    # Display background. Used for comparison against the display segments.
    displayBackground = Feature(size=displayBackgroundSize, location=FeatureCoords(x=215,y=235))

    # Panel background. White part of the panel. Used for comparison.
    panelBackground = Feature(size=displayBackgroundSize, location=FeatureCoords(x=215,y=380))

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

    MSDA = Feature(size=horizontalSegmentSize, location = RelA+OffMSD, threshold=sevenThreshold)
    MSDB = Feature(size=verticalSegmentSize,   location = RelB+OffMSD, threshold=sevenThreshold)
    MSDC = Feature(size=verticalSegmentSize,   location = RelC+OffMSD, threshold=sevenThreshold)
    MSDD = Feature(size=horizontalSegmentSize, location = RelD+OffMSD, threshold=sevenThreshold)
    MSDE = Feature(size=verticalSegmentSize,   location = RelE+OffMSD, threshold=sevenThreshold)
    MSDF = Feature(size=verticalSegmentSize,   location = RelF+OffMSD, threshold=sevenThreshold)
    MSDG = Feature(size=horizontalSegmentSize, location = RelG+OffMSD, threshold=sevenThreshold)

    LSDA = Feature(size=horizontalSegmentSize, location = RelA+OffLSD, threshold=sevenThreshold)
    LSDB = Feature(size=verticalSegmentSize,   location = RelB+OffLSD, threshold=sevenThreshold)
    LSDC = Feature(size=verticalSegmentSize,   location = RelC+OffLSD, threshold=sevenThreshold)
    LSDD = Feature(size=horizontalSegmentSize, location = RelD+OffLSD, threshold=sevenThreshold)
    LSDE = Feature(size=verticalSegmentSize,   location = RelE+OffLSD, threshold=sevenThreshold)
    LSDF = Feature(size=verticalSegmentSize,   location = RelF+OffLSD, threshold=sevenThreshold)
    LSDG = Feature(size=horizontalSegmentSize, location = RelG+OffLSD, threshold=sevenThreshold)

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
        "Filter"                :   Filter,
        "displayBackground"     :   displayBackground,
        "panelBackground"       :   panelBackground,
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

class AccPanelFan(Enum):
    pass
    FAN_NONE = 0
    FAN_AUTO = 1
    FAN_HIGH = 2
    FAN_MED  = 3
    FAN_LOW  = 4
    
class AccPanelMode(Enum):
    pass
    MODE_NONE = 0
    MODE_COOL = 1
    MODE_FAN  = 2
    MODE_ECO  = 3

class AccPanelDelay(Enum):
    pass
    DELAY_NONE = 0
    DELAY_ON   = 1
    DELAY_OFF  = 2

SevenSegment = namedtuple('SevenSegment', 
                          ['a', 'b', 'c', 'd',
                           'e', 'f', 'g'])

class AccParsedPanel:
    def __init__(self, 
                 fan:   AccPanelFan     =   AccPanelFan(0),
                 mode:  AccPanelMode    =   AccPanelMode(0),
                 delay: AccPanelDelay   =   AccPanelDelay(0),
                 msdigit: int           =   -1,
                 lsdigit: int           =   -1,
                 filterbad: bool        =   False
                 ):

        self.fan       = fan
        self.mode      = mode
        self.delay     = delay
        self.msdigit   = msdigit
        self.lsdigit   = lsdigit
        self.filterbad = filterbad

    def __repr__(self):
        return f'AccParsedPanel({repr(self.fan.value)},{repr(self.mode.value)},{repr(self.delay.value)},{self.msdigit},{self.lsdigit},{self.filterbad})'
    
    def __dict__(self) -> dict:
        return {
            'fan'       :   self.fan.name,
            'mode'      :   self.mode.name,
            'delay'     :   self.delay.name,
            'msdigit'   :   self.msdigit,
            'lsdigit'   :   self.lsdigit,
            'filterbad' :   str(self.filterbad)
        }
    def __str__(self):
        d = self.__dict__()
        return json.dumps(d)

# Determines the state of the AC panel
class AccPanelParser:
    def __init__(self, 
                 sourceImage: AccImage,
                 keyfeatures: AccKeyFeatures = AccKeyFeatures()):
        self._panel = AccParsedPanel()
        self._keyfeatures = keyfeatures
        self._parser = FeatureParser(sourceImage=sourceImage)
        self._socketfam = socket.AF_INET
        self._socketpath = 'localhost'    # UNIX socket path, or IP address
        self._socketport = 64000

    def parse(self):
        featureVals = {}
        for key, feature in self._keyfeatures.FeatureDict.items():
            self._parser.feature = feature
            featureVals[key] = self._parser.evaluateThreshold()
        
        fv = featureVals
        msd = SevenSegment(fv['MSDA'], fv['MSDB'], fv['MSDC'], fv['MSDD'], 
                           fv['MSDE'], fv['MSDF'], fv['MSDG'])
        lsd = SevenSegment(fv['LSDA'], fv['LSDB'], fv['LSDC'], fv['LSDD'], 
                           fv['LSDE'], fv['LSDF'], fv['LSDG'])
        fan = [fv['FanAuto'], fv['FanHigh'], fv['FanMed'], fv['FanLow']]
        mode = [fv['ModeCool'], fv['ModeFan'], fv['ModeEco']]
        delay = [fv['DelayOn'], fv['DelayOff']]
        filterbad = fv['Filter']

        self._panel.msdigit = self._sevendecode(msd)
        self._panel.lsdigit = self._sevendecode(lsd)
        self._panel.fan = AccPanelFan(self._rowdecode(fan))
        self._panel.mode = AccPanelMode(self._rowdecode(mode))
        self._panel.delay = AccPanelDelay(self._rowdecode(delay))
        self._panel.filterbad = self._filterdecode(filterbad)

    def transmit(self):
        try:
            self._socket
        except AttributeError:
            self._socket = socket.socket(self._socketfam, socket.SOCK_DGRAM)
        self.parse()
        self._socket.sendto(
            bytes(str(self._panel), 'utf-8') + b'\x00', 
            (self._socketpath, self._socketport))
    
    def _sevendecode(self, s: SevenSegment) -> int:
        encdict = {
            0   :   SevenSegment(True,True,True,True,True,True,False),
            1   :   SevenSegment(False,True,True,False,False,False,False),
            2   :   SevenSegment(True,True,False,True,True,False,True),
            3   :   SevenSegment(True,True,True,True,False,False,True),
            4   :   SevenSegment(False,True,True,False,False,True,True),
            5   :   SevenSegment(True,False,True,True,False,True,True),
            6   :   SevenSegment(True,False,True,True,True,True,True),
            7   :   SevenSegment(True,True,True,False,False,False,False),
            8   :   SevenSegment(True,True,True,True,True,True,True),
            9   :   SevenSegment(True,True,True,True,False,True,True)
        }
        for nd, sd in encdict.items():
            if s == sd:
                return nd
        return -1
    
    def _rowdecode(self, row: dict) -> int:
        for i, val in enumerate(row):
            if val == True:
                return i + 1
        return 0
    
    def _filterdecode(self, filterbad: bool) -> bool:
        if self._panel.filterbad == True:
            return True
        else:
            return filterbad
