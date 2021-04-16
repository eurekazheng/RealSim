# RealSimNode.py
#   Produces random locations to be used with the Maya instancer node.
import sys
import random
import subprocess

import maya.OpenMaya as OpenMaya
import maya.OpenMayaAnim as OpenMayaAnim
import maya.OpenMayaMPx as OpenMayaMPx
import maya.cmds as cmds
import maya.mel as mel

import numpy as np


# Useful functions for declaring attributes as inputs or outputs.
def MAKE_INPUT(attr):
    attr.setKeyable(True)
    attr.setStorable(True)
    attr.setReadable(True)
    attr.setWritable(True)


def MAKE_OUTPUT(attr):
    attr.setKeyable(False)
    attr.setStorable(False)
    attr.setReadable(True)
    attr.setWritable(False)


# Define the name of the node
kPluginNodeTypeName = "RealSimNode"

# Give the node a unique ID. Make sure this ID is different from all of your
# other nodes!
RealSimNodeId = OpenMaya.MTypeId(0x8704)


# Node definition
class RealSimNode(OpenMayaMPx.MPxNode):
    # Declare class variables:
    # TODO:: declare the input and output class variables
    time = OpenMaya.MObject()
    inNPnt = OpenMaya.MObject()
    outPnt = OpenMaya.MObject()
    minX = OpenMaya.MObject()
    minY = OpenMaya.MObject()
    minZ = OpenMaya.MObject()
    maxX = OpenMaya.MObject()
    maxY = OpenMaya.MObject()
    maxZ = OpenMaya.MObject()
    minV = OpenMaya.MObject()
    maxV = OpenMaya.MObject()

    initialized = False
    initPos = OpenMaya.MVectorArray()
    initTime = OpenMaya.MFloatArray()

    # constructor
    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)
        cmd = "D:\studies\CIS-660\Authoring Tool\RealSim\RealSimCL\Debug\RealSimCL.exe"
        output = subprocess.Popen([cmd], stdout=subprocess.PIPE).communicate()[0]

        output = [line.split(", ") for line in output.splitlines()]
        self.pos = np.asarray(output, dtype=np.float).reshape(800, 512, 3)
        OpenMaya.MGlobal.displayInfo(self.pos.shape)

    # compute
    def compute(self, plug, data):
        # TODO:: create the main functionality of the node. Your node should 
        #         take in three floats for max position (X,Y,Z), three floats 
        #         for min position (X,Y,Z), and the number of random points to
        #         be generated. Your node should output an MFnArrayAttrsData 
        #         object containing the random points. Consult the homework
        #         sheet for how to deal with creating the MFnArrayAttrsData.

        if plug == RealSimNode.outPnt:
            time_ = data.inputValue(RealSimNode.time).asTime().value()
            inNPnt_ = data.inputValue(RealSimNode.inNPnt).asInt()
            minV_ = data.inputValue(RealSimNode.minV).asDouble3()   
            maxV_ = data.inputValue(RealSimNode.maxV).asDouble3()

            OpenMaya.MGlobal.displayInfo(time_)

            pointsData = data.outputValue(RealSimNode.outPnt)
            pointsAAD = OpenMaya.MFnArrayAttrsData()
            pointsObject = pointsAAD.create()
            positionArr = pointsAAD.vectorArray("position")
            idArr = pointsAAD.doubleArray("id")
            
            for i in range(inNPnt_):
                positionArr.append(OpenMaya.MVector(*self.pos[time_ * 6][i] / 50))
                idArr.append(i)
            
            pointsData.setMObject(pointsObject)
        data.setClean(plug)


# initializer
def nodeInitializer():
    # TODO:: initialize the input and output attributes. Be sure to use the 
    #         MAKE_INPUT and MAKE_OUTPUT functions.
    unit = OpenMaya.MFnUnitAttribute()
    typed = OpenMaya.MFnTypedAttribute()
    numeric = OpenMaya.MFnNumericAttribute()

    RealSimNode.time = unit.create("time", "t", OpenMaya.MFnUnitAttribute.kTime, 0.0)
    MAKE_INPUT(unit)

    RealSimNode.inNPnt = numeric.create("numPoints", "n", OpenMaya.MFnNumericData.kInt, 512)
    MAKE_INPUT(numeric)

    RealSimNode.minX = numeric.create("minX", "miX", OpenMaya.MFnNumericData.kDouble, 0)
    MAKE_INPUT(numeric)

    RealSimNode.maxX = numeric.create("maxX", "maX", OpenMaya.MFnNumericData.kDouble, 1)
    MAKE_INPUT(numeric)
    
    RealSimNode.minY = numeric.create("minY", "miY", OpenMaya.MFnNumericData.kDouble, 3)
    MAKE_INPUT(numeric)
    
    RealSimNode.maxY = numeric.create("maxY", "maY", OpenMaya.MFnNumericData.kDouble, 3)
    MAKE_INPUT(numeric)
    
    RealSimNode.minZ = numeric.create("minZ", "miZ", OpenMaya.MFnNumericData.kDouble, 0)
    MAKE_INPUT(numeric)
    
    RealSimNode.maxZ = numeric.create("maxZ", "maZ", OpenMaya.MFnNumericData.kDouble, 1)
    MAKE_INPUT(numeric)

    RealSimNode.minV = numeric.create("minV", "miV", RealSimNode.minX, RealSimNode.minY, RealSimNode.minZ)
    MAKE_INPUT(numeric)
    
    RealSimNode.maxV = numeric.create("maxV", "maV", RealSimNode.maxX, RealSimNode.maxY, RealSimNode.maxZ)
    MAKE_INPUT(numeric)

    RealSimNode.outPnt = typed.create("outPnt", "op", OpenMaya.MFnArrayAttrsData.kDynArrayAttrs)
    MAKE_OUTPUT(typed)

    try:
        # TODO:: add the attributes to the node and set up the
        #         attributeAffects (addAttribute, and attributeAffects)
        RealSimNode.addAttribute(RealSimNode.outPnt)

        RealSimNode.addAttribute(RealSimNode.time)
        RealSimNode.attributeAffects(RealSimNode.time, RealSimNode.outPnt)

        RealSimNode.addAttribute(RealSimNode.inNPnt)
        RealSimNode.attributeAffects(RealSimNode.inNPnt, RealSimNode.outPnt)

        RealSimNode.addAttribute(RealSimNode.minV)
        RealSimNode.attributeAffects(RealSimNode.minV, RealSimNode.outPnt)

        RealSimNode.addAttribute(RealSimNode.maxV)
        RealSimNode.attributeAffects(RealSimNode.maxV, RealSimNode.outPnt)
    except:
        sys.stderr.write(("Failed to create attributes of %s node\n", kPluginNodeTypeName))


# creator
def nodeCreator():
    return OpenMayaMPx.asMPxPtr(RealSimNode())


# initialize the script plug-in
def initializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        mplugin.registerNode(kPluginNodeTypeName, RealSimNodeId, nodeCreator, nodeInitializer)
    except:
        sys.stderr.write("Failed to register node: %s\n" % kPluginNodeTypeName)

    def optionRealSimNode(*args):
        pSphere = cmds.polySphere(radius=0.05)
        instancer = cmds.instancer()
        realSimNode = cmds.createNode('RealSimNode')

        cmds.connectAttr(pSphere[0] + '.matrix', instancer + '.inputHierarchy[0]')
        cmds.connectAttr(realSimNode + '.outPnt', instancer + '.inputPoints')
        cmds.connectAttr('time1.outTime', realSimNode + '.time')

    cmds.menu('RealSimNodeMenu', parent='MayaWindow', label='RealSim', tearOff=True)
    cmds.menuItem(label='RealSim', command=optionRealSimNode)


# uninitialize the script plug-in
def uninitializePlugin(mobject):
    mplugin = OpenMayaMPx.MFnPlugin(mobject)
    try:
        mplugin.deregisterNode(RealSimNodeId)
    except:
        sys.stderr.write("Failed to unregister node: %s\n" % kPluginNodeTypeName)

    cmds.deleteUI('RealSimNodeMenu')
