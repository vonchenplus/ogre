/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "OgreStableHeaders.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreRectangle2D.h"
#include "OgreTextureManager.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
    template<typename T>
    void deleteAllSecondClear( T& container )
    {
        //Delete all workspace definitions
        typename T::const_iterator itor = container.begin();
        typename T::const_iterator end  = container.end();
        while( itor != end )
        {
            OGRE_DELETE itor->second;
            ++itor;
        }
        container.clear();
    }
    template<typename T>
    void deleteAllClear( T& container )
    {
        //Delete all workspace definitions
        typename T::const_iterator itor = container.begin();
        typename T::const_iterator end  = container.end();
        while( itor != end )
            OGRE_DELETE *itor++;
        container.clear();
    }

    CompositorManager2::CompositorManager2( RenderSystem *renderSystem ) :
        mFrameCount( 0 ),
        mRenderSystem( renderSystem ),
        mSharedTriangleFS( 0 ),
        mSharedQuadFS( 0 ),
        mCompositorPassProvider( 0 )
    {
        mSharedTriangleFS   = OGRE_NEW Rectangle2D( false );
        mSharedQuadFS       = OGRE_NEW Rectangle2D( true );

        //----------------------------------------------------------------
        // Create a default Node & Workspace for basic rendering:
        //      * Clears the screen
        //      * Renders all objects from RQ 0 to Max.
        //      * No shadows
        //----------------------------------------------------------------
        CompositorNodeDef *nodeDef = this->addNodeDefinition( "Default Node RenderScene" );

        //Input texture
        nodeDef->addTextureSourceName( "WindowRT", 0, TextureDefinitionBase::TEXTURE_INPUT );

        nodeDef->setNumTargetPass( 1 );
        {
            CompositorTargetDef *targetDef = nodeDef->addTargetPass( "WindowRT" );
            targetDef->setNumPasses( 2 );
            {
                {
                    CompositorPassClearDef *passClear = static_cast<CompositorPassClearDef*>( targetDef->addPass( PASS_CLEAR ) );
                    passClear->mColourValue = ColourValue( 0.6f, 0.0f, 0.6f );
                }
                {
                    CompositorPassQuadDef *passQuad = static_cast<CompositorPassQuadDef*>( targetDef->addPass( PASS_QUAD ) );
                    passQuad->mMaterialName = "MyQuadTest";
                }
                CompositorPassSceneDef *passScene = static_cast<CompositorPassSceneDef*>( targetDef->addPass( PASS_SCENE ) );

                passScene->mShadowNode = "Default Shadow Node";
            }
        }

        //-------
        /*CompositorShadowNodeDef *shadowNode = this->addShadowNodeDefinition( "Default Shadow Node" );
        shadowNode->setNumShadowTextureDefinitions( 3 );
        
        {
            ShadowTextureDefinition *texDef = shadowNode->addShadowTextureDefinition( 0, 0, "MyFirstTex", false );
            texDef->width   = 2048;
            texDef->height  = 2048;
            texDef->formatList.push_back( PF_FLOAT32_R );
            //texDef->formatList.push_back( PF_A8B8G8R8 );
            texDef->shadowMapTechnique = SHADOWMAP_FOCUSED;
        }
        {
            ShadowTextureDefinition *texDef = shadowNode->addShadowTextureDefinition( 1, 0, "MyFirstTex2", false );
            texDef->width   = 1024;
            texDef->height  = 1024;
            texDef->formatList.push_back( PF_FLOAT32_R );
            //texDef->formatList.push_back( PF_A8B8G8R8 );
            texDef->shadowMapTechnique = SHADOWMAP_FOCUSED;
        }
        {
            ShadowTextureDefinition *texDef = shadowNode->addShadowTextureDefinition( 2, 0, "MyFirstTex3", false );
            texDef->width   = 1024;
            texDef->height  = 1024;
            texDef->formatList.push_back( PF_FLOAT32_R );
            //texDef->formatList.push_back( PF_A8B8G8R8 );
            texDef->shadowMapTechnique = SHADOWMAP_FOCUSED;
        }

        shadowNode->setNumTargetPass( 3 );
        {
            CompositorTargetDef *targetDef = shadowNode->addTargetPass( "MyFirstTex" );
            targetDef->setNumPasses( 2 );
            {
                CompositorPassDef *passDef = targetDef->addPass( PASS_CLEAR );
                ((CompositorPassClearDef*)(passDef))->mColourValue = ColourValue::White;
                //CompositorPassDef *passDef = targetDef->addPass( PASS_SCENE );
                passDef = targetDef->addPass( PASS_SCENE );
                passDef->mShadowMapIdx = 0;
                passDef->mIncludeOverlays = false;
            }
            targetDef = shadowNode->addTargetPass( "MyFirstTex2" );
            targetDef->setNumPasses( 2 );
            {
                CompositorPassDef *passDef = targetDef->addPass( PASS_CLEAR );
                ((CompositorPassClearDef*)(passDef))->mColourValue = ColourValue::White;
                //CompositorPassDef *passDef = targetDef->addPass( PASS_SCENE );
                passDef = targetDef->addPass( PASS_SCENE );
                passDef->mShadowMapIdx = 1;
                passDef->mIncludeOverlays = false;
            }
            targetDef = shadowNode->addTargetPass( "MyFirstTex3" );
            targetDef->setNumPasses( 2 );
            {
                CompositorPassDef *passDef = targetDef->addPass( PASS_CLEAR );
                ((CompositorPassClearDef*)(passDef))->mColourValue = ColourValue::White;
                //CompositorPassDef *passDef = targetDef->addPass( PASS_SCENE );
                passDef = targetDef->addPass( PASS_SCENE );
                passDef->mShadowMapIdx = 2;
                passDef->mIncludeOverlays = false;
            }
        }*/

        validateAllNodes();
    }
    //-----------------------------------------------------------------------------------
    CompositorManager2::~CompositorManager2()
    {
        for( TextureVec::iterator i = mNullTextureList.begin(); i != mNullTextureList.end(); ++i )
            TextureManager::getSingleton().remove( (*i)->getHandle() );

        removeAllWorkspaces();

        removeAllWorkspaceDefinitions();
        removeAllShadowNodeDefinitions();
        removeAllNodeDefinitions();

        OGRE_DELETE mSharedTriangleFS;
        mSharedTriangleFS = 0;
        OGRE_DELETE mSharedQuadFS;
        mSharedQuadFS = 0;
    }
    //-----------------------------------------------------------------------------------
    bool CompositorManager2::hasNodeDefinition( IdString nodeDefName ) const
    {
        return mNodeDefinitions.find( nodeDefName ) != mNodeDefinitions.end();
    }
    //-----------------------------------------------------------------------------------
    CompositorNodeDef* CompositorManager2::getNodeDefinitionNonConst( IdString nodeDefName ) const
    {
        CompositorNodeDef *retVal = 0;

        CompositorNodeDefMap::const_iterator itor = mNodeDefinitions.find( nodeDefName );
        if( itor == mNodeDefinitions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Node definition with name '" +
                            nodeDefName.getFriendlyText() + "' not found",
                            "CompositorManager2::getNodeDefinitionNonConst" );
        }
        else
        {
            retVal = itor->second;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const CompositorNodeDef* CompositorManager2::getNodeDefinition( IdString nodeDefName ) const
    {
        CompositorNodeDef const *retVal = 0;
        
        CompositorNodeDefMap::const_iterator itor = mNodeDefinitions.find( nodeDefName );
        if( itor == mNodeDefinitions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Node definition with name '" +
                            nodeDefName.getFriendlyText() + "' not found",
                            "CompositorManager2::getNodeDefinition" );
        }
        else
        {
            retVal = itor->second;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorNodeDef* CompositorManager2::addNodeDefinition( const String &name )
    {
        CompositorNodeDef *retVal = 0;

        if( mNodeDefinitions.find( name ) == mNodeDefinitions.end() )
        {
            retVal = OGRE_NEW CompositorNodeDef( name, this );
            mNodeDefinitions[name] = retVal;
        }
        else
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "A node definition with name '" +
                         name + "' already exists", "CompositorManager2::addNodeDefinition" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const CompositorShadowNodeDef* CompositorManager2::getShadowNodeDefinition(
                                                                    IdString nodeDefName ) const
    {
        CompositorShadowNodeDef const *retVal = 0;
        
        CompositorShadowNodeDefMap::const_iterator itor = mShadowNodeDefs.find( nodeDefName );
        if( itor == mShadowNodeDefs.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "ShadowNode definition with name '" +
                         nodeDefName.getFriendlyText() + "' not found",
                         "CompositorManager2::getShadowNodeDefinition" );
        }
        else
        {
            retVal = itor->second;

            if( !retVal )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE, "ShadowNode definition with name '" +
                            nodeDefName.getFriendlyText() + "' was found but not validated.\n"
                            "Did you call validateAllObjects?",
                            "CompositorManager2::getShadowNodeDefinition" );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorShadowNodeDef* CompositorManager2::addShadowNodeDefinition( const String &name )
    {
        CompositorShadowNodeDef *retVal = 0;

        if( mShadowNodeDefs.find( name ) == mShadowNodeDefs.end() )
        {
            retVal = OGRE_NEW CompositorShadowNodeDef( name, this );
            mShadowNodeDefs[name] = 0; //Fill with a null ptr, it will later be validated
            mUnfinishedShadowNodes.push_back( retVal );
        }
        else
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "A shadow node definition with name '" +
                         name + "' already exists", "CompositorManager2::addShadowNodeDefinition" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspaceDef* CompositorManager2::addWorkspaceDefinition( IdString name )
    {
        CompositorWorkspaceDef *retVal = 0;

        if( mWorkspaceDefs.find( name ) == mWorkspaceDefs.end() )
        {
            retVal = OGRE_NEW CompositorWorkspaceDef( name, this );
            mWorkspaceDefs[name] = retVal;
        }
        else
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "A workspace with name '" +
                            name.getFriendlyText() + "' already exists",
                            "CompositorManager2::addWorkspaceDefinition" );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    bool CompositorManager2::hasWorkspaceDefinition( IdString name ) const
    {
        return mWorkspaceDefs.find( name ) != mWorkspaceDefs.end();
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspaceDef* CompositorManager2::getWorkspaceDefinition( IdString name ) const
    {
        CompositorWorkspaceDefMap::const_iterator itor = mWorkspaceDefs.find( name );
        if( itor == mWorkspaceDefs.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Workspace definition with name '" +
                            name.getFriendlyText() + "' not found",
                            "CompositorManager2::getWorkspaceDefinition" );
        }

        return itor->second;
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspace* CompositorManager2::addWorkspace( SceneManager *sceneManager,
                                             RenderTarget *finalRenderTarget, Camera *defaultCam,
                                             IdString definitionName, bool bEnabled, int position )
    {
        CompositorChannel channel;
        channel.target = finalRenderTarget;
        return addWorkspace( sceneManager, channel, defaultCam, definitionName, bEnabled, position );
    }
    //-----------------------------------------------------------------------------------
    CompositorWorkspace* CompositorManager2::addWorkspace( SceneManager *sceneManager,
                                             const CompositorChannel &finalRenderTarget,
                                             Camera *defaultCam, IdString definitionName,
                                             bool bEnabled, int position )
    {
        validateAllNodes();

        CompositorWorkspace *workspace = 0;

        CompositorWorkspaceDefMap::const_iterator itor = mWorkspaceDefs.find( definitionName );
        if( itor == mWorkspaceDefs.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Workspace definition '" +
                            definitionName.getFriendlyText() + "' not found",
                            "CompositorManager2::addWorkspace" );
        }
        else
        {
            workspace = OGRE_NEW CompositorWorkspace(
                                Id::generateNewId<CompositorWorkspace>(), itor->second,
                                finalRenderTarget, sceneManager, defaultCam, mRenderSystem, bEnabled );

            mQueuedWorkspaces.push_back( QueuedWorkspace( workspace, position ) );
        }

        return workspace;
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::addQueuedWorkspaces(void)
    {
        QueuedWorkspaceVec::const_iterator itor = mQueuedWorkspaces.begin();
        QueuedWorkspaceVec::const_iterator end  = mQueuedWorkspaces.end();

        while( itor != end )
        {
            int position = std::min<int>( itor->position, mWorkspaces.size() );

            if( position < 0 )
                mWorkspaces.push_back( itor->workspace );
            else
                mWorkspaces.insert( mWorkspaces.begin() + position, itor->workspace );
            ++itor;
        }

        mQueuedWorkspaces.clear();
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::removeWorkspace( CompositorWorkspace *workspace )
    {
        WorkspaceVec::iterator itor = std::find( mWorkspaces.begin(), mWorkspaces.end(), workspace );
        if( itor == mWorkspaces.end() )
        {
            QueuedWorkspaceVec::iterator it = mQueuedWorkspaces.begin();
            QueuedWorkspaceVec::iterator en = mQueuedWorkspaces.end();

            while( it != en && it->workspace != workspace )
                ++it;

            if( it == mQueuedWorkspaces.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Workspace not created with this "
                             "Compositor Manager", "CompositorManager2::removeWorkspace" );
            }
            else
            {
                OGRE_DELETE it->workspace;
                mQueuedWorkspaces.erase( it ); //Preserve the order of workspace execution
            }
        }
        else
        {
            OGRE_DELETE *itor;
            mWorkspaces.erase( itor ); //Preserve the order of workspace execution
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::removeAllWorkspaces(void)
    {
        addQueuedWorkspaces();
        deleteAllClear( mWorkspaces );
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::removeAllWorkspaceDefinitions(void)
    {
        deleteAllSecondClear( mWorkspaceDefs );
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::removeAllShadowNodeDefinitions(void)
    {
        deleteAllClear( mUnfinishedShadowNodes );
        deleteAllSecondClear( mShadowNodeDefs );
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::removeAllNodeDefinitions(void)
    {
        deleteAllSecondClear( mNodeDefinitions );
    }
    //-----------------------------------------------------------------------------------
    TexturePtr CompositorManager2::getNullShadowTexture( PixelFormat format )
    {
        for (TextureVec::iterator t = mNullTextureList.begin(); t != mNullTextureList.end(); ++t)
        {
            const TexturePtr& tex = *t;

            if (format == tex->getFormat())
            {
                // Ok, a match
                return tex;
            }
        }

        // not found, create a new one
        // A 1x1 texture of the correct format, not a render target
        static const String baseName = "Ogre/ShadowTextureNull";
        String targName = baseName + StringConverter::toString( mNullTextureList.size() );
        TexturePtr shadowTex = TextureManager::getSingleton().createManual(
            targName, ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME, 
            TEX_TYPE_2D, 1, 1, 0, format, TU_STATIC_WRITE_ONLY );
        mNullTextureList.push_back( shadowTex );

        // lock & populate the texture based on format
        shadowTex->getBuffer()->lock(HardwareBuffer::HBL_DISCARD);
        const PixelBox& box = shadowTex->getBuffer()->getCurrentLock();

        // set high-values across all bytes of the format 
        PixelUtil::packColour( 1.0f, 1.0f, 1.0f, 1.0f, format, box.data );

        shadowTex->getBuffer()->unlock();

        return shadowTex;
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::validateAllNodes()
    {
        CompositorShadowNodeDefVec::iterator itor = mUnfinishedShadowNodes.begin();
        CompositorShadowNodeDefVec::iterator end  = mUnfinishedShadowNodes.end();

        while( itor != end )
        {
            (*itor)->_validateAndFinish();
            mShadowNodeDefs[(*itor)->getName()] = *itor;
            //Nullify in case next iterator's call throws: we would free previous pointers twice
            *itor = 0;
            ++itor;
        }

        mUnfinishedShadowNodes.clear();
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::_update(void)
    {
        addQueuedWorkspaces();

        WorkspaceVec::const_iterator itor = mWorkspaces.begin();
        WorkspaceVec::const_iterator end  = mWorkspaces.end();

        // We need to validate the device (D3D9) before calling _beginFrame()
        while( itor != end )
        {
            CompositorWorkspace *workspace = (*itor);
            if( workspace->getEnabled() )
                workspace->_validateFinalTarget();
            ++itor;
        }

        itor = mWorkspaces.begin();

        while( itor != end )
        {
            CompositorWorkspace *workspace = (*itor);
            if( workspace->getEnabled() )
            {
                if( workspace->isValid() )
                {
                    workspace->_beginUpdate( false );
                }
                else
                {
                    //TODO: We may end up recreating this every frame for invalid workspaces
                    workspace->recreateAllNodes();
                    if( workspace->isValid() )
                        workspace->_beginUpdate( false );
                }
            }
            ++itor;
        }

        //The actual update
        itor = mWorkspaces.begin();

        while( itor != end )
        {
            CompositorWorkspace *workspace = (*itor);
            if( workspace->getEnabled() && workspace->isValid() )
                    workspace->_update();
            ++itor;
        }

        itor = mWorkspaces.begin();

        while( itor != end )
        {
            CompositorWorkspace *workspace = (*itor);
            if( workspace->getEnabled() && workspace->isValid() )
                    workspace->_endUpdate( false );
            ++itor;
        }

        ++mFrameCount;
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::_swapAllFinalTargets(void)
    {
        WorkspaceVec::const_iterator itor = mWorkspaces.begin();
        WorkspaceVec::const_iterator end  = mWorkspaces.end();

        vector<RenderTarget*>::type swappedTargets;
        swappedTargets.reserve( mWorkspaces.size() );

        while( itor != end )
        {
            CompositorWorkspace *workspace = (*itor);

            RenderTarget *finalTarget = workspace->getFinalTarget();
            bool alreadySwapped = std::find( swappedTargets.begin(),
                                             swappedTargets.end(), finalTarget ) != swappedTargets.end();

            if( workspace->getEnabled() && workspace->isValid() && !alreadySwapped )
            {
                workspace->_swapFinalTarget();
                swappedTargets.push_back( finalTarget );
            }

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::createBasicWorkspaceDef( const IdString &workspaceDefName,
                                                        const ColourValue &backgroundColour,
                                                        IdString shadowNodeName )
    {
        CompositorNodeDef *nodeDef = this->addNodeDefinition( "AutoGen " + (workspaceDefName +
                                                                IdString("/Node")).getReleaseText() );

        //Input texture
        nodeDef->addTextureSourceName( "WindowRT", 0, TextureDefinitionBase::TEXTURE_INPUT );

        nodeDef->setNumTargetPass( 1 );
        {
            CompositorTargetDef *targetDef = nodeDef->addTargetPass( "WindowRT" );
            targetDef->setNumPasses( 2 );
            {
                {
                    CompositorPassClearDef *passClear = static_cast<CompositorPassClearDef*>
                                                            ( targetDef->addPass( PASS_CLEAR ) );
                    passClear->mColourValue = backgroundColour;
                }
                {
                    CompositorPassSceneDef *passScene = static_cast<CompositorPassSceneDef*>
                                                                ( targetDef->addPass( PASS_SCENE ) );
                    passScene->mShadowNode = shadowNodeName;
                }
            }
        }

        CompositorWorkspaceDef *workDef = this->addWorkspaceDefinition( workspaceDefName );
        workDef->connectOutput( nodeDef->getName(), 0 );
    }
    //-----------------------------------------------------------------------------------
    void CompositorManager2::setCompositorPassProvider( CompositorPassProvider *passProvider )
    {
        mCompositorPassProvider = passProvider;
    }
    //-----------------------------------------------------------------------------------
    CompositorPassProvider* CompositorManager2::getCompositorPassProvider(void) const
    {
        return mCompositorPassProvider;
    }
}
