#!/usr/bin/env python3

from collections import namedtuple
import glob
import cv2
import numpy as np

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



cap = cv2.VideoCapture("udp://@239.0.0.10:5000", cv2.CAP_FFMPEG)

while(cap.isOpened()):
    ret, frame = cap.read()
    if(ret == True):
        ai = AccImage(frame)
        normframe = ai.norm
        if normframe is None:
            normframe = np.zeros(shape=[512,512,3], dtype=np.uint8)
        cv2.imshow("Live feed", frame)
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
