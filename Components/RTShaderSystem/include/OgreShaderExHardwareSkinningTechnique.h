/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2009 Torus Knot Software Ltd
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
#ifndef _ShaderExHardwareSkinningTechnique_
#define _ShaderExHardwareSkinningTechnique_

#include "OgreShaderPrerequisites.h"
#ifdef RTSHADER_SYSTEM_BUILD_EXT_SHADERS
#include "OgreShaderSubRenderState.h"
#include "OgreShaderParameter.h"
#include "OgreRenderSystem.h"
#include "OgreShaderFunctionAtom.h"

#define HS_MAX_WEIGHT_COUNT 4

namespace Ogre {
namespace RTShader {

class HardwareSkinningFactory;

/** \addtogroup Core
*  @{
*/
/** \addtogroup RTShader
*  @{
*/

/** Implement a sub render state which performs hardware skinning.
Meaning, this sub render states adds calculations which multiply
the points and normals by their assigned bone matricies.
*/
class _OgreRTSSExport HardwareSkinningTechnique : public SubRenderState
{
public:
	struct SkinningData
	{
		SkinningData() :
			isValid(true), maxBoneCount(0), maxWeightCount(0)
		{}

		bool isValid;
		ushort maxBoneCount;
		ushort maxWeightCount;
	};

	enum SkinningType
	{
		ST_LINEAR,
		ST_DUAL_QUATERNION
	};

// Interface.
public:

	/** Class default constructor */
	HardwareSkinningTechnique();

	/**
	@see SubRenderState::getType.
	*/
	virtual const String& getType() const;

	/**
	@see SubRenderState::getType.
	*/
	virtual int getExecutionOrder() const;

	/**
	@see SubRenderState::copyFrom.
	*/
	virtual void copyFrom(const SubRenderState& rhs);

	/**
	Set the hardware skinning parameters.
	@param boneCount The maximum number of bones in the model this material
		 is assigned to. Note that this parameter can be higher but not
		 lower than the actual number of bones.
	@param weightCount The maximum number of weights/bones affecting
		a vertex. Note that this parameter can be higher but not
		 lower than the actual number of affecting bones.
	@param dualQuaternionSkinning
	@param correctAntipodalityHandling Only applicable for dual quaternion skinning.
	@param scalingShearingSupport Only applicable for dual quaternion skinning.
	*/
	void setHardwareSkinningParam(ushort boneCount, ushort weightCount, SkinningType skinningType = ST_LINEAR,  bool correctAntipodalityHandling = false, bool scalingShearingSupport = false);

	/**
	Returns the number of bones in the model assigned to the material.
	@see setHardwareSkinningParam().
	*/
	ushort getBoneCount();

	/**
	Returns the number of weights/bones affecting a vertex.
	@see setHardwareSkinningParam().
	*/
	ushort getWeightCount();

	/**
	
	 */
	SkinningType getSkinningType();

	/**
	Only applicable for dual quaternion skinning.
	@see setHardwareSkinningParam()
	*/
	bool hasCorrectAntipodalityHandling();

	/**
	Only applicable for dual quaternion skinning.
	@see setHardwareSkinningParam()
	*/
	bool hasScalingShearingSupport();

	/**
	@see SubRenderState::preAddToRenderState.
	*/
	virtual bool preAddToRenderState(const RenderState* renderState, Pass* srcPass, Pass* dstPass) = 0;

	/**
	Set the factory which created this sub render state
	*/
	void _setCreator(const HardwareSkinningFactory* pCreator) { mCreator = pCreator; }
	
	static String Type;

// Protected methods
protected:
	/**
	@see SubRenderState::resolveParameters.
	*/
	virtual bool resolveParameters (ProgramSet* programSet) = 0;

	/**
	@see SubRenderState::resolveDependencies.
	*/
	virtual bool resolveDependencies (ProgramSet* programSet) = 0;

	/**
	@see SubRenderState::addFunctionInvocations.
	*/
	virtual bool addFunctionInvocations (ProgramSet* programSet) = 0;

	/** Translates an index number to a mask value */
	Operand::OpMask indexToMask (int index);

// Attributes.
protected:
	ushort mBoneCount;
	ushort mWeightCount;
	SkinningType mSkinningType;
	bool mCorrectAntipodalityHandling;
	bool mScalingShearingSupport;

	bool mDoBoneCalculations;

	ParameterPtr mParamInPosition;
	ParameterPtr mParamInNormal;
	ParameterPtr mParamInBiNormal;
	ParameterPtr mParamInTangent;
	ParameterPtr mParamInIndices;
	ParameterPtr mParamInWeights;
	UniformParameterPtr mParamInWorldMatrices;
	UniformParameterPtr mParamInInvWorldMatrix;
	UniformParameterPtr mParamInViewProjMatrix;
	UniformParameterPtr mParamInWorldMatrix;
	UniformParameterPtr mParamInWorldViewProjMatrix;

	ParameterPtr mParamTempFloat4;
	ParameterPtr mParamTempFloat3;
	ParameterPtr mParamLocalPositionWorld;
	ParameterPtr mParamLocalNormalWorld;
	ParameterPtr mParamLocalTangentWorld;
	ParameterPtr mParamLocalBinormalWorld;
	ParameterPtr mParamOutPositionProj;

	const HardwareSkinningFactory* mCreator; ///The factory which created this sub render state
};

_OgreRTSSExport void operator<<(std::ostream& o, const HardwareSkinningTechnique::SkinningData& data);

}
}

#endif
#endif

