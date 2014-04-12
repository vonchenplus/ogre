/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd
Also see acknowledgements in Readme.html

You may use this sample code for anything you like, it is not covered by the
same license as the rest of the engine.
-----------------------------------------------------------------------------
*/

/**
    @file 
        Shadows.cpp
    @brief
        Shows a few ways to use Ogre's shadowing techniques
*/

#include "SamplePlugin.h"
#include "Test.h"

#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/OgreCompositorPassDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"
#include "Compositor/OgreCompositorShadowNode.h"

using namespace Ogre;
using namespace OgreBites;

Sample_Test::Sample_Test()
    : mFloorPlane( 0 )
    , mMainLight( 0 )
    , mMinFlareSize( 40 )
    , mMaxFlareSize( 80 )
    , mPssm( true )
{
    mInfo["Title"] = "Shadows v2";
    mInfo["Description"] =
            "Shows how to setup a shadow scene using depth-based shadow mapping.\n"
            "Shadow mapping involves setting up custom shaders and a proper compositor.\n\n"
            "This sample supports 8 different lights. Only the first one is a directional "
            "light and shadow caster. The rest of the lights are point lights.\n\n"
            "By default this sample uses PSSM technique which gives the best quality. To "
            "test the Focused technique, change the mPssm variable and recompile.\n\n"
            "Note in this CTP (Community Technology Preview) only directional shadow caster"
            " lights have been thoroughly tested. Point and Spot casters should work with "
            "propper shader tweaks, but this hasn't been tested yet.\n\n"
            "Relevant Media files:\n"
            "   * Examples_Shadows.material\n"
            "   * Examples_Shadows.program\n"
            "   * Example_Shadows.compositor\n"
            "OpenGL\n"
            "   * Example_ShadowsCasterFp.glsl\n"
            "   * Example_ShadowsDebugViewFp.glsl\n"
            "   * Example_ShadowsFp.glsl\n"
            "   * Example_ShadowsVp.glsl\n"
            "DX9\n"
            "   * Example_Shadows_ps.hlsl\n"
            "   * Example_Shadows_vs.hlsl\n"
            "   * Example_ShadowsCaster_ps.hlsl\n"
            "   * Example_ShadowsDebugView_ps.hlsl";
    mInfo["Thumbnail"] = "thumb_shadows.png";
    mInfo["Category"] = "API Usage";
}
//---------------------------------------------------------------------------------------
CompositorWorkspace* Sample_Test::setupCompositor(void)
{
    CompositorManager2 *compositorManager = mRoot->getCompositorManager2();
    return compositorManager->addWorkspace( mSceneMgr, mWindow, mCamera, "TESTWorkspace", true );
}
//---------------------------------------------------------------------------------------
void Sample_Test::setupContent(void)
{
    Light *light = mSceneMgr->createLight();
    mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject( light );
    light->setType( Light::LT_DIRECTIONAL );
    light->setDirection( Vector3( -0.1f, -1.0f, -1.0f ).normalisedCopy() );
    //light->setDirection( Vector3::NEGATIVE_UNIT_Y );
    light->setSpecularColour( ColourValue::White );
    //light->setDiffuseColour( ColourValue::Black );
    //light->setSpecularColour( ColourValue::Black );

    srand( 101 );
    for( size_t i=0; i<4; ++i )
    {
        light = light = mSceneMgr->createLight();
        SceneNode *lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        lightNode->attachObject( light );
        light->setName( "Spot " + StringConverter::toString(i) );
        light->setType( Light::LT_SPOTLIGHT );
        light->setAttenuation( 1000.0f, 1.0f, 0.0f, 0.0f );
        lightNode->setPosition( (rand() / (float)RAND_MAX) * 300.0f - 150.0f,
                                40,
                                (rand() / (float)RAND_MAX) * 300.0f - 150.0f );
        light->setDirection( (-lightNode->getPosition()).normalisedCopy() );
        //light->setDirection( Vector3( -0.1f, -1.0f, -1.0f ).normalisedCopy() );
        //light->setDirection( Vector3::NEGATIVE_UNIT_Y );
        //lightNode->setPosition( -lightNode->getOrientation().zAxis() * 80.0f );
        light->setSpotlightOuterAngle( Degree( 60.0f ) );
        mCreatedLights.push_back( light );
        light->setDiffuseColour( ColourValue::White );
        light->setSpecularColour( ColourValue::White );
        //light->setDiffuseColour( ColourValue::Black );
        //light->setSpecularColour( ColourValue::Black );
    }

    SceneNode *sceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mEntity = mSceneMgr->createEntity( "penguin.mesh" );
    mEntity->setMaterialName( "TEST" );
    mEntity->setName( "Penguin" );
    sceneNode->attachObject( mEntity );
    //sceneNode->scale( 0.1f, 0.1f, 0.1f );

    mCamera->setPosition( 0, 10, 60.0f );
    mCamera->lookAt( 0, 10, 0 );

    mSceneMgr->setShadowDirectionalLightExtrusionDistance( 200.0f );
    mSceneMgr->setShadowFarDistance( 200.0f );
    mCamera->setNearClipDistance( 0.1f );
    mCamera->setFarClipDistance( 5000.0f );
    mCamera->setAutoAspectRatio( true );

    // create a mesh for our ground
    MeshManager::getSingleton().createPlane( "ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Plane(Vector3::UNIT_Y, -20), 1000, 1000, 1, 1, true, 1, 6, 6, Vector3::UNIT_Z );

    mFloorPlane = mSceneMgr->createEntity( "ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            SCENE_STATIC );
    mFloorPlane->setMaterialName( "TEST" );
    mFloorPlane->setCastShadows( false );
    mSceneMgr->getRootSceneNode( SCENE_STATIC )->attachObject( mFloorPlane );
    /*mSceneMgr->setAmbientLight( ColourValue( 0.1f, 0.1f, 0.1f ) );

    SceneNode *lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mMainLight = mSceneMgr->createLight();
    mMainLight->setName( "Sun" );
    lightNode->attachObject( mMainLight );
    mMainLight->setType( Light::LT_DIRECTIONAL );
    mMainLight->setDirection( Vector3( -0.1f, -1.0f, -1.25f ).normalisedCopy() );
    //mMainLight->setDirection( Vector3::NEGATIVE_UNIT_Y );

    // create a mesh for our ground
    MeshManager::getSingleton().createPlane( "ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Plane(Vector3::UNIT_Y, -20), 1000, 1000, 1, 1, true, 1, 6, 6, Vector3::UNIT_Z );

    mFloorPlane = mSceneMgr->createEntity( "ground", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                            SCENE_STATIC );
    mFloorPlane->setMaterialName( mPssm ? "Example_Shadows_Floor_pssm" : "Example_Shadows_Floor" );
    mFloorPlane->setCastShadows( true );
    mSceneMgr->getRootSceneNode( SCENE_STATIC )->attachObject( mFloorPlane );

    Entity *penguin = mSceneMgr->createEntity( "penguin.mesh" );
    SceneNode *node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject( penguin );
    penguin->setName( "Penguin" );
    penguin->setMaterialName( mPssm ? "Example_Shadows_Penguin_pssm" :"Example_Shadows_Penguin" );
    mCasters.push_back( penguin );

    mCamera->setPosition( 0, 20, 50.0f );
    mCamera->lookAt( 0, 20, 0 );

    //Shadow quality is very sensible to these settings. The defaults are insane
    //(covers a huge distance) which is often not needed. Using more conservative
    //values greatly improves quality
    mSceneMgr->setShadowDirectionalLightExtrusionDistance( 200.0f );
    mSceneMgr->setShadowFarDistance( 200.0f );
    mCamera->setNearClipDistance( 0.1f );
    mCamera->setFarClipDistance( 5000.0f );

    createExtraLights();*/
    createDebugOverlays();
}
//---------------------------------------------------------------------------------------
void Sample_Test::createExtraLights(void)
{
    srand( 7907 ); //Prime number. For debugging, we want all runs to be deterministic
    rand();

    mLightRootNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

    //Create 7 more lights
    for( size_t i=0; i<7; ++i )
    {
        SceneNode *lightNode = mLightRootNode->createChildSceneNode();
        Light *light = mSceneMgr->createLight();
        light->setName( "Extra Point Light" );
        lightNode->attachObject( light );
        light->setType( Light::LT_POINT );

        light->setAttenuation( 1000.0f, 1.0f, 0.0f, 1.0f );

        light->setDiffuseColour( (((unsigned int)rand()) % 255) / 255.0f * 0.25f,
                                 (((unsigned int)rand()) % 255) / 255.0f * 0.25f,
                                 (((unsigned int)rand()) % 255) / 255.0f * 0.25f );
        lightNode->setPosition( ( (rand() / (Real)RAND_MAX) * 2.0f - 1.0f ) * 60.0f,
                                ( (rand() / (Real)RAND_MAX) * 2.0f - 1.0f ) * 10.0f,
                                ( (rand() / (Real)RAND_MAX) * 2.0f - 1.0f ) * 60.0f );
    }
}
//---------------------------------------------------------------------------------------
void Sample_Test::createDebugOverlays(void)
{
    ////////////////////////////////////////////////////////////
    // BEGIN DEBUG
    IdString shadowNodeName = "TEST_ShadowNode";

    Ogre::MaterialPtr baseWhite = Ogre::MaterialManager::getSingletonPtr()->
                                                            getByName("Example_Shadows_DebugView");
    Ogre::MaterialPtr DepthShadowTexture = baseWhite->clone("DepthShadowTexture0");
    Ogre::TextureUnitState* textureUnit = DepthShadowTexture->getTechnique(0)->getPass(0)->
                                                            getTextureUnitState(0);
    CompositorShadowNode *shadowNode = mWorkspace->findShadowNode( shadowNodeName );
    Ogre::TexturePtr tex = shadowNode->getLocalTextures()[3].textures[0];
    textureUnit->setTextureName( tex->getName() );

    DepthShadowTexture = baseWhite->clone("DepthShadowTexture1");
    textureUnit = DepthShadowTexture->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    tex = shadowNode->getLocalTextures()[4].textures[0];
    textureUnit->setTextureName(tex->getName());

    DepthShadowTexture = baseWhite->clone("DepthShadowTexture2");
    textureUnit = DepthShadowTexture->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    tex = shadowNode->getLocalTextures()[5].textures[0];
    textureUnit->setTextureName(tex->getName());

    DepthShadowTexture = baseWhite->clone("DepthShadowTexture3");
    textureUnit = DepthShadowTexture->getTechnique(0)->getPass(0)->getTextureUnitState(0);
    tex = shadowNode->getLocalTextures()[6].textures[0];
    textureUnit->setTextureName(tex->getName());

    Ogre::OverlayManager& overlayManager = Ogre::OverlayManager::getSingleton();
    // Create an overlay
    Overlay *debugOverlay = overlayManager.create( "OverlayName" );

    // Create a panel
    Ogre::OverlayContainer* panel = static_cast<Ogre::OverlayContainer*>(
        overlayManager.createOverlayElement("Panel", "PanelName0"));
    panel->setMetricsMode(Ogre::GMM_PIXELS);
    panel->setPosition(10, 10);
    panel->setDimensions(100, 100);
    panel->setMaterialName("DepthShadowTexture0");
    debugOverlay->add2D(panel);

    //if( mPssm )
    {
        panel = static_cast<Ogre::OverlayContainer*>(
            overlayManager.createOverlayElement("Panel", "PanelName1"));
        panel->setMetricsMode(Ogre::GMM_PIXELS);
        panel->setPosition(120, 10);
        panel->setDimensions(100, 100);
        panel->setMaterialName("DepthShadowTexture1");
        debugOverlay->add2D(panel);
        panel = static_cast<Ogre::OverlayContainer*>(
            overlayManager.createOverlayElement("Panel", "PanelName2"));
        panel->setMetricsMode(Ogre::GMM_PIXELS);
        panel->setPosition(230, 10);
        panel->setDimensions(100, 100);
        panel->setMaterialName("DepthShadowTexture2");
        debugOverlay->add2D(panel);
        panel = static_cast<Ogre::OverlayContainer*>(
            overlayManager.createOverlayElement("Panel", "PanelName3"));
        panel->setMetricsMode(Ogre::GMM_PIXELS);
        panel->setPosition(340, 10);
        panel->setDimensions(100, 100);
        panel->setMaterialName("DepthShadowTexture3");
        debugOverlay->add2D(panel);
    }

    debugOverlay->show();

    // END DEBUG
    ////////////////////////////////////////////////////////////
}
//---------------------------------------------------------------------------------------
bool Sample_Test::frameRenderingQueued(const FrameEvent& evt)
{
    /*mLightRootNode->yaw( Radian( evt.timeSinceLastFrame ) );

    Node *mainLightNode = mMainLight->getParentNode();
    mainLightNode->yaw( Radian( -evt.timeSinceLastFrame * 0.1f ), Node::TS_PARENT );*/

    CompositorShadowNode *shadowNode = mWorkspace->findShadowNode( "TEST_ShadowNode" );

    //HACK: To get the lights in the same order they'll be sent to the GPU
    const LightClosestArray &lightList = shadowNode->getShadowCastingLights();

    const MaterialPtr &mat = mEntity->getSubEntity(0)->getMaterial();
    //GpuProgramParametersSharedPtr vsParams = mat->getBestTechnique()->getPass(0)->getVertexProgramParameters();
    //vsParams->setNamedConstant(  );
    GpuProgramParametersSharedPtr psParams = mat->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
    Vector2 invShadowMapSize[7];
    for( size_t i=0; i<7; ++i )
        invShadowMapSize[i] = Vector2( 1.0f / 1024.0f, 1.0f / 1024.0f );
    invShadowMapSize[0] = Vector2( 1.0f / 2048.0f, 1.0f / 2048.0f );
    psParams->setNamedConstant( "invShadowMapSize", (Real*)invShadowMapSize, 7, 2 );

    Matrix3 viewMat;
    mCamera->getViewMatrix().extract3x3Matrix( viewMat );

    Vector3 tmp3[4];
    for( size_t i=0; i<4; ++i )
        tmp3[i] = viewMat * mCreatedLights[lightList[i+1].globalIndex-1]->getDirection();
    psParams->setNamedConstant( "spotDirection", (Real*)tmp3, 4, 3 );
    for( size_t i=0; i<4; ++i )
    {
        size_t idx = lightList[i+1].globalIndex-1;
        tmp3[i].x = 1.0f / ( cosf( mCreatedLights[idx]->getSpotlightInnerAngle().valueRadians() * 0.5f ) -
                             cosf( mCreatedLights[idx]->getSpotlightOuterAngle().valueRadians() * 0.5f ) );
        tmp3[i].y = cosf( mCreatedLights[idx]->getSpotlightOuterAngle().valueRadians() * 0.5f );
        tmp3[i].z = mCreatedLights[idx]->getSpotlightFalloff();
    }
    psParams->setNamedConstant( "spotParams", (Real*)tmp3, 4, 3 );
    for( size_t i=0; i<4; ++i )
    {
        size_t idx = lightList[i+1].globalIndex-1;
        tmp3[i].x = mCreatedLights[idx]->getAttenuationRange();
        tmp3[i].y = mCreatedLights[idx]->getAttenuationLinear();
        tmp3[i].z = mCreatedLights[idx]->getAttenuationQuadric();
    }
    psParams->setNamedConstant( "attenuation", (Real*)tmp3, 4, 3 );

    return SdkSample::frameRenderingQueued(evt);  // don't forget the parent class updates!
}
