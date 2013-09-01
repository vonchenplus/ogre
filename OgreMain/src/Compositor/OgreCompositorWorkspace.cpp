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

#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorShadowNode.h"

#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"

#include "OgreSceneManager.h"
#include "OgreRenderTarget.h"
#include "OgreLogManager.h"

namespace Ogre
{
	CompositorWorkspace::CompositorWorkspace( IdType id, const CompositorWorkspaceDef *definition,
												RenderTarget *finalRenderTarget,
												SceneManager *sceneManager, Camera *defaultCam,
												RenderSystem *renderSys, bool bEnabled ) :
			IdObject( id ),
			mDefinition( definition ),
			mRenderWindow( finalRenderTarget ),
			mValid( false ),
			mEnabled( bEnabled ),
			mDefaultCamera( defaultCam ),
			mSceneManager( sceneManager ),
			mRenderSys( renderSys )
	{
		//Create global textures
		TextureDefinitionBase::createTextures( definition->mLocalTextureDefs, mGlobalTextures,
												id, true, mRenderWindow, mRenderSys );

		createAllNodes();
		connectAllNodes();
	}
	//-----------------------------------------------------------------------------------
	CompositorWorkspace::~CompositorWorkspace()
	{
		destroyAllNodes();

		//Destroy our global textures
		TextureDefinitionBase::destroyTextures( mDefinition->mLocalTextureDefs, mGlobalTextures,
												getId(), true, mRenderSys );
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::createAllNodes(void)
	{
		destroyAllNodes();

		CompositorWorkspaceDef::NodeAliasMap::const_iterator itor = mDefinition->mAliasedNodes.begin();
		CompositorWorkspaceDef::NodeAliasMap::const_iterator end  = mDefinition->mAliasedNodes.end();

		const CompositorManager2 *compoManager = mDefinition->mCompositorManager;

		while( itor != end )
		{
			const CompositorNodeDef *nodeDef = compoManager->getNodeDefinition( itor->second );
			CompositorNode *newNode = OGRE_NEW CompositorNode( Id::generateNewId<CompositorNode>(),
																itor->first, nodeDef, this, mRenderSys );
			mNodeSequence.push_back( newNode );
			++itor;
		}
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::destroyAllNodes(void)
	{
		mValid = false;
		CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
		CompositorNodeVec::const_iterator end  = mNodeSequence.end();

		while( itor != end )
			OGRE_DELETE *itor++;
		mNodeSequence.clear();
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::connectAllNodes(void)
	{
		{
			//First connect the RenderWindow, otherwise the node could end up not being processed
			CompositorNode *finalNode = findNode( mDefinition->mFinalNode );
			finalNode->connectFinalRT( mRenderWindow, CompositorChannel::TextureVec(),
										mDefinition->mFinalInChannel );
		}

		CompositorNodeVec unprocessedList( mNodeSequence.begin(), mNodeSequence.end() );
		CompositorNodeVec processedList;
		processedList.reserve( mNodeSequence.size() );

		bool noneProcessed = false;

		while( !unprocessedList.empty() && !noneProcessed )
		{
			noneProcessed = true;
			CompositorNodeVec::iterator itor = unprocessedList.begin();
			CompositorNodeVec::iterator end  = unprocessedList.end();

			while( itor != end )
			{
				CompositorNode *node = *itor;
				if( node->areAllInputsConnected() )
				{
					//This node has no missing dependency, we can process it!
					CompositorWorkspaceDef::ChannelRouteList::const_iterator itRoute =
																	mDefinition->mChannelRoutes.begin();
					CompositorWorkspaceDef::ChannelRouteList::const_iterator enRoute =
																	mDefinition->mChannelRoutes.end();

					//Connect all nodes according to our routing map. Perhaps I could've chosen a more
					//efficient representation for lookup. Oh well, it's not like there's a 1000 nodes
					//per workspace. If this is a hotspot, refactor.
					while( itRoute != enRoute )
					{
						if( itRoute->outNode == node->getName() )
						{
							node->connectTo( itRoute->outChannel, findNode( itRoute->inNode, true ),
											 itRoute->inChannel );
						}
						++itRoute;
					}

					//The processed list is now in order
					processedList.push_back( *itor );

					//Remove processed nodes from the list. We'll keep until there's no one left
					itor = efficientVectorRemove( unprocessedList, itor );
					end  = unprocessedList.end();

					noneProcessed = false;
				}
				else
				{
					++itor;
				}
			}
		}

		if( !unprocessedList.empty() )
		{
			CompositorNodeVec::const_iterator itor = unprocessedList.begin();
			CompositorNodeVec::const_iterator end  = unprocessedList.end();
			while( itor != end )
			{
				LogManager::getSingleton().logMessage(
					"WARNING: Node '" + (*itor)->getName().getFriendlyText() + "' has the following "
					"channels in a disconnected state. Workspace won't work until they're solved:" );

				const CompositorChannelVec& inputChannels = (*itor)->getInputChannel();
				CompositorChannelVec::const_iterator itChannels = inputChannels.begin();
				CompositorChannelVec::const_iterator enChannels = inputChannels.end();
				while( itChannels != enChannels )
				{
					if( !itChannels->isValid() )
					{
						const size_t channelIdx = itChannels - inputChannels.begin();
						LogManager::getSingleton().logMessage( "\t\t\t Channel # " +
											StringConverter::toString( channelIdx ) );
					}
					++itChannels;
				}
				
				++itor;
			}
		}
		else
		{
			// We need mNodeSequence in the right order of execution!
			mNodeSequence.clear();
			mNodeSequence.insert( mNodeSequence.end(), processedList.begin(), processedList.end() );

			CompositorNodeVec::iterator itor = mNodeSequence.begin();
			CompositorNodeVec::iterator end  = mNodeSequence.end();

			while( itor != end )
			{
				(*itor)->createPasses();
				++itor;
			}

			//Now manage automatic shadow nodes present PASS_SCENE passes
			//(when using SHADOW_NODE_FIRST_ONLY)
			setupPassesShadowNodes();

			mValid = true;
		}
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::setupPassesShadowNodes(void)
	{
		CompositorShadowNodeVec::iterator itShadowNode = mShadowNodes.begin();
		CompositorShadowNodeVec::iterator enShadowNode = mShadowNodes.end();

		while( itShadowNode != enShadowNode )
		{
			Camera *lastCamera = 0;
			CompositorShadowNode *shadowNode = *itShadowNode;

			CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
			CompositorNodeVec::const_iterator end  = mNodeSequence.end();
			while( itor != end )
			{
				const CompositorPassVec &passes = (*itor)->_getPasses();
				CompositorPassVec::const_iterator itPasses = passes.begin();
				CompositorPassVec::const_iterator enPasses = passes.end();

				while( itPasses != enPasses )
				{
					if( (*itPasses)->getType() == PASS_SCENE )
					{
						assert( dynamic_cast<CompositorPassScene*>( *itPasses ) );
						CompositorPassScene *pass = static_cast<CompositorPassScene*>( *itPasses );

						if( shadowNode == pass->getShadowNode() )
						{
							ShadowNodeRecalculation recalc =
													pass->getDefinition()->mShadowNodeRecalculation;

							if( recalc == SHADOW_NODE_RECALCULATE )
							{
								//We're forced to recalculate anyway, save the new camera
								lastCamera = pass->getCamera();
							}
							else if( recalc == SHADOW_NODE_FIRST_ONLY )
							{
								if( lastCamera != pass->getCamera() )
								{
									//Either this is the first one, or camera changed.
									//We need to recalculate
									pass->_setUpdateShadowNode( true );
									lastCamera = pass->getCamera();
								}
								else
								{
									pass->_setUpdateShadowNode( false );
								}
							}
						}
					}
					++itPasses;
				}
				++itor;
			}
			++itShadowNode;
		}
	}
	//-----------------------------------------------------------------------------------
	CompositorNode* CompositorWorkspace::findNode( IdString aliasName, bool includeShadowNodes ) const
	{
		CompositorNode *retVal = 0;
		CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
		CompositorNodeVec::const_iterator end  = mNodeSequence.end();

		while( itor != end && !retVal )
		{
			if( (*itor)->getName() == aliasName )
				retVal = *itor;
			++itor;
		}

		if( !retVal && includeShadowNodes )
		{
			bool shadowNodeCreated;
			retVal = getShadowNode( aliasName, shadowNodeCreated );
			assert( !shadowNodeCreated && "Shadow Node should be created by now, or referencing a shadow"
					" node that will never be used in a pass!" );
		}

		return retVal;
	}
	//-----------------------------------------------------------------------------------
	const CompositorChannel& CompositorWorkspace::getGlobalTexture( IdString name ) const
	{
		size_t index;
		TextureDefinitionBase::TextureSource textureSource;
		mDefinition->getTextureSource( name, index, textureSource );
		return mGlobalTextures[index];
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::revalidateAllNodes(void)
	{
		createAllNodes();
		connectAllNodes();
	}
	//-----------------------------------------------------------------------------------
	Camera* CompositorWorkspace::findCamera( IdString cameraName ) const 
	{
		return mSceneManager->findCamera( cameraName );
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::_update( bool swapFinalTargets, bool waitForVSync )
	{
		CompositorNodeVec::const_iterator itor = mNodeSequence.begin();
		CompositorNodeVec::const_iterator end  = mNodeSequence.end();

		while( itor != end )
		{
			CompositorNode *node = *itor;
			node->_update();
			++itor;
		}

		if( swapFinalTargets && mRenderWindow )
			mRenderWindow->swapBuffers( waitForVSync );
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspace::_swapFinalTarget( bool waitForVSync )
	{
		if( mRenderWindow )
			mRenderWindow->swapBuffers( waitForVSync );
	}
	//-----------------------------------------------------------------------------------
	CompositorShadowNode* CompositorWorkspace::getShadowNode( IdString nodeDefName, bool &bCreated ) const
	{
		CompositorShadowNode *retVal = 0;
		bCreated = false;

		CompositorShadowNodeVec::const_iterator itor = mShadowNodes.begin();
		CompositorShadowNodeVec::const_iterator end  = mShadowNodes.end();

		while( itor != end && !retVal )
		{
			if( (*itor)->getName() == nodeDefName )
				retVal = *itor;
			++itor;
		}

		if( !retVal )
		{
			//Not found, create one.
			const CompositorManager2 *compoManager = mDefinition->mCompositorManager;
			const CompositorShadowNodeDef *def = compoManager->getShadowNodeDefinition( nodeDefName );
			retVal = OGRE_NEW CompositorShadowNode( Id::generateNewId<CompositorNode>(),
													def, this, mRenderSys );
			bCreated = true;
		}

		return retVal;
	}
}
