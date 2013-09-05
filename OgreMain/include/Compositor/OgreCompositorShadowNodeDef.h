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

#ifndef __CompositorShadowNodeDef_H__
#define __CompositorShadowNodeDef_H__

#include "OgreHeaderPrefix.h"

#include "Compositor/OgreCompositorNodeDef.h"

namespace Ogre
{
	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup Effects
	*  @{
	*/

	enum ShadowMapTechniques
	{
		SHADOWMAP_DEFAULT,
		SHADOWMAP_PLANEOPTIMAL,
		SHADOWMAP_FOCUSED,
		SHADOWMAP_LiPSSM,
		SHADOWMAP_PSSM
	};

	/** Shadow Nodes are special nodes (not to be confused with @see CompositorNode)
		that are only used for rendering shadow maps.
		Normal Compositor Nodes can share or own a ShadowNode. The ShadowNode will
		render the scene enough times to fill all shadow maps so the main scene pass
		can use them.
	@par
		ShadowNode are very flexible compared to Ogre 1.x; as they allow mixing multiple
		shadow camera setups for different lights.
    @author
		Matias N. Goldberg
    @version
        1.0
    */
	class _OgreExport CompositorShadowNodeDef : public CompositorNodeDef
	{
		friend class CompositorShadowNode;

	public:
		/// Local texture definition
        class ShadowTextureDefinition : public CompositorInstAlloc
        {
        public:
            
            uint		width;
            uint		height;
            PixelFormatList formatList; // more than one means MRT
			uint		fsaa;			// FSAA level
			bool		hwGammaWrite;	// Do sRGB gamma correction on write (only 8-bit per channel formats)
			uint16		depthBufferId;	// Depth Buffer's pool ID

			size_t		light;	//Render Nth closest light
			size_t		split;	//Split for that light (only for PSSM/CSM)
			ShadowMapTechniques	shadowMapTechnique;
			IdString	name;

			ShadowTextureDefinition( ShadowMapTechniques t, IdString _name,
									size_t _light, size_t _split ) :
					width(1024), height(1024), fsaa(0), hwGammaWrite(false), depthBufferId(2),
					light(_light), split(_split), shadowMapTechnique(t), name( _name ) {}
        };

	protected:
		typedef vector<ShadowTextureDefinition>::type	ShadowMapTexDefVec;
		ShadowMapTexDefVec	mShadowMapTexDefinitions;
		ShadowMapTechniques	mDefaultTechnique;

	public:
		CompositorShadowNodeDef( IdString name ) :
				CompositorNodeDef( name ), mDefaultTechnique( SHADOWMAP_DEFAULT ) {}

		/// Overloaded to prevent creating input channels.
		virtual IdString addTextureSourceName( const String &name, size_t index,
												TextureSource textureSource );

		/** Reserves enough memory for all texture definitions
		@remarks
			Calling this function is not obligatory, but recommended
		@param numTex
			The number of shadow textures expected to contain.
		*/
		void setNumShadowTextureDefinitions( size_t numTex )
														{ mShadowMapTexDefinitions.reserve( numTex ); }

		/** Adds a new ShadowTexture definition.
		@remarks
			WARNING: Calling this function may invalidate all previous returned pointers
			unless you've properly called setNumShadowTextureDefinitions
		@param lightIdx
			Nth Closest Light to assign this texture to. Must be unique unless split is different.
		@param split
			Split for the given light. Only valid for CSM/PSSM shadow maps.
			Must be unique for the same lightIdx.
		@param name
			Name to alias this texture for reference. Can be blank. If not blank, must be
			unique and not contain the "global_" prefix. We need a hard copy
		@param isAtlas
			True if this shadow map is rendered in an UV atlas; which means we don't create
			our own texture, but rather reference another through the local name.
		*/
		ShadowTextureDefinition* addShadowTextureDefinition( size_t lightIdx, size_t split,
															 String name, bool isAtlas );

		/** Checks that paremeters are correctly set, and finalizes whatever needs to be
			done, probably because not enough data was available at the time of creation.
		@remarks
			If possible, try to validate parameters at creation time to avoid delaying
			when the error shows up.
			We should validate here if it's not possible to validate at any other time
			or if it's substantially easier to do so here.
		*/
		virtual void _validateAndFinish();
	};

	/** @} */
	/** @} */
}

#include "OgreHeaderSuffix.h"

#endif
