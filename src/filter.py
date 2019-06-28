from __future__ import absolute_import

import math
import rospy
import rospkg
import tf2_ros as tf2
from tf.transformations import euler_from_quaternion
import numpy as np

class VisualCompassFilter:
    def __init__(self, config):
        self.config = config

        # TODO remove next lines
        rospack = rospkg.RosPack()
        self.package_path = rospack.get_path('bitbots_visual_compass')

        rospy.init_node('bitbots_visual_compass_setup')

        # TODO config file
        self.base_frame = 'base_footprint'
        self.camera_frame = 'camera_optical_frame'
        self.odom_frame = "odom"

        self.tf_buffer = tf2.Buffer(cache_time=rospy.Duration(120))
        self.listener = tf2.TransformListener(self.tf_buffer)

        # Filter constant
        self.K = 0.8

        self.threshold = 0.3

        self.filteredVector = None

        currentTimestamp = None

        lastTimestamp = None

    def _getYawOrientationInBasefootprintFrame(self, visualCompassAngle, header):
        # TODO stamp
        camera_yaw_angle = self._getYawFromTf(self.camera_frame, self.base_frame, header) + 0.5 * math.pi
        correctedAngle = (visualCompassAngle - camera_yaw_angle) % (2 * math.pi)
        return correctedAngle

    def _initFilter(self, vector):
        rospy.loginfo("Init filter")
        self.filteredVector = vector

    def setMeasurement(self, angle, confidence, header):

        limitedConfidence = self._cutAtThreshold(confidence)

        relativeYawAngle = self._getYawOrientationInBasefootprintFrame(angle, header)
        relativeYawVector = self._angleToVector(relativeYawAngle, limitedConfidence)

        self.currentTimestamp = header

        if self.filteredVector is None:
            self._initFilter(relativeYawVector)
        else:
            filteredVectorWithOdomOffset = self._odomOffset(self.filteredVector, self.lastTimestamp, self.currentTimestamp)

            self.filteredVector = self._runFilter(filteredVectorWithOdomOffset, relativeYawVector)

        # Set new timestamp
        self.lastTimestamp = self.currentTimestamp

        return math.degrees(self.getFilteredValue()[0]), self.getFilteredValue()[1]

    def _cutAtThreshold(self, confidence):
        if confidence <= self.threshold:
            confidence = 0
        return confidence

    def _odomOffset(self, vector, lastTimestamp, currentTimestamp):
        # reset filter if timestamps are too fare off.
        oldOrientation = self._getYawFromTf(self.odom_frame, self.base_frame, lastTimestamp)
        currentOrientation = self._getYawFromTf(self.odom_frame, self.base_frame, currentTimestamp)

        orientationDelta = (currentOrientation - oldOrientation)
        odomOffsetYawAngle = orientationDelta
        odomCorrectedVector = self._rotateVectorByAngle(vector, odomOffsetYawAngle)
        return odomCorrectedVector
    
    def _angleToVector(self, angle, confidence):
        return np.array([math.sin(angle) * confidence,
                         math.cos(angle) * confidence], dtype=np.float32)

    def _vectorToAngle(self, vector):
        confidence = math.sqrt(vector[0] ** 2 + vector[1] ** 2)
        angle = (math.atan2(vector[0], vector[1])) % (math.pi * 2)
        angle = angle % (math.pi * 2)
        return angle, confidence

    def _rotateVectorByAngle(self, vector, offsetAngle):
        angle, confidence = self._vectorToAngle(vector)
        angle = (angle + offsetAngle) % (math.pi * 2)
        return self._angleToVector(angle, confidence)

    def _runFilter(self, oldVectorWithOdom, currentVector):
        return oldVectorWithOdom * self.K + currentVector * (1 - self.K)

    def _getYawFromTf(self, frame1, frame2, stamp, timeout=0.5):
        orientation = self.tf_buffer.lookup_transform(frame1, frame2, stamp, timeout=rospy.Duration(timeout)).transform.rotation
        yaw_angle = (euler_from_quaternion((orientation.x, 
                                            orientation.y, 
                                            orientation.z, 
                                            orientation.w))[2]) % (2 * math.pi)
        return yaw_angle
    
    def getFilteredValue(self):
        now = rospy.Time.now()
        vector = self._odomOffset(self.filteredVector, self.currentTimestamp, now)
        return self._vectorToAngle(vector)
