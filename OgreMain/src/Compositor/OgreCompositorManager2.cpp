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

#include "OgreStableHeaders.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"

#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

namespace Ogre
{
	template<typename T>
	typename void deleteAllSecondClear( T& container )
	{
		//Delete all workspace definitions
		T::const_iterator itor = container.begin();
		T::const_iterator end  = container.end();
		while( itor != end )
		{
			OGRE_DELETE itor->second;
			++itor;
		}
		container.clear();
	}
	template<typename T>
	typename void deleteAllClear( T& container )
	{
		//Delete all workspace definitions
		T::const_iterator itor = container.begin();
		T::const_iterator end  = container.end();
		while( itor != end )
			OGRE_DELETE *itor++;
		container.clear();
	}

	CompositorManager2::CompositorManager2() :
		mFrameCount( 0 ),
		mRenderSystem( 0 )
	{
		//----------------------------------------------------------------
		// Create a default Node & Workspace for basic rendering:
		//		* Clears the screen
		//		* Renders all objects from RQ 0 to Max.
		//		* No shadows
		//----------------------------------------------------------------
		CompositorNodeDef *nodeDef = this->addNodeDefinition( "Default Node RenderScene" );

		//Input texture
		nodeDef->addTextureSourceName( "WindowRT", 0, TextureDefinitionBase::TEXTURE_INPUT );

		nodeDef->setNumTargetPass( 1 );
		{
			CompositorTargetDef *targetDef = nodeDef->addTargetPass( "WindowRT" );
			targetDef->setNumPasses( 2 );
			{
				targetDef->addPass( PASS_CLEAR );
				CompositorPassSceneDef *passScene = static_cast<CompositorPassSceneDef*>( targetDef->addPass( PASS_SCENE ) );

				passScene->mShadowNode = "Default Shadow Node";
			}
		}

		CompositorWorkspaceDef *workDef = this->addWorkspaceDefinition( "Default RenderScene" );
		workDef->connectOutput( 0, "Default Node RenderScene" );

		//-------
		CompositorShadowNodeDef *shadowNode = this->addShadowNodeDefinition( "Default Shadow Node" );
		shadowNode->setNumShadowTextureDefinitions( 1 );
		CompositorShadowNodeDef::ShadowTextureDefinition *texDef = shadowNode->addShadowTextureDefinition( 0, 0, "MyFirstTex", false );
		texDef->width	= 1024;
		texDef->height	= 1024;
		texDef->formatList.push_back( PF_FLOAT32_R );

		shadowNode->setNumTargetPass( 1 );
		{
			CompositorTargetDef *targetDef = shadowNode->addTargetPass( "MyFirstTex" );
			targetDef->setNumPasses( 2 );
			{
				targetDef->addPass( PASS_CLEAR );
				CompositorPassDef *passDef = targetDef->addPass( PASS_SCENE );
				passDef->mShadowMapIdx = 0;
				passDef->mIncludeOverlays = false;
			}
		}

		validateAllNodes();
	}
	//-----------------------------------------------------------------------------------
	CompositorManager2::~CompositorManager2()
	{
		deleteAllClear( mUnfinishedShadowNodes );
		deleteAllSecondClear( mWorkspaceDefs );
		deleteAllSecondClear( mNodeDefinitions );
		deleteAllSecondClear( mShadowNodeDefs );
		deleteAllClear( mWorkspaces );
	}
	//-----------------------------------------------------------------------------------
	bool CompositorManager2::hasNodeDefinition( IdString nodeDefName ) const
	{
		return mNodeDefinitions.find( nodeDefName ) != mNodeDefinitions.end();
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
	CompositorNodeDef* CompositorManager2::addNodeDefinition( IdString name )
	{
		CompositorNodeDef *retVal = 0;

		if( mNodeDefinitions.find( name ) == mNodeDefinitions.end() )
		{
			retVal = OGRE_NEW CompositorNodeDef( name );
			mNodeDefinitions[name] = retVal;
		}
		else
		{
			OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "A node definition with name '" +
							name.getFriendlyText() + "' already exists",
							"CompositorManager2::addNodeDefinition" );
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
	CompositorShadowNodeDef* CompositorManager2::addShadowNodeDefinition( IdString name )
	{
		CompositorShadowNodeDef *retVal = 0;

		if( mShadowNodeDefs.find( name ) == mShadowNodeDefs.end() )
		{
			retVal = OGRE_NEW CompositorShadowNodeDef( name );
			mShadowNodeDefs[name] = 0; //Fill with a null ptr, it will later be validated
			mUnfinishedShadowNodes.push_back( retVal );
		}
		else
		{
			OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM, "A shadow node definition with name '" +
							name.getFriendlyText() + "' already exists",
							"CompositorManager2::addShadowNodeDefinition" );
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
	CompositorWorkspace* CompositorManager2::addWorkspace( SceneManager *sceneManager,
											 RenderTarget *finalRenderTarget, Camera *defaultCam,
											 IdString definitionName, bool bEnabled )
	{
		CompositorWorkspaceDefMap::const_iterator itor = mWorkspaceDefs.find( definitionName );
		if( itor == mWorkspaceDefs.end() )
		{
			OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Workspace definition '" +
							definitionName.getFriendlyText() + "' not found",
							"CompositorManager2::addWorkspace" );
		}
		else
		{
			CompositorWorkspace* workspace = OGRE_NEW CompositorWorkspace(
								Id::generateNewId<CompositorWorkspace>(), itor->second,
								finalRenderTarget, sceneManager, defaultCam, mRenderSystem, bEnabled );
			mWorkspaces.push_back( workspace );
		}

		return mWorkspaces.back();
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
	void CompositorManager2::_update( bool swapFinalTargets, bool waitForVSync )
	{
		WorkspaceVec::const_iterator itor = mWorkspaces.begin();
		WorkspaceVec::const_iterator end  = mWorkspaces.end();

		while( itor != end )
		{
			CompositorWorkspace *workspace = (*itor);
			if( workspace->getEnabled() )
			{
				if( workspace->isValid() )
				{
					workspace->_update( swapFinalTargets, waitForVSync );
				}
				else
				{
					//TODO: We may end up recreating this every frame for invalid workspaces
					workspace->revalidateAllNodes();
					if( workspace->isValid() )
						workspace->_update( swapFinalTargets, waitForVSync );
				}
			}
			++itor;
		}

		++mFrameCount;
	}
	//-----------------------------------------------------------------------------------
	void CompositorManager2::_swapAllFinalTargets( bool waitForVSync )
	{
		WorkspaceVec::const_iterator itor = mWorkspaces.begin();
		WorkspaceVec::const_iterator end  = mWorkspaces.end();

		while( itor != end )
		{
			CompositorWorkspace *workspace = (*itor);
			if( workspace->getEnabled() && workspace->isValid() )
				workspace->_swapFinalTarget( waitForVSync );

			++itor;
		}
	}
}
