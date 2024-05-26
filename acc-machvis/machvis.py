#!/usr/bin/env python3

import glob
import cv2
import numpy as np

DIGITS_LOOKUP = {
	(1, 1, 1, 0, 1, 1, 1): 0,
	(0, 0, 1, 0, 0, 1, 0): 1,
	(1, 0, 1, 1, 1, 1, 0): 2,
	(1, 0, 1, 1, 0, 1, 1): 3,
	(0, 1, 1, 1, 0, 1, 0): 4,
	(1, 1, 0, 1, 0, 1, 1): 5,
	(1, 1, 0, 1, 1, 1, 1): 6,
	(1, 0, 1, 0, 0, 1, 0): 7,
	(1, 1, 1, 1, 1, 1, 1): 8,
	(1, 1, 1, 1, 0, 1, 1): 9
}

# Feature,      Value,  Saturation,   Hue
# LED center    210     220           50
# LED corner    180     160           50
# Seg center    86      38            8
# Seg corner    87      70            164

imgpath = glob.glob('/home/ivan/Sources/ac-cloudifier/acc-machvis/imgs/**/*.jpg')
images = []
hsvimages = []
masks = []
maskedgreenimages=[]
markers = []

class AccImage:
    def __init__ (self, srcimg: cv2.Mat):
        srcrot = cv2.rotate(srcimg, cv2.ROTATE_90_CLOCKWISE)
        srchsv = cv2.cvtColor(srcrot, cv2.COLOR_BGR2HSV)

        # These masks are good for the LEDs, iffy for the 7 segment display
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



# Aruco
dict = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_5X5_250)
magicmarker = 105
detectorparams = cv2.aruco.DetectorParameters()
detector = cv2.aruco.ArucoDetector(dict, detectorparams)

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
