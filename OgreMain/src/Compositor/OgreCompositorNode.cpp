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

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorChannel.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassScene.h"
#include "Compositor/OgreCompositorWorkspace.h"

#include "OgreRenderTexture.h"
#include "OgreHardwarePixelBuffer.h"
#include "OgreRenderSystem.h"

namespace Ogre
{
	CompositorNode::CompositorNode( IdType id, IdString name, const CompositorNodeDef *definition,
									const CompositorWorkspace *workspace, RenderSystem *renderSys ) :
			IdObject( id ),
			mName( name ),
			mNumConnectedInputs( 0 ),
			mWorkspace( workspace ),
			mRenderSystem( renderSys ),
			mDefinition( definition )
	{
	}
	//-----------------------------------------------------------------------------------
	CompositorNode::CompositorNode( IdType id, IdString name, const CompositorNodeDef *definition,
									const CompositorWorkspace *workspace, RenderSystem *renderSys,
									const RenderTarget *finalTarget ) :
			IdObject( id ),
			mName( name ),
			mNumConnectedInputs( 0 ),
			mWorkspace( workspace ),
			mRenderSystem( renderSys ),
			mDefinition( definition )
	{
		bool defaultHwGamma		= false;
		uint defaultFsaa		= 0;
		String defaultFsaaHint	= StringUtil::BLANK;
		if( finalTarget )
		{
			// Inherit settings from target
			defaultHwGamma	= finalTarget->isHardwareGammaEnabled();
			defaultFsaa		= finalTarget->getFSAA();
			defaultFsaaHint	= finalTarget->getFSAAHint();
		}

		//Create the local textures
		CompositorNodeDef::TextureDefinitionVec::const_iterator itor =
																definition->mLocalTextureDefs.begin();
		CompositorNodeDef::TextureDefinitionVec::const_iterator end  =
																definition->mLocalTextureDefs.end();

		while( itor != end )
		{
			CompositorChannel newChannel;

			//If undefined, use main target's hw gamma settings, else use explicit setting
			bool hwGamma	= itor->hwGammaWrite == CompositorNodeDef::TextureDefinition::Undefined ?
								defaultHwGamma :
								itor->hwGammaWrite == CompositorNodeDef::TextureDefinition::True;
			//If true, use main target's fsaa settings, else disable
			uint fsaa		= itor->fsaa ? defaultFsaa : 0;
			const String &fsaaHint = itor->fsaa ? defaultFsaaHint : StringUtil::BLANK;

			String textureName = (itor->name + IdString( id )).getFriendlyText();
			if( itor->formatList.size() == 1 )
			{
				//Normal RT
				TexturePtr tex = TextureManager::getSingleton().createManual( textureName,
												ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
												TEX_TYPE_2D, itor->width, itor->height, 0,
												itor->formatList[0], TU_RENDERTARGET, 0, hwGamma,
												fsaa, fsaaHint );
				RenderTexture* rt = tex->getBuffer()->getRenderTarget();
				rt->setAutoUpdated(false);
				newChannel.target = rt;
				newChannel.textures.push_back( tex );
			}
			else
			{
				//MRT
				MultiRenderTarget* mrt = mRenderSystem->createMultiRenderTarget( textureName );
				PixelFormatList::const_iterator pixIt = itor->formatList.begin();
				PixelFormatList::const_iterator pixEn = itor->formatList.end();

				newChannel.target = mrt;

				while( pixIt != pixEn )
				{
					size_t rtNum = pixIt - itor->formatList.begin();
					TexturePtr tex = TextureManager::getSingleton().createManual(
												textureName + StringConverter::toString( rtNum ),
												ResourceGroupManager::INTERNAL_RESOURCE_GROUP_NAME,
												TEX_TYPE_2D, itor->width, itor->height, 0,
												*pixIt, TU_RENDERTARGET, 0, hwGamma,
												fsaa, fsaaHint );
					RenderTexture* rt = tex->getBuffer()->getRenderTarget();
					rt->setAutoUpdated(false);
					mrt->bindSurface( rtNum, rt );
					newChannel.textures.push_back( tex );
					++pixIt;
				}
			}

			mLocalTextures.push_back( newChannel );

			++itor;
		}
	}
	//-----------------------------------------------------------------------------------
	CompositorNode::~CompositorNode()
	{
		//Don't leave dangling pointers
		disconnectOutput();

		//Destroy our local textures
		CompositorNodeDef::TextureDefinitionVec::const_iterator itor =
																mDefinition->mLocalTextureDefs.begin();
		CompositorNodeDef::TextureDefinitionVec::const_iterator end  =
																mDefinition->mLocalTextureDefs.end();

		while( itor != end )
		{
			String textureName = (itor->name + IdString( getId() )).getFriendlyText();
			if( itor->formatList.size() == 1 )
			{
				//Normal RT. We don't hold any reference to, so just deregister from TextureManager
				TextureManager::getSingleton().remove( textureName );
			}
			else
			{
				//MRT. We need to destroy both the MultiRenderTarget ptr AND the textures
				mRenderSystem->destroyRenderTarget( textureName );
				for( size_t i=0; i<itor->formatList.size(); ++i )
					TextureManager::getSingleton().remove( textureName + StringConverter::toString(i) );
			}

			++itor;
		}

		mLocalTextures.clear();
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::routeOutputs()
	{
		assert( mNumConnectedInputs <= mInTextures.size() );

		CompositorChannelVec::iterator itor = mOutTextures.begin();
		CompositorChannelVec::iterator end  = mOutTextures.end();
		CompositorChannelVec::iterator begin= mOutTextures.begin();

		while( itor != end )
		{
			size_t index;
			TextureDefinitionBase::TextureSource textureSource;
			mDefinition->getTextureSource( itor - begin, index, textureSource );

			assert( textureSource == TextureDefinitionBase::TEXTURE_LOCAL ||
					textureSource == TextureDefinitionBase::TEXTURE_INPUT );

			if( textureSource == TextureDefinitionBase::TEXTURE_LOCAL )
				*itor = mLocalTextures[index];
			else
				*itor = mInTextures[index];
			++itor;
		}
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::disconnectOutput()
	{
		CompositorNodeVec::const_iterator itor = mConnectedNodes.begin();
		CompositorNodeVec::const_iterator end  = mConnectedNodes.end();

		while( itor != end )
		{
			CompositorChannelVec::const_iterator texIt = mLocalTextures.begin();
			CompositorChannelVec::const_iterator texEn = mLocalTextures.end();

			while( texIt != texEn )
				(*itor)->notifyDestroyed( *texIt++ );

			++itor;
		}

		mConnectedNodes.clear();
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::notifyDestroyed( const CompositorChannel &channel )
	{
		//Clear out outputs
		CompositorChannelVec::iterator texIt = mInTextures.begin();
		CompositorChannelVec::iterator texEn = mInTextures.end();

		//We can't early out, it's possible to assign the same output to two different
		//input channels (though it would work very unintuitively...)
		while( texIt != texEn )
		{
			if( *texIt == channel )
			{
				*texIt = CompositorChannel();
				--mNumConnectedInputs;
			}
			++texIt;
		}

		//Clear out outputs
		bool bFoundOuts = false;
		texIt = mOutTextures.begin();
		texEn = mOutTextures.end();

		while( texIt != texEn )
		{
			if( *texIt == channel )
			{
				bFoundOuts = true;
				*texIt = CompositorChannel();
				--mNumConnectedInputs;
			}
			++texIt;
		}

		if( bFoundOuts )
		{
			//Our attachees may have that texture too.
			CompositorNodeVec::const_iterator itor = mConnectedNodes.begin();
			CompositorNodeVec::const_iterator end  = mConnectedNodes.end();

			while( itor != end )
			{
				(*itor)->notifyDestroyed( channel );
				++itor;
			}
		}

		CompositorPassVec::const_iterator passIt = mPasses.begin();
		CompositorPassVec::const_iterator passEn = mPasses.end();
		while( passIt != passEn )
		{
			(*passIt)->notifyDestroyed( channel );
			++passIt;
		}
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::connectTo( size_t outChannelA, CompositorNode *nodeB, size_t inChannelB )
	{
		//Nodes must be connected in the right order (and after routeOutputs was called)
		//to avoid passing null pointers (which is probably not what we wanted)
		assert( this->mOutTextures[outChannelA].isValid() &&
				"Compositor node got connected in the wrong order!" );

		if( !nodeB->mInTextures[inChannelB].isValid() )
			++nodeB->mNumConnectedInputs;
		nodeB->mInTextures[inChannelB] = this->mOutTextures[outChannelA];

		if( nodeB->mNumConnectedInputs >= nodeB->mInTextures.size() )
			nodeB->routeOutputs();

		this->mConnectedNodes.push_back( nodeB );
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::connectFinalRT( RenderTarget *rt, CompositorChannel::TextureVec &textures,
											size_t inChannelA )
	{
		mInTextures[inChannelA].target		= rt;
		mInTextures[inChannelA].textures	= textures;
	}
	//-----------------------------------------------------------------------------------
	void CompositorNode::initializePasses(void)
	{
		CompositorTargetDefVec::const_iterator itor = mDefinition->mTargetPasses.begin();
		CompositorTargetDefVec::const_iterator end  = mDefinition->mTargetPasses.end();

		while( itor != end )
		{
			CompositorChannel const * channel = 0;
			size_t index;
			TextureDefinitionBase::TextureSource textureSource;
			mDefinition->getTextureSource( itor->getRenderTargetName(), index, textureSource );
			switch( textureSource )
			{
			case TextureDefinitionBase::TEXTURE_INPUT:
				channel = &mInTextures[index];
				break;
			case TextureDefinitionBase::TEXTURE_LOCAL:
				channel = &mLocalTextures[index];
				break;
			case TextureDefinitionBase::TEXTURE_GLOBAL:
				channel = &mWorkspace->getGlobalTexture( itor->getRenderTargetName() );
				break;
			}

			const CompositorPassDefVec &passes = itor->getCompositorPasses();
			CompositorPassDefVec::const_iterator itPass = passes.begin();
			CompositorPassDefVec::const_iterator enPass = passes.end();

			while( itPass != enPass )
			{
				CompositorPass *newPass = 0;
				switch( (*itPass)->getType() )
				{
				case PASS_SCENE:
					newPass = new CompositorPassScene( static_cast<CompositorPassSceneDef*>(*itPass),
														mWorkspace, channel->target );
					break;
				default:
					OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
								"Pass type not implemented or not recognized",
								"CompositorNode::initializePasses" );
					break;
				}

				mPasses.push_back( newPass );
				++itPass;
			}

			++itor;
		}
	}
}
