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
grayimages = []
greenimages = []
hsvimages = []
masks = []
maskedgreenimages=[]

# These masks are good for the LEDs, iffy for the 7 segment display
lower_nhue = np.array([85,0,0])             # negated hue
upper_nhue = np.array([180,255,255])        # negated hue
lower_brightness = np.array([0,30,70])       # allowed brightness
upper_brightness = np.array([255,255,255])   # allowed brightness

for img in imgpath:
    n = cv2.imread(img)
    n = cv2.rotate(n, cv2.ROTATE_90_CLOCKWISE)

    grayn = cv2.cvtColor(n, cv2.COLOR_BGR2GRAY)
    greenn = n[:,:,1]
    hsvn = cv2.cvtColor(n, cv2.COLOR_BGR2HSV)

    images.append(n)
    grayimages.append(grayn)
    greenimages.append(greenn)
    hsvimages.append(hsvn)

for hsv in hsvimages:
    hue_m = cv2.bitwise_not(cv2.inRange(hsv, lower_nhue, upper_nhue))
    sat_m = cv2.inRange(hsv,lower_brightness, upper_brightness)
    m = cv2.bitwise_and(hue_m, sat_m)
    masks.append(m)

for img, mask in zip(images,masks):
    #mask = cv2.bitwise_not(mask)
    r = cv2.bitwise_and(img, img, mask=mask)
    maskedgreenimages.append(r)


cv2.imshow("Image 0", images[0])
cv2.imshow("Image 0 gray", grayimages[0])
cv2.imshow("Image 0 green", greenimages[0])
cv2.imshow("Image 0 HSL", hsvimages[0])
cv2.imshow("Image 0 mask", masks[0])
cv2.imshow("Image 0 masked", maskedgreenimages[0])

cv2.waitKey()

# Now do a slideshow
cv2.destroyAllWindows()
for img, himg, gimg, n in zip(images, hsvimages, maskedgreenimages, enumerate(images, start=0)):
    cv2.imshow("Image {n}", img)
    cv2.imshow("Image {n} HSL", himg)
    cv2.imshow("Image {n} masked", gimg)
    cv2.waitKey()
    cv2.destroyWindow("Image {n}")
    cv2.destroyWindow("Image {n} HSL")
    cv2.destroyWindow("Image {n} masked")

