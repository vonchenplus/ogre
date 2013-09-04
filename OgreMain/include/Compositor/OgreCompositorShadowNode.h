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

#ifndef __CompositorShadowNode_H__
#define __CompositorShadowNode_H__

#include "OgreHeaderPrefix.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorShadowNodeDef.h"
#include "OgreShadowCameraSetup.h"

namespace Ogre
{
	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup Effects
	*  @{
	*/

	/** Shadow Nodes are special nodes (not to be confused with @see CompositorNode)
		that are only used for rendering shadow maps.
		Normal Compositor Nodes can share or own a ShadowNode. The ShadowNode will
		render the scene enough times to fill all shadow maps so the main scene pass
		can use them.
	@par
		ShadowNode are very flexible compared to Ogre 1.x; as they allow mixing multiple
		shadow camera setups for different lights.
	@par
		Shadow Nodes derive from nodes so that they can be used as regular nodes
	@par
		During a render with shadow mapping enabled, in theory we should render first
		the Shadow Node's pass, then render the regular scene. However in practice we
		need information that is calculated during the reulgar scene render, namely:
			* An AABB enclosing all visible objects (calculated in cullFrusum)
			* An AABB enclosing all visible objects that receive shadows (also in cullFrusum)
		Unfortunately calculating them twice (first for shadow map, then for the regular pass)
		is very expensive so the smart thing to do is to reuse such data.

		As a result, Ogre divides the rendering into two stages: The culling phase (01),
		and the rendering phase (02). Ogre first calls the culling phase 01 of the regular
		pass, and the resulting output is:
			* An array(s) containing all visible/culled objects (@See SceneManager::mVisibleObjects)
			* The 2 aabbs we need. (@See SceneManager::mVisibleObjsPerRenderQueue)
		The next step, before entering rendering phase 02, is to update the shadow node (which
		implies entering both its cull & render phases); only then, enter rendering phase 02.

		There is a caveat: When entering shadow node's cull phase 01, the array of visible
		objects is overwritten, but we'll still need it for phase 02. As a result, we save
		the content of the array before updating the shadow node, and restore it afterwards.

		To summarize: a normal rendering flow with shadow map looks like this:
			normal->_cullPhase01();
			saveCulledObjects( normal->getSceneManager() );
				shadowNode->setupShadowCamera( normal->getVisibleBoundsInfo() );
				shadowNode->_cullPhase01();
				shadowNode->_renderPhase02();
			restoreCulledObjects( normal->getSceneManager() );
			normal->_renderPhase02();

		Another issues that has to take care is that if shadow map will render render queues 0 to 4
		and the normal pass only renders RQs from 0 to 2, then unfortunately we'll need to
		calculate the bounds information of RQs 3 & 4.

		It may sound complicated, but it's just the old rendering sequence divided into stages.
		It's much more elegant than Ogre 1.x, which just interrupted the normal rendering sequence
		with lots of branches and called itself recursively to render the shadow maps.
		This separation also provides a way to isolate & encapsulate systems (SceneManager now has
		no idea of how to take care of shadow map rendering)
    @author
		Matias N. Goldberg
    @version
        1.0
    */
	class _OgreExport CompositorShadowNode : public CompositorNode
	{
		CompositorShadowNodeDef const *mDefinition;

		struct ShadowMapCamera
		{
			ShadowCameraSetupPtr	shadowCameraSetup;
			Camera					*camera;
		};

		typedef vector<ShadowMapCamera>::type ShadowMapCameraVec;
		/// One per shadow map (whether texture or atlas)
		ShadowMapCameraVec		mShadowMapCameras;

	public:
		CompositorShadowNode( IdType id, const CompositorShadowNodeDef *definition,
								CompositorWorkspace *workspace, RenderSystem *renderSys );
		~CompositorShadowNode();

		/** Renders into the shadow map, executes passes
		@param camera
			Camera used to calculate our shadow camera (in case of directional lights).
		*/
		void _update( Camera* camera );

		/// We derive so we can override the camera with ours
		virtual void postInitializePassScene( CompositorPassScene *pass );
	};

	/** @} */
	/** @} */
}

#include "OgreHeaderSuffix.h"

#endif
