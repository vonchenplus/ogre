#ifndef __Tesselation_H__
#define __Tesselation_H__

#include "SdkSample.h"
#include "OgreImage.h"

using namespace Ogre;
using namespace OgreBites;

class _OgreSampleClassExport Sample_Tesselation : public SdkSample
{
 public:

    Sample_Tesselation()
    {
        mInfo["Title"] = "Tesselation";
        mInfo["Description"] = "Sample for tessellation support (Hull, Domain shaders)";
        mInfo["Thumbnail"] = "thumb_tesselation.png";
        mInfo["Category"] = "Unsorted";
        mInfo["Help"] = "Top Left: Multi-frame\nTop Right: Scrolling\nBottom Left: Rotation\nBottom Right: Scaling";
    }

    void testCapabilities(const RenderSystemCapabilities* caps)
    {
        if (!caps->hasCapability(RSC_VERTEX_PROGRAM) || !caps->hasCapability(RSC_FRAGMENT_PROGRAM))
        {
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Your graphics card does not support vertex and fragment"
                        " programs, so you cannot run this sample. Sorry!", "Sample_Tesselation::testCapabilities");
        }
        if (!caps->hasCapability(RSC_TESSELATION_HULL_PROGRAM) || !caps->hasCapability(RSC_TESSELATION_DOMAIN_PROGRAM))
        {
            OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Your graphics card does not support tesselation shaders. Sorry!",
                        "Sample_Tesselation:testCapabilities");
        }
        if (!GpuProgramManager::getSingleton().isSyntaxSupported("vs_5_0") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("hs_5_0") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("ds_5_0") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("ps_5_0") &&
            !GpuProgramManager::getSingleton().isSyntaxSupported("glsl"))
        {
            OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Your card does not support the shader model 5.0 needed for this sample, "
                        "so you cannot run this sample. Sorry!", "Sample_Tesselation::testCapabilities");
        }
    }

protected:

    void setupContent()
    {
        //JAJ lighting
        //FIXME
        /* mSceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5)); */
        /* Ogre::Light* l = mSceneMgr->createLight("MainLight"); */
        /* l->setType(Ogre::Light::LT_DIRECTIONAL); */
        /* //l->setPosition(20,80,50); */
        /* l->setDirection(Ogre::Vector3(0,-1,-1)); */
          
        // set our camera
        mTrayMgr->showCursor();
        mCameraMan->setStyle(CS_ORBIT);
        //mCameraMan->setStyle(CS_FREELOOK);
        Camera* cam = mCameraMan->getCamera();
        cam->setPosition(0, 5, -30);
        cam->setPolygonMode(PM_WIREFRAME);

        // create material and set the texture unit to our texture
        MaterialPtr tMat = MaterialManager::getSingleton().createOrRetrieve("Ogre/TesselationExample", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first.staticCast<Material>();
        tMat->compile();
        tMat->getBestTechnique()->getPass(0);

        // create a plain with float3 tex cord
        ManualObject* tObject = mSceneMgr->createManualObject("TesselatedObject");
        
        // create a triangle that uses our material 
        tObject->begin(tMat->getName(), RenderOperation::OT_TRIANGLE_LIST);
        //tObject->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);
        tObject->position(10, 10, 0);
        tObject->position(0, 10, 0);
        tObject->position(0, 0, 0);
        //tObject->triangle(0, 1, 2);
        tObject->end();

        // attach it to a node and position appropriately
        SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(tObject);


        ManualObject* tObject2 = mSceneMgr->createManualObject("TesselatedObject2");

        tObject2->begin(tMat->getName(), RenderOperation::OT_TRIANGLE_LIST);
        //tObject2->begin("BaseWhiteNoLighting", RenderOperation::OT_TRIANGLE_LIST);
        tObject2->position(20, 20, 10);
        tObject2->position(10, 20, 10);
        tObject2->position(10, 10, 10);
        tObject2->triangle(0, 1, 2);
        tObject2->end();

        // attach it to a node and position appropriately
        SceneNode* node2 = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node2->attachObject(tObject2);

        //JAJ ogre heads for debugging
        /* //FIXME */
        /* Entity *ogreEnt = mSceneMgr->createEntity("TessellatedHead", "ogrehead.mesh"); */
        /* ogreEnt->setMaterialName(tMat->getName()); */
        /* SceneNode* ogre = mSceneMgr->getRootSceneNode()->createChildSceneNode(); */
        /* //ogre->setPosition(0,0,0) */
        /* ogre->attachObject(ogreEnt); */

        /* Entity *ogreEnt2 = mSceneMgr->createEntity("PlainHead", "ogrehead.mesh"); */
        /* //ogreEnt2->setMaterialName("Examples/SphereMappedRustySteel"); */
        /* SceneNode* ogre2 = mSceneMgr->getRootSceneNode()->createChildSceneNode(); */
        /* ogre2->setPosition(0, 20, 30); */
        /* ogre2->attachObject(ogreEnt2); */

        printf("Cam position: %.2f, %.2f, %.2f", cam->getPosition().x, cam->getPosition().y, cam->getPosition().z);
    }
};

#endif
