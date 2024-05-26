#!/usr/bin/env python3

from collections import namedtuple
import glob
import cv2
import numpy as np

imgpath = glob.glob('/home/ivan/Sources/ac-cloudifier/acc-machvis/imgs/with_aruco/*.jpg')
images = []

class AccImage:
    def __init__ (self, srcimg: cv2.Mat):
        srcrot = cv2.rotate(srcimg, cv2.ROTATE_90_CLOCKWISE)
        srchsv = cv2.cvtColor(srcrot, cv2.COLOR_BGR2HSV)

        # Aruco detection
        dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_5X5_250)
        magicmarker = 105
        detectorparams = cv2.aruco.DetectorParameters()
        detector = cv2.aruco.ArucoDetector(dict, detectorparams)

        (cornersarray, idarray, rejectedImgPointsarray) = \
            detector.detectMarkers(srcrot)

        if idarray:
            for n,id in enumerate(idarray):
                if(id == magicmarker):
                    corners = cornersarray[n]
        else:
            print('Could not find aruco number {}'.format(magicmarker))
        
        # Affine transformation
        if cornersarray:
            points1 = corners[0]
            points2 = np.float32([[500,300],
                                [600,300],
                                [600,400],
                                [500,400]])
            dimensions = (600,1200)
            # After the transform, one pixel = 0.1mm
            ap1 = points1[0:3]
            ap2 = points2[0:3]
            M = cv2.getAffineTransform(ap1, ap2)
            rows,cols,ch = srcrot.shape
            dst = cv2.warpAffine(srcrot, M, (dimensions))
            cv2.imshow("Original", srcrot)
            cv2.imshow("Transformed", dst)
            cv2.waitKey()
            cv2.destroyWindow("Original")
            cv2.destroyWindow("Transformed")
            # TODO: store the images as transformed with perspective correction.
        else:
            cv2.imshow("NOT Transformed, could not find Aruco", srcrot)
            cv2.waitKey()
            cv2.destroyWindow("NOT Transformed, could not find Aruco")
    
        srchsv = cv2.cvtColor(srcrot, cv2.COLOR_BGR2HSV)

        # Masking
        #These masks are good for the LEDs, iffy for the 7 segment display
        lower_nhue = np.array([85,0,0])              # negated hue
        upper_nhue = np.array([180,255,255])         # negated hue
        lower_brightness = np.array([0,30,70])       # allowed brightness
        upper_brightness = np.array([255,255,255])   # allowed brightness

        hue_m = cv2.bitwise_not(cv2.inRange(srchsv, lower_nhue, upper_nhue))
        sat_m = cv2.inRange(srchsv,lower_brightness, upper_brightness)
        mask = cv2.bitwise_and(hue_m, sat_m)
        srcmaskedled = cv2.bitwise_and(srcrot, srcrot, mask=mask)

        self.sourceImage: cv2.Mat = srcrot
        self.hsvImage: cv2.Mat = srchsv
        self.maskedLedImage: cv2.Mat = srcmaskedled

    def showall(self, prefix: str = '', suffix: str = ''):
        p,s = prefix, suffix
        cv2.imshow("{}sourceImage{}".format(p,s), self.sourceImage)
        cv2.imshow("{}hsvImage{}".format(p,s), self.hsvImage)
        cv2.imshow("{}maskedLedImage{}".format(p,s), self.maskedLedImage)
        self._showallxfix = (p,s)

    def destroyall(self):
        if(self._showallxfix):
            p, s = self._showallxfix
            cv2.destroyWindow("{}sourceImage{}".format(p,s))
            cv2.destroyWindow("{}hsvImage{}".format(p,s))
            cv2.destroyWindow("{}maskedLedImage{}".format(p,s))
            self._showallxfix = None
            return True
        else:
            return False





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
