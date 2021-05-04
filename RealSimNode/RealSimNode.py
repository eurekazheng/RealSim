# RealSimNode.py
# Client maya node

import sys
from subprocess import Popen, PIPE
import os

import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx
import maya.cmds as cmds

import numpy as np

obj = ''

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
    # inNPnt = OpenMaya.MObject()
    outPnt = OpenMaya.MObject()
    # viscosity = OpenMaya.MObject()
    # boxSize = OpenMaya.MObject()
    # k = OpenMaya.MObject()
    # liquidDensity = OpenMaya.MObject()
    # mass = OpenMaya.MObject()

    # constructor
    def __init__(self):
        OpenMayaMPx.MPxNode.__init__(self)

        root = 'D:/studies/CIS-660/Authoring Tool/RealSim'

        os.environ['PYTHONHOME'] = '%s/mesh2pc/python' % root
        os.environ['PYTHONPATH'] = '%s/mesh2pc/python' % root

        cmd = '%s/RealSimCL/Debug/RealSimCL.exe' % root
        p1 = Popen(['%s/mesh2pc/python/python.exe' % root,
                    '%s/mesh2pc/mesh2pc.py' % root,
                    root, obj.split(':')[0]],
                   stdin=PIPE, stdout=PIPE, stderr=PIPE)
        p2 = Popen([cmd], stdin=p1.stdout, stdout=PIPE)
        output, err = p2.communicate()

        output = [line.split(", ") for line in output.splitlines()]
        self.y_scale = float(output[0][0])
        scale = self.y_scale / 50

        OpenMaya.MGlobal.displayInfo(obj)
        cmds.scale(scale, scale, scale, obj)

        self.pos = np.asarray(output[1:], dtype=np.float).reshape(800, 4096, 3)

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
            # inNPnt_ = data.inputValue(RealSimNode.inNPnt).asInt()

            pointsData = data.outputValue(RealSimNode.outPnt)
            pointsAAD = OpenMaya.MFnArrayAttrsData()
            pointsObject = pointsAAD.create()
            positionArr = pointsAAD.vectorArray("position")
            idArr = pointsAAD.doubleArray("id")

            for i in range(self.pos.shape[1]):
                positionArr.append(OpenMaya.MVector(*self.pos[time_ * 6][i]) / 50)
                idArr.append(i)

            pointsData.setMObject(pointsObject)
        data.setClean(plug)


# initializer
def nodeInitializer():
    # TODO:: initialize the input and output attributes. Be sure to use the 
    #         MAKE_INPUT and MAKE_OUTPUT functions.
    unit = OpenMaya.MFnUnitAttribute()
    typed = OpenMaya.MFnTypedAttribute()
    # numeric = OpenMaya.MFnNumericAttribute()

    RealSimNode.time = unit.create("time", "t", OpenMaya.MFnUnitAttribute.kTime, 0.0)
    MAKE_INPUT(unit)

    # RealSimNode.inNPnt = numeric.create("numPoints", "n", OpenMaya.MFnNumericData.kInt, 4096)
    # MAKE_INPUT(numeric)
    #
    # RealSimNode.viscosity = numeric.create("viscosity", "v", OpenMaya.MFnNumericData.kDouble, 2)
    # MAKE_INPUT(numeric)
    #
    # RealSimNode.boxSize = numeric.create("box_size", "b", OpenMaya.MFnNumericData.kDouble, 20)
    # MAKE_INPUT(numeric)
    #
    # RealSimNode.k = numeric.create("k_constant", "k", OpenMaya.MFnNumericData.kDouble, 1)
    # MAKE_INPUT(numeric)
    #
    # RealSimNode.liquidDensity = numeric.create("liquid_density", "d", OpenMaya.MFnNumericData.kDouble, 20)
    # MAKE_INPUT(numeric)
    #
    # RealSimNode.mass = numeric.create("mass", "m", OpenMaya.MFnNumericData.kDouble, 0.1)
    # MAKE_INPUT(numeric)
    #
    # defaultObj = OpenMaya.MFnStringData().create('baymax:RightArm1')
    # RealSimNode.obj = typed.create("obj", "o", OpenMaya.MFnData.kString, defaultObj)
    # MAKE_INPUT(numeric)

    RealSimNode.outPnt = typed.create("outPnt", "op", OpenMaya.MFnArrayAttrsData.kDynArrayAttrs)
    MAKE_OUTPUT(typed)

    try:
        # TODO:: add the attributes to the node and set up the
        #         attributeAffects (addAttribute, and attributeAffects)
        RealSimNode.addAttribute(RealSimNode.outPnt)

        RealSimNode.addAttribute(RealSimNode.time)
        RealSimNode.attributeAffects(RealSimNode.time, RealSimNode.outPnt)

        # RealSimNode.addAttribute(RealSimNode.inNPnt)
        # RealSimNode.attributeAffects(RealSimNode.inNPnt, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.viscosity)
        # RealSimNode.attributeAffects(RealSimNode.viscosity, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.boxSize)
        # RealSimNode.attributeAffects(RealSimNode.boxSize, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.k)
        # RealSimNode.attributeAffects(RealSimNode.k, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.liquidDensity)
        # RealSimNode.attributeAffects(RealSimNode.liquidDensity, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.mass)
        # RealSimNode.attributeAffects(RealSimNode.mass, RealSimNode.outPnt)
        #
        # RealSimNode.addAttribute(RealSimNode.obj)
        # RealSimNode.attributeAffects(RealSimNode.obj, RealSimNode.outPnt)
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

    def applyMaterial(node):
        if cmds.objExists(node):
            shd = cmds.shadingNode('lambert', name="%s_lambert" % node, asShader=True)
            shdSG = cmds.sets(name='%sSG' % shd, empty=True, renderable=True, noSurfaceShader=True)
            cmds.connectAttr('%s.outColor' % shd, '%s.surfaceShader' % shdSG)
            cmds.sets(node, e=True, forceElement=shdSG)
            cmds.setAttr('%s.color' % shd, 0, 0.129, 1)

    def optionRealSimNode(*args):
        global obj
        obj = cmds.ls(sl=True)[0]

        pSphere = cmds.polySphere(radius=0.02)
        applyMaterial(pSphere[0])

        instancer = cmds.instancer()
        realSimNode = cmds.createNode('RealSimNode')

        cmds.connectAttr(pSphere[0] + '.matrix', instancer + '.inputHierarchy[0]')
        cmds.connectAttr(realSimNode + '.outPnt', instancer + '.inputPoints')
        cmds.connectAttr('time1.outTime', realSimNode + '.time')
        # cmds.setAttr(realSimNode + '.obj', obj)

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
