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
#include "OgreLight.h"

#include "OgreException.h"
#include "OgreSceneNode.h"
#include "OgreCamera.h"
#include "OgreSceneManager.h"

namespace Ogre {
    //-----------------------------------------------------------------------
    Light::Light( IdType id, ObjectMemoryManager *objectMemoryManager )
		: MovableObject( id, objectMemoryManager ),
		  mLightType(LT_POINT),
          mPosition(Vector3::ZERO),
          mDiffuse(ColourValue::White),
          mSpecular(ColourValue::Black),
          mDirection(Vector3::UNIT_Z),
		  mSpotOuter(Degree(40.0f)),
          mSpotInner(Degree(30.0f)),
          mSpotFalloff(1.0f),
          mSpotNearClip(0.0f),
		  mRange(100000),
		  mAttenuationConst(1.0f),
		  mAttenuationLinear(0.0f),
          mAttenuationQuad(0.0f),
		  mPowerScale(1.0f),
		  mIndexInFrame(0),
		  mOwnShadowFarDist(false),
		  mShadowFarDist(0),
		  mShadowFarDistSquared(0),
		  mShadowNearClipDist(-1),
		  mShadowFarClipDist(-1),
          mDerivedPosition(Vector3::ZERO),
          mDerivedDirection(Vector3::UNIT_Z),
		  mDerivedCamRelativePosition(Vector3::ZERO),
		  mDerivedCamRelativeDirty(false),
		  mCameraToBeRelativeTo(0),
          mDerivedTransformDirty(false),
		  mCustomShadowCameraSetup()
    {
		//mMinPixelSize should always be zero for lights otherwise lights will disapear
    	mMinPixelSize = 0;

		mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Real>::infinity();
		mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Real>::infinity();
    }
    //-----------------------------------------------------------------------
    Light::~Light()
    {
    }
    //-----------------------------------------------------------------------
    void Light::setType(LightTypes type)
    {
        mLightType = type;

		switch( mLightType )
		{
		case LT_POINT:
			mObjectData.mLocalRadius[mObjectData.mIndex] = mRange;
			mObjectData.mLocalAabb->setFromAabb( Aabb( Vector3::ZERO, Vector3( mRange ) ),
												 mObjectData.mIndex );
			break;
		case LT_DIRECTIONAL:
			mObjectData.mLocalAabb->setFromAabb( Aabb::BOX_INFINITE, mObjectData.mIndex );
			mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Real>::infinity();
			break;
		case LT_SPOTLIGHT:
			//TODO: (dark_sylinc)
			//mObjectData.mLocalRadius[mObjectData.mIndex] = mRange;
			//mObjectData.mLocalAabb->setFromAabb( Aabb( Vector3::ZERO, Vector3( mRange ) ) );
			break;
		}
    }
    //-----------------------------------------------------------------------
    Light::LightTypes Light::getType(void) const
    {
        return mLightType;
    }
    //-----------------------------------------------------------------------
    void Light::setPosition(Real x, Real y, Real z)
    {
        mPosition.x = x;
        mPosition.y = y;
        mPosition.z = z;
        mDerivedTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    void Light::setPosition(const Vector3& vec)
    {
        mPosition = vec;
        mDerivedTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getPosition(void) const
    {
        return mPosition;
    }
    //-----------------------------------------------------------------------
    void Light::setDirection(Real x, Real y, Real z)
    {
        mDirection.x = x;
        mDirection.y = y;
        mDirection.z = z;
        mDerivedTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    void Light::setDirection(const Vector3& vec)
    {
        mDirection = vec;
        mDerivedTransformDirty = true;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDirection(void) const
    {
        return mDirection;
    }
    //-----------------------------------------------------------------------
    void Light::setSpotlightRange(const Radian& innerAngle, const Radian& outerAngle, Real falloff)
    {
        mSpotInner = innerAngle;
        mSpotOuter = outerAngle;
        mSpotFalloff = falloff;

		if( mLightType == LT_SPOTLIGHT )
		{
			//TODO: (dark_sylinc) Change bounds
		}
    }
	//-----------------------------------------------------------------------
	void Light::setSpotlightInnerAngle(const Radian& val)
	{
		mSpotInner = val;
		if( mLightType == LT_SPOTLIGHT )
		{
			//TODO: (dark_sylinc) Change bounds
		}
	}
	//-----------------------------------------------------------------------
	void Light::setSpotlightOuterAngle(const Radian& val)
	{
		mSpotOuter = val;
		if( mLightType == LT_SPOTLIGHT )
		{
			//TODO: (dark_sylinc) Change bounds
		}
	}
	//-----------------------------------------------------------------------
	void Light::setSpotlightFalloff(Real val)
	{
		mSpotFalloff = val;
	}
    //-----------------------------------------------------------------------
    const Radian& Light::getSpotlightInnerAngle(void) const
    {
        return mSpotInner;
    }
    //-----------------------------------------------------------------------
    const Radian& Light::getSpotlightOuterAngle(void) const
    {
        return mSpotOuter;
    }
    //-----------------------------------------------------------------------
    Real Light::getSpotlightFalloff(void) const
    {
        return mSpotFalloff;
    }
    //-----------------------------------------------------------------------
    void Light::setDiffuseColour(Real red, Real green, Real blue)
    {
        mDiffuse.r = red;
        mDiffuse.b = blue;
        mDiffuse.g = green;
    }
    //-----------------------------------------------------------------------
    void Light::setDiffuseColour(const ColourValue& colour)
    {
        mDiffuse = colour;
    }
    //-----------------------------------------------------------------------
    const ColourValue& Light::getDiffuseColour(void) const
    {
        return mDiffuse;
    }
    //-----------------------------------------------------------------------
    void Light::setSpecularColour(Real red, Real green, Real blue)
    {
        mSpecular.r = red;
        mSpecular.b = blue;
        mSpecular.g = green;
    }
    //-----------------------------------------------------------------------
    void Light::setSpecularColour(const ColourValue& colour)
    {
        mSpecular = colour;
    }
    //-----------------------------------------------------------------------
    const ColourValue& Light::getSpecularColour(void) const
    {
        return mSpecular;
    }
    //-----------------------------------------------------------------------
    void Light::setAttenuation(Real range, Real constant,
                        Real linear, Real quadratic)
    {
        mRange = range;
        mAttenuationConst = constant;
        mAttenuationLinear = linear;
        mAttenuationQuad = quadratic;

		if( mLightType == LT_POINT )
		{
			mObjectData.mLocalRadius[mObjectData.mIndex] = mRange;
			mObjectData.mLocalAabb->setFromAabb( Aabb( Vector3::ZERO, Vector3( mRange ) ),
												 mObjectData.mIndex );
		}
		else if( mLightType == LT_SPOTLIGHT )
		{
			//TODO: (dark_sylinc)
		}
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationRange(void) const
    {
        return mRange;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationConstant(void) const
    {
        return mAttenuationConst;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationLinear(void) const
    {
        return mAttenuationLinear;
    }
    //-----------------------------------------------------------------------
    Real Light::getAttenuationQuadric(void) const
    {
        return mAttenuationQuad;
    }
    //-----------------------------------------------------------------------
	void Light::setPowerScale(Real power)
	{
		mPowerScale = power;
	}
    //-----------------------------------------------------------------------
	Real Light::getPowerScale(void) const
	{
		return mPowerScale;
	}
    //-----------------------------------------------------------------------
    void Light::update(void) const
    {
        if (mDerivedTransformDirty)
        {
            if (mParentNode)
            {
                // Ok, update with SceneNode we're attached to
                const Quaternion& parentOrientation = mParentNode->_getDerivedOrientation();
                const Vector3& parentPosition = mParentNode->_getDerivedPosition();
                mDerivedDirection = parentOrientation * mDirection;
                mDerivedPosition = (parentOrientation * mPosition) + parentPosition;
            }
            else
            {
                mDerivedPosition = mPosition;
                mDerivedDirection = mDirection;
            }

            mDerivedTransformDirty = false;
            //if the position has been updated we must update also the relative position
            mDerivedCamRelativeDirty = true;
        }
		if (mCameraToBeRelativeTo && mDerivedCamRelativeDirty)
		{
			mDerivedCamRelativePosition = mDerivedPosition - mCameraToBeRelativeTo->getDerivedPosition();
			mDerivedCamRelativeDirty = false;
		}
    }
    //-----------------------------------------------------------------------
    void Light::_notifyAttached(Node* parent, bool isTagPoint)
    {
        mDerivedTransformDirty = true;

        MovableObject::_notifyAttached( parent );
    }
    //-----------------------------------------------------------------------
    void Light::_notifyMoved(void)
    {
        mDerivedTransformDirty = true;

        MovableObject::_notifyMoved();
    }
    //-----------------------------------------------------------------------
    const AxisAlignedBox& Light::getBoundingBox(void) const
    {
        // Null, lights are not visible
        static AxisAlignedBox box;
        return box;
    }
    //-----------------------------------------------------------------------
    void Light::_updateRenderQueue(RenderQueue* queue, Camera *camera)
    {
        // Do nothing
    }
	//-----------------------------------------------------------------------
	void Light::visitRenderables(Renderable::Visitor* visitor, 
		bool debugRenderables)
	{
		// nothing to render
	}
    //-----------------------------------------------------------------------
    const String& Light::getMovableType(void) const
    {
		return LightFactory::FACTORY_TYPE_NAME;
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDerivedPosition(bool cameraRelative) const
    {
        update();
		if (cameraRelative && mCameraToBeRelativeTo)
		{
			return mDerivedCamRelativePosition;
		}
		else
		{
			return mDerivedPosition;
		}
    }
    //-----------------------------------------------------------------------
    const Vector3& Light::getDerivedDirection(void) const
    {
        update();
        return mDerivedDirection;
    }
    //-----------------------------------------------------------------------
    void Light::setVisible(bool visible)
    {
        MovableObject::setVisible(visible);
    }
    //-----------------------------------------------------------------------
	Vector4 Light::getAs4DVector(bool cameraRelativeIfSet) const
	{
		Vector4 ret;
        if (mLightType == Light::LT_DIRECTIONAL)
        {
            ret = -(getDerivedDirection()); // negate direction as 'position'
            ret.w = 0.0; // infinite distance
        }	
		else
        {
            ret = getDerivedPosition(cameraRelativeIfSet);
            ret.w = 1.0;
        }
		return ret;
	}
	//-----------------------------------------------------------------------
	uint32 Light::getTypeFlags(void) const
	{
		return SceneManager::LIGHT_TYPE_MASK;
	}
	//---------------------------------------------------------------------
	void Light::_calcTempSquareDist(const Vector3& worldPos)
	{
		if (mLightType == LT_DIRECTIONAL)
		{
			tempSquareDist = 0;
		}
		else
		{
			tempSquareDist = 
				(worldPos - getDerivedPosition()).squaredLength();
		}

	}
	//-----------------------------------------------------------------------
	const String& Light::getAnimableDictionaryName(void) const
	{
		return LightFactory::FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------
	void Light::initialiseAnimableDictionary(StringVector& vec) const
	{
		vec.push_back("diffuseColour");
		vec.push_back("specularColour");
		vec.push_back("attenuation");
		vec.push_back("spotlightInner");
		vec.push_back("spotlightOuter");
		vec.push_back("spotlightFalloff");

	}
	//-----------------------------------------------------------------------
	class LightDiffuseColourValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightDiffuseColourValue(Light* l) :AnimableValue(COLOUR) 
		{ mLight = l; }
		void setValue(const ColourValue& val)
		{
			mLight->setDiffuseColour(val);
		}
		void applyDeltaValue(const ColourValue& val)
		{
			setValue(mLight->getDiffuseColour() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getDiffuseColour());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpecularColourValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpecularColourValue(Light* l) :AnimableValue(COLOUR) 
		{ mLight = l; }
		void setValue(const ColourValue& val)
		{
			mLight->setSpecularColour(val);
		}
		void applyDeltaValue(const ColourValue& val)
		{
			setValue(mLight->getSpecularColour() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpecularColour());
		}

	};
	//-----------------------------------------------------------------------
	class LightAttenuationValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightAttenuationValue(Light* l) :AnimableValue(VECTOR4) 
		{ mLight = l; }
		void setValue(const Vector4& val)
		{
			mLight->setAttenuation(val.x, val.y, val.z, val.w);
		}
		void applyDeltaValue(const Vector4& val)
		{
			setValue(mLight->getAs4DVector() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getAs4DVector());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightInnerValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightInnerValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightInnerAngle(Radian(val));
		}
		void applyDeltaValue(Real val)
		{
			setValue(mLight->getSpotlightInnerAngle().valueRadians() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightInnerAngle().valueRadians());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightOuterValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightOuterValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightOuterAngle(Radian(val));
		}
		void applyDeltaValue(Real val)
		{
			setValue(mLight->getSpotlightOuterAngle().valueRadians() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightOuterAngle().valueRadians());
		}

	};
	//-----------------------------------------------------------------------
	class LightSpotlightFalloffValue : public AnimableValue
	{
	protected:
		Light* mLight;
	public:
		LightSpotlightFalloffValue(Light* l) :AnimableValue(REAL) 
		{ mLight = l; }
		void setValue(Real val)
		{
			mLight->setSpotlightFalloff(val);
		}
		void applyDeltaValue(Real val)
		{
			setValue(mLight->getSpotlightFalloff() + val);
		}
		void setCurrentStateAsBaseValue(void)
		{
			setAsBaseValue(mLight->getSpotlightFalloff());
		}

	};
	//-----------------------------------------------------------------------
	AnimableValuePtr Light::createAnimableValue(const String& valueName)
	{
		if (valueName == "diffuseColour")
		{
			return AnimableValuePtr(
				OGRE_NEW LightDiffuseColourValue(this));
		}
		else if(valueName == "specularColour")
		{
			return AnimableValuePtr(
				OGRE_NEW LightSpecularColourValue(this));
		}
		else if (valueName == "attenuation")
		{
			return AnimableValuePtr(
				OGRE_NEW LightAttenuationValue(this));
		}
		else if (valueName == "spotlightInner")
		{
			return AnimableValuePtr(
				OGRE_NEW LightSpotlightInnerValue(this));
		}
		else if (valueName == "spotlightOuter")
		{
			return AnimableValuePtr(
				OGRE_NEW LightSpotlightOuterValue(this));
		}
		else if (valueName == "spotlightFalloff")
		{
			return AnimableValuePtr(
				OGRE_NEW LightSpotlightFalloffValue(this));
		}
		else
		{
			return MovableObject::createAnimableValue(valueName);
		}
	}
	//-----------------------------------------------------------------------
	void Light::setCustomShadowCameraSetup(const ShadowCameraSetupPtr& customShadowSetup)
	{
		mCustomShadowCameraSetup = customShadowSetup;
	}
	//-----------------------------------------------------------------------
	void Light::resetCustomShadowCameraSetup()
	{
		mCustomShadowCameraSetup.setNull();
	}
	//-----------------------------------------------------------------------
	const ShadowCameraSetupPtr& Light::getCustomShadowCameraSetup() const
	{
		return mCustomShadowCameraSetup;
	}
	//-----------------------------------------------------------------------
	void Light::setShadowFarDistance(Real distance)
	{
		mOwnShadowFarDist = true;
		mShadowFarDist = distance;
		mShadowFarDistSquared = distance * distance;
	}
	//-----------------------------------------------------------------------
	void Light::resetShadowFarDistance(void)
	{
		mOwnShadowFarDist = false;
	}
	//-----------------------------------------------------------------------
	Real Light::getShadowFarDistance(void) const
	{
		if (mOwnShadowFarDist)
			return mShadowFarDist;
		else
			return mManager->getShadowFarDistance ();
	}
	//-----------------------------------------------------------------------
	Real Light::getShadowFarDistanceSquared(void) const
	{
		if (mOwnShadowFarDist)
			return mShadowFarDistSquared;
		else
			return mManager->getShadowFarDistanceSquared ();
	}
	//---------------------------------------------------------------------
	void Light::_setCameraRelative(Camera* cam)
	{
		mCameraToBeRelativeTo = cam;
		mDerivedCamRelativeDirty = true;
	}
	//---------------------------------------------------------------------
	Real Light::_deriveShadowNearClipDistance(const Camera* maincam) const
	{
		if (mShadowNearClipDist > 0)
			return mShadowNearClipDist;
		else
			return maincam->getNearClipDistance();
	}
	//---------------------------------------------------------------------
	Real Light::_deriveShadowFarClipDistance(const Camera* maincam) const
	{
		if (mShadowFarClipDist >= 0)
			return mShadowFarClipDist;
		else
		{
			if (mLightType == LT_DIRECTIONAL)
				return 0;
			else
				return mRange;
		}
	}
	//-----------------------------------------------------------------------
	void Light::setCustomParameter(uint16 index, const Ogre::Vector4 &value)
	{
		mCustomParameters[index] = value;
	}
	//-----------------------------------------------------------------------
	const Vector4 &Light::getCustomParameter(uint16 index) const
	{
		CustomParameterMap::const_iterator i = mCustomParameters.find(index);
        if (i != mCustomParameters.end())
        {
            return i->second;
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND, 
                "Parameter at the given index was not found.",
                "Light::getCustomParameter");
        }
	}
	//-----------------------------------------------------------------------
	void Light::_updateCustomGpuParameter(uint16 paramIndex, const GpuProgramParameters::AutoConstantEntry& constantEntry, GpuProgramParameters *params) const
	{
		CustomParameterMap::const_iterator i = mCustomParameters.find(paramIndex);
        if (i != mCustomParameters.end())
        {
            params->_writeRawConstant(constantEntry.physicalIndex, i->second, 
				constantEntry.elementCount);
        }
	}
	//-----------------------------------------------------------------------
	bool Light::isInLightRange(const Ogre::Sphere& container) const
	{
		bool isIntersect = true;
		//directional light always intersects (check only spotlight and point)
		if (mLightType != LT_DIRECTIONAL)
		{
			//Check that the sphere is within the sphere of the light
			isIntersect = container.intersects(Sphere(mDerivedPosition, mRange));
			//If this is a spotlight, check that the sphere is within the cone of the spot light
			if ((isIntersect) && (mLightType == LT_SPOTLIGHT))
			{
				//check first check of the sphere surrounds the position of the light
				//(this covers the case where the center of the sphere is behind the position of the light
				// something which is not covered in the next test).
				isIntersect = container.intersects(mDerivedPosition);
				//if not test cones
				if (!isIntersect)
				{
					//Calculate the cone that exists between the sphere and the center position of the light
					Ogre::Vector3 lightSphereConeDirection = container.getCenter() - mDerivedPosition;
					Ogre::Radian halfLightSphereConeAngle = Math::ASin(container.getRadius() / lightSphereConeDirection.length());

					//Check that the light cone and the light-position-to-sphere cone intersect)
					Radian angleBetweenConeDirections = lightSphereConeDirection.angleBetween(mDerivedDirection);
					isIntersect = angleBetweenConeDirections <=  halfLightSphereConeAngle + mSpotOuter * 0.5;
				}
			}
		}
		return isIntersect;
	}

	//-----------------------------------------------------------------------
	bool Light::isInLightRange(const Ogre::AxisAlignedBox& container) const
	{
		bool isIntersect = true;
		//Check the 2 simple / obvious situations. Light is directional or light source is inside the container
		if ((mLightType != LT_DIRECTIONAL) && (container.intersects(mDerivedPosition) == false))
		{
			//Check that the container is within the sphere of the light
			isIntersect = Math::intersects(Sphere(mDerivedPosition, mRange),container);
			//If this is a spotlight, do a more specific check
			if ((isIntersect) && (mLightType == LT_SPOTLIGHT) && (mSpotOuter.valueRadians() <= Math::PI))
			{
				//Create a rough bounding box around the light and check if
				Quaternion localToWorld = Vector3::NEGATIVE_UNIT_Z.getRotationTo(mDerivedDirection);

				Real boxOffset = Math::Sin(mSpotOuter * 0.5) * mRange;
				AxisAlignedBox lightBoxBound;
				lightBoxBound.merge(Vector3::ZERO);
				lightBoxBound.merge(localToWorld * Vector3(boxOffset, boxOffset, -mRange));
				lightBoxBound.merge(localToWorld * Vector3(-boxOffset, boxOffset, -mRange));
				lightBoxBound.merge(localToWorld * Vector3(-boxOffset, -boxOffset, -mRange));
				lightBoxBound.merge(localToWorld * Vector3(boxOffset, -boxOffset, -mRange));
				lightBoxBound.setMaximum(lightBoxBound.getMaximum() + mDerivedPosition);
				lightBoxBound.setMinimum(lightBoxBound.getMinimum() + mDerivedPosition);
				isIntersect = lightBoxBound.intersects(container);
				
				//If the bounding box check succeeded do one more test
				if (isIntersect)
				{
					//Check intersection again with the bounding sphere of the container
					//Helpful for when the light is at an angle near one of the vertexes of the bounding box
					isIntersect = isInLightRange(Sphere(container.getCenter(), 
						container.getHalfSize().length()));
				}
			}
		}
		return isIntersect;
	}
	//-----------------------------------------------------------------------
	//-----------------------------------------------------------------------
	String LightFactory::FACTORY_TYPE_NAME = "Light";
	//-----------------------------------------------------------------------
	const String& LightFactory::getType(void) const
	{
		return FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------
	MovableObject* LightFactory::createInstanceImpl( IdType id,
											ObjectMemoryManager *objectMemoryManager,
											const NameValuePairList* params )
	{

		Light* light = OGRE_NEW Light( id, objectMemoryManager );
 
		if(params)
		{
			NameValuePairList::const_iterator ni;

			// Setting the light type first before any property specific to a certain light type
			if ((ni = params->find("type")) != params->end())
			{
				if (ni->second == "point")
					light->setType(Light::LT_POINT);
				else if (ni->second == "directional")
					light->setType(Light::LT_DIRECTIONAL);
				else if (ni->second == "spotlight")
					light->setType(Light::LT_SPOTLIGHT);
				else
					OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
						"Invalid light type '" + ni->second + "'.",
						"LightFactory::createInstance");
			}

			// Common properties
			if ((ni = params->find("position")) != params->end())
				light->setPosition(StringConverter::parseVector3(ni->second));

			if ((ni = params->find("direction")) != params->end())
				light->setDirection(StringConverter::parseVector3(ni->second));

			if ((ni = params->find("diffuseColour")) != params->end())
				light->setDiffuseColour(StringConverter::parseColourValue(ni->second));

			if ((ni = params->find("specularColour")) != params->end())
				light->setSpecularColour(StringConverter::parseColourValue(ni->second));

			if ((ni = params->find("attenuation")) != params->end())
			{
				Vector4 attenuation = StringConverter::parseVector4(ni->second);
				light->setAttenuation(attenuation.x, attenuation.y, attenuation.z, attenuation.w);
			}

			if ((ni = params->find("castShadows")) != params->end())
				light->setCastShadows(StringConverter::parseBool(ni->second));

			if ((ni = params->find("visible")) != params->end())
				light->setVisible(StringConverter::parseBool(ni->second));

			if ((ni = params->find("powerScale")) != params->end())
				light->setPowerScale(StringConverter::parseReal(ni->second));

			if ((ni = params->find("shadowFarDistance")) != params->end())
				light->setShadowFarDistance(StringConverter::parseReal(ni->second));


			// Spotlight properties
			if ((ni = params->find("spotlightInner")) != params->end())
				light->setSpotlightInnerAngle(StringConverter::parseAngle(ni->second));

			if ((ni = params->find("spotlightOuter")) != params->end())
				light->setSpotlightOuterAngle(StringConverter::parseAngle(ni->second));

			if ((ni = params->find("spotlightFalloff")) != params->end())
				light->setSpotlightFalloff(StringConverter::parseReal(ni->second));
		}

		return light;
	}
	//-----------------------------------------------------------------------
	void LightFactory::destroyInstance( MovableObject* obj)
	{
		OGRE_DELETE obj;
	}




} // Namespace
