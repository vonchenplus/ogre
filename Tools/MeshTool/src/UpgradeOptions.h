
#ifndef _OgreToolUpgradeOptions_H_
#define _OgreToolUpgradeOptions_H_

#include "OgreHardwareVertexBuffer.h"
#include "OgreMeshSerializer.h"
#include "OgreMesh2Serializer.h"

struct UpgradeOptions
{
    bool interactive;
    bool suppressEdgeLists;
    bool generateTangents;
    Ogre::VertexElementSemantic tangentSemantic;
    bool tangentUseParity;
    bool tangentSplitMirrored;
    bool tangentSplitRotated;
    bool dontReorganise;
    bool destColourFormatSet;
    Ogre::VertexElementType destColourFormat;
    bool srcColourFormatSet;
    Ogre::VertexElementType srcColourFormat;
    bool lodAutoconfigure;
    unsigned short numLods;
    Ogre::Real lodDist;
    Ogre::Real lodPercent;
    size_t lodFixed;
    bool usePercent;
    Ogre::Serializer::Endian endian;
    bool recalcBounds;
    Ogre::v1::MeshVersion targetVersion;
    Ogre::MeshVersion targetVersionV2;

    bool exportAsV2;

    bool optimizeBuffer;
    bool halfPos;
    bool halfTexCoords;
    bool qTangents;
    bool optimizeForShadowMapping;
};

extern UpgradeOptions opts;

#endif
