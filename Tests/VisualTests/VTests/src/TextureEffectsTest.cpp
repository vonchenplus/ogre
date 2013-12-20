/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2013 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "TextureEffectsTest.h"

TextureEffectsTest::TextureEffectsTest()
{
    mInfo["Title"] = "VTests_TextureEffects";
    mInfo["Description"] = "Tests texture effects.";
    addScreenshotFrame(50);
}
//---------------------------------------------------------------------------

void TextureEffectsTest::setupContent()
{
    mViewport->setBackgroundColour(ColourValue(0.8,0.8,0.8));

    // the names of the four materials we will use
    String matNames[] = {"Examples/OgreDance", "Examples/OgreParade", "Examples/OgreSpin", "Examples/OgreWobble"};

    for (unsigned int i = 0; i < 4; i++)
    {
        // create a standard plane entity
        Entity* ent = mSceneMgr->createEntity("Plane" + StringConverter::toString(i + 1), SceneManager::PT_PLANE);

        // attach it to a node, scale it, and position appropriately
        SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node->setPosition(i % 2 ? 25 : -25, i / 2 ? -25 : 25, 0);
        node->setScale(0.25, 0.25, 0.25);
        node->attachObject(ent);

        ent->setMaterialName(matNames[i]);  // give it the material we prepared
    }

    mCamera->setPosition(0,0,125);
    mCamera->setDirection(0,0,-1);
}
//-----------------------------------------------------------------------


