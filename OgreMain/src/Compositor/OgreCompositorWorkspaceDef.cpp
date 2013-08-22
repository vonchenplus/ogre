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

#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorManager2.h"
#include "OgreLogManager.h"

namespace Ogre
{
	CompositorWorkspaceDef::CompositorWorkspaceDef( IdString name,
													CompositorManager2 *compositorManager ) :
			TextureDefinitionBase( TEXTURE_GLOBAL ),
			mName( name ),
			mFinalInChannel( 0 ),
			mCompositorManager( compositorManager )
	{
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspaceDef::createImplicitAlias( IdString nodeName )
	{
		if( mAliasedNodes.find( nodeName ) == mAliasedNodes.end() )
		{
			if( !mCompositorManager->hasNodeDefinition( nodeName ) )
			{
				OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
							 "Can't find node " + nodeName.getFriendlyText(),
							 "CompositorWorkspaceDef::createImplicitAlias" );
			}
			else
			{
				//Create the implicit alias
				mAliasedNodes[nodeName] = nodeName;
			}
		}
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspaceDef::connect( uint32 outChannel, IdString outNode,
											uint32 inChannel, IdString inNode )
	{
		ChannelRouteList::const_iterator itor = mChannelRoutes.begin();
		ChannelRouteList::const_iterator end  = mChannelRoutes.end();

		while( itor != end )
		{
			if( itor->inNode == inNode && itor->inChannel == inChannel )
			{
				LogManager::getSingleton().logMessage( "WARNING: Node '" +
							itor->outNode.getFriendlyText() + "' and Node '" +
							outNode.getFriendlyText() + "' are both trying to connect "
							"to the same input channel #" + StringConverter::toString( inChannel ) +
							" from node '" + inNode.getFriendlyText() + "'. Only the latter will work" );
				break; //Early out
			}
			++itor;
		}

		createImplicitAlias( outNode );
		createImplicitAlias( inNode );

		mChannelRoutes.push_back( ChannelRoute( outChannel, outNode, inChannel, inNode ) );
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspaceDef::connectOutput( uint32 inChannel, IdString inNode )
	{
		createImplicitAlias( inNode );
		mFinalInChannel = inChannel;
		mFinalNode		= inNode;
	}
	//-----------------------------------------------------------------------------------
	void CompositorWorkspaceDef::addNodeAlias( IdString alias, IdString nodeName )
	{
		if( alias != nodeName && mCompositorManager->hasNodeDefinition( alias ) )
		{
			OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
						 "Can't use the name of a node definition as alias.",
						 "CompositorWorkspaceDef::addAlias" );
		}

		mAliasedNodes[alias] = nodeName;
	}
}
