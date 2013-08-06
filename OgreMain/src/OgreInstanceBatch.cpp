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
#include "OgreInstanceManager.h"
#include "OgreInstanceBatch.h"
#include "OgreSubMesh.h"
#include "OgreRenderOperation.h"
#include "OgreInstancedEntity.h"
#include "OgreSceneNode.h"
#include "OgreCamera.h"
#include "OgreLodStrategy.h"
#include "OgreException.h"

namespace Ogre
{
	InstanceBatch::InstanceBatch( IdType id, ObjectMemoryManager *objectMemoryManager,
									InstanceManager *creator, MeshPtr &meshReference,
									const MaterialPtr &material, size_t instancesPerBatch,
									const Mesh::IndexMap *indexToBoneMap, const String &batchName ) :
				Renderable(),
                MovableObject( id, objectMemoryManager ),
				mInstancesPerBatch( instancesPerBatch ),
				mCreator( creator ),
				mMaterial( material ),
				mMeshReference( meshReference ),
				mIndexToBoneMap( indexToBoneMap ),
				mCurrentCamera( 0 ),
				mIsStatic( false ),
				mMaterialLodIndex( 0 ),
				mTechnSupportsSkeletal( true ),
				mCachedCamera( 0 ),
				mTransformSharingDirty(true),
				mRemoveOwnVertexData(false),
				mRemoveOwnIndexData(false)
	{
		assert( mInstancesPerBatch );

		//Force batch visibility to be always visible. The instanced entities
		//have individual visibility flags. If none matches the scene's current,
		//then this batch won't rendered.
		setVisibilityFlags( std::numeric_limits<Ogre::uint32>::max() );

		if( indexToBoneMap )
		{
			assert( !(meshReference->hasSkeleton() && indexToBoneMap->empty()) );
		}

		mName = batchName;

		mCustomParams.resize( mCreator->getNumCustomParams() * mInstancesPerBatch, Ogre::Vector4::ZERO );
	}

	InstanceBatch::~InstanceBatch()
	{
		deleteAllInstancedEntities();

		//Remove the parent scene node automatically
		SceneNode *sceneNode = getParentSceneNode();
		if( sceneNode )
		{
			sceneNode->detachAllObjects();
			sceneNode->getParentSceneNode()->removeAndDestroyChild( sceneNode );
		}

		if( mRemoveOwnVertexData )
			OGRE_DELETE mRenderOperation.vertexData;
		if( mRemoveOwnIndexData )
			OGRE_DELETE mRenderOperation.indexData;

	}

	void InstanceBatch::_setInstancesPerBatch( size_t instancesPerBatch )
	{
		if( !mInstancedEntities.empty() )
		{
			OGRE_EXCEPT(Exception::ERR_INVALID_STATE, "Instances per batch can only be changed before"
						" building the batch.", "InstanceBatch::_setInstancesPerBatch");
		}

		mInstancesPerBatch = instancesPerBatch;
	}
	//-----------------------------------------------------------------------
	bool InstanceBatch::checkSubMeshCompatibility( const SubMesh* baseSubMesh )
	{
		if( baseSubMesh->operationType != RenderOperation::OT_TRIANGLE_LIST )
		{
			OGRE_EXCEPT(Exception::ERR_NOT_IMPLEMENTED, "Only meshes with OT_TRIANGLE_LIST are supported",
						"InstanceBatch::checkSubMeshCompatibility");
		}

		if( !mCustomParams.empty() && mCreator->getInstancingTechnique() != InstanceManager::HWInstancingBasic )
		{
			//Implementing this for ShaderBased is impossible. All other variants can be.
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Custom parameters not supported for this "
														"technique. Do you dare implementing it?"
														"See InstanceManager::setNumCustomParams "
														"documentation.",
						"InstanceBatch::checkSubMeshCompatibility");
		}

		return true;
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_updateAnimations(void)
	{
		InstancedEntityArray::const_iterator itor = mAnimatedEntities.begin();
		InstancedEntityArray::const_iterator end  = mAnimatedEntities.end();

		while( itor != end )
		{
			(*itor)->_updateAnimation();
			++itor;
		}
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_updateBounds(void)
	{
		//If this assert triggers, then we did not properly remove ourselves from
		//the Manager's update list (it's a performance optimization warning)
		assert( mUnusedEntities.size() != mInstancedEntities.size() );

		//First update all bounds from our objects
		ObjectData objData;
		const size_t numObjs = mLocalObjectMemoryManager.getFirstObjectData( objData, 0 );
		MovableObject::updateAllBounds( numObjs, objData );

		//Now merge the bounds to ours
		ArrayReal maxWorldRadius = ARRAY_REAL_ZERO;
		ArrayVector3 vMinBounds( Mathlib::MAX_POS, Mathlib::MAX_POS, Mathlib::MAX_POS );
		ArrayVector3 vMaxBounds( Mathlib::MAX_NEG, Mathlib::MAX_NEG, Mathlib::MAX_NEG );

		for( size_t i=0; i<numObjs; i += ARRAY_PACKED_REALS )
		{
			ArrayReal * RESTRICT_ALIAS worldRadius = reinterpret_cast<ArrayReal*RESTRICT_ALIAS>
																		(objData.mWorldRadius);
			ArrayInt * RESTRICT_ALIAS visibilityFlags = reinterpret_cast<ArrayInt*RESTRICT_ALIAS>
																		(objData.mVisibilityFlags);
			ArrayReal inUse = CastIntToReal(Mathlib::TestFlags4( *visibilityFlags,
																 Mathlib::SetAll( LAYER_VISIBILITY ) ));

			//Merge with bounds only if they're in use (and not explicitly hidden,
			//but may be invisible for some cameras or out of frustum)
			ArrayVector3 newVal( vMinBounds );
			newVal.makeFloor( objData.mWorldAabb->m_center - objData.mWorldAabb->m_halfSize );
			vMinBounds.CmovRobust( inUse, newVal );

			newVal = vMaxBounds;
			newVal.makeCeil( objData.mWorldAabb->m_center + objData.mWorldAabb->m_halfSize );
			vMaxBounds.CmovRobust( inUse, newVal );

			maxWorldRadius = Mathlib::Max( maxWorldRadius, *worldRadius );

			objData.advanceDirtyInstanceMgr();
		}

		//We've been merging and processing in bulks, but we now need to join all simd results
		Vector3 vMin = vMinBounds.collapseMin();
		Vector3 vMax = vMaxBounds.collapseMax();

		Real maxRadius = Mathlib::ColapseMax( maxWorldRadius );

		Aabb aabb = Aabb::newFromExtents( vMin - maxRadius, vMax + maxRadius );
		mObjectData.mLocalAabb->setFromAabb( aabb, mObjectData.mIndex );
		mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::updateVisibility(void)
	{
#ifdef ENABLE_INCOMPATIBLE_OGRE_2_0
		mVisible = false;

		InstancedEntityVec::const_iterator itor = mInstancedEntities.begin();
		InstancedEntityVec::const_iterator end  = mInstancedEntities.end();

		while( itor != end && !mVisible )
		{
			//Trick to force Ogre not to render us if none of our instances is visible
			//Because we do Camera::isVisible(), it is better if the SceneNode from the
			//InstancedEntity is not part of the scene graph (i.e. ultimate parent is root node)
			//to avoid unnecessary wasteful calculations
			mVisible |= (*itor)->findVisible( mCurrentCamera );
			++itor;
		}
#endif
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::createAllInstancedEntities()
	{
		mInstancedEntities.reserve( mInstancesPerBatch );
		mUnusedEntities.reserve( mInstancesPerBatch );
		mAnimatedEntities.reserve( mInstancesPerBatch );

		for( size_t i=0; i<mInstancesPerBatch; ++i )
		{
			InstancedEntity *instance = generateInstancedEntity(i);
			mInstancedEntities.push_back( instance );
			mUnusedEntities.push_back( instance );
		}
	}
	//-----------------------------------------------------------------------
	InstancedEntity* InstanceBatch::generateInstancedEntity(size_t num)
	{
		return OGRE_NEW InstancedEntity( Id::generateNewId<InstancedEntity>(),
										 &mLocalObjectMemoryManager, this, num );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::deleteAllInstancedEntities()
	{
		InstancedEntityVec::const_iterator itor = mInstancedEntities.begin();
		InstancedEntityVec::const_iterator end  = mInstancedEntities.end();

		while( itor != end )
		{
			if( (*itor)->getParentSceneNode() )
				(*itor)->getParentSceneNode()->detachObject( (*itor) );

			OGRE_DELETE *itor++;
		}
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::deleteUnusedInstancedEntities()
	{
		InstancedEntityVec::const_iterator itor = mUnusedEntities.begin();
		InstancedEntityVec::const_iterator end  = mUnusedEntities.end();

		while( itor != end )
			OGRE_DELETE *itor++;

		mUnusedEntities.clear();
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::makeMatrixCameraRelative3x4( float *mat3x4, size_t numFloats )
	{
		const Vector3 &cameraRelativePosition = mCurrentCamera->getDerivedPosition();

		for( size_t i=0; i<numFloats >> 2; i += 3 )
		{
			const Vector3 worldTrans( mat3x4[(i+0) * 4 + 3], mat3x4[(i+1) * 4 + 3],
										mat3x4[(i+2) * 4 + 3] );
			const Vector3 newPos( worldTrans - cameraRelativePosition );

			mat3x4[(i+0) * 4 + 3] = (float)newPos.x;
			mat3x4[(i+1) * 4 + 3] = (float)newPos.y;
			mat3x4[(i+2) * 4 + 3] = (float)newPos.z;
		}
	}
	//-----------------------------------------------------------------------
	RenderOperation InstanceBatch::build( const SubMesh* baseSubMesh )
	{
		if( checkSubMeshCompatibility( baseSubMesh ) )
		{
			//Only triangle list at the moment
			mRenderOperation.operationType	= RenderOperation::OT_TRIANGLE_LIST;
			mRenderOperation.srcRenderable	= this;
			mRenderOperation.useIndexes	= true;
			setupVertices( baseSubMesh );
			setupIndices( baseSubMesh );

			createAllInstancedEntities();
		}

		return mRenderOperation;
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::buildFrom( const SubMesh *baseSubMesh, const RenderOperation &renderOperation )
	{
		mRenderOperation = renderOperation;
		createAllInstancedEntities();
	}
	//-----------------------------------------------------------------------
	InstancedEntity* InstanceBatch::createInstancedEntity()
	{
		InstancedEntity *retVal = 0;

		if( !mUnusedEntities.empty() )
		{
			if( mUnusedEntities.size() == mInstancedEntities.size() && !mIsStatic && mCreator )
				mCreator->_addToDynamicBatchList( this );

			retVal = mUnusedEntities.back();
			mUnusedEntities.pop_back();

			retVal->setInUse(true);
		}

		return retVal;
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::removeInstancedEntity( InstancedEntity *instancedEntity )
	{
		if( instancedEntity->mBatchOwner != this )
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
						"Trying to remove an InstancedEntity from scene created"
						" with a different InstanceBatch",
						"InstanceBatch::removeInstancedEntity()");
		}
		if( !instancedEntity->isInUse() )
		{
			OGRE_EXCEPT(Exception::ERR_INVALID_STATE,
						"Trying to remove an InstancedEntity that is already removed!",
						"InstanceBatch::removeInstancedEntity()");
		}

		if( instancedEntity->getParentSceneNode() )
			instancedEntity->getParentSceneNode()->detachObject( instancedEntity );

		instancedEntity->setInUse(false);
		instancedEntity->stopSharingTransform();

		//Put it back into the queue
		mUnusedEntities.push_back( instancedEntity );

		if( mUnusedEntities.size() == mInstancedEntities.size() && !mIsStatic && mCreator )
			mCreator->_removeFromDynamicBatchList( this );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_addAnimatedInstance( InstancedEntity *instancedEntity )
	{
		assert( std::find( mAnimatedEntities.begin(), mAnimatedEntities.end(), instancedEntity ) ==
				mAnimatedEntities.end() && "Calling _addAnimatedInstance twice" );
		assert( instancedEntity->mBatchOwner == this && "Instanced Entity should belong to us" );

		mAnimatedEntities.push_back( instancedEntity );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_removeAnimatedInstance( const InstancedEntity *instancedEntity )
	{
		InstanceBatch::InstancedEntityArray::iterator itor = std::find( mAnimatedEntities.begin(),
																		mAnimatedEntities.end(),
																		instancedEntity );
		if( itor != mAnimatedEntities.end() )
			efficientVectorRemove( mAnimatedEntities, itor );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::getInstancedEntitiesInUse( InstancedEntityVec &outEntities,
													CustomParamsVec &outParams )
	{
		InstancedEntityVec::const_iterator itor = mInstancedEntities.begin();
		InstancedEntityVec::const_iterator end  = mInstancedEntities.end();

		while( itor != end )
		{
			if( (*itor)->isInUse() )
			{
				outEntities.push_back( *itor );

				for( unsigned char i=0; i<mCreator->getNumCustomParams(); ++i )
					outParams.push_back( _getCustomParam( *itor, i ) );
			}

			++itor;
		}
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::defragmentBatchNoCull( InstancedEntityVec &usedEntities,
												CustomParamsVec &usedParams )
	{
		const size_t maxInstancesToCopy = std::min( mInstancesPerBatch, usedEntities.size() );
		InstancedEntityVec::iterator first = usedEntities.end() - maxInstancesToCopy;
		CustomParamsVec::iterator firstParams = usedParams.end() - maxInstancesToCopy *
																	mCreator->getNumCustomParams();

		//Copy from the back to front, into m_instancedEntities
		mInstancedEntities.insert( mInstancedEntities.begin(), first, usedEntities.end() );
		//Remove them from the array
		usedEntities.resize( usedEntities.size() - maxInstancesToCopy );	

		mCustomParams.insert( mCustomParams.begin(), firstParams, usedParams.end() );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::defragmentBatchDoCull( InstancedEntityVec &usedEntities,
												CustomParamsVec &usedParams )
	{
		//Get the the entity closest to the minimum bbox edge and put into "first"
		InstancedEntityVec::const_iterator itor   = usedEntities.begin();
		InstancedEntityVec::const_iterator end   = usedEntities.end();

		Vector3 vMinPos = Vector3::ZERO, firstPos = Vector3::ZERO;
		InstancedEntity *first = 0;

		if( !usedEntities.empty() )
		{
			first      = *usedEntities.begin();
			firstPos   = first->getParentNode()->_getDerivedPosition();
			vMinPos    = first->getParentNode()->_getDerivedPosition();
		}

		while( itor != end )
		{
			const Vector3 &vPos      = (*itor)->getParentNode()->_getDerivedPosition();

			vMinPos.x = Ogre::min( vMinPos.x, vPos.x );
			vMinPos.y = Ogre::min( vMinPos.y, vPos.y );
			vMinPos.z = Ogre::min( vMinPos.z, vPos.z );

			if( vMinPos.squaredDistance( vPos ) < vMinPos.squaredDistance( firstPos ) )
			{
				firstPos   = vPos;
			}

			++itor;
		}

		//Now collect entities closest to 'first'
		while( !usedEntities.empty() && mInstancedEntities.size() < mInstancesPerBatch )
		{
			InstancedEntityVec::iterator closest   = usedEntities.begin();
			InstancedEntityVec::iterator it        = usedEntities.begin();
			InstancedEntityVec::iterator e         = usedEntities.end();

			Vector3 closestPos;
			closestPos = (*closest)->getParentNode()->_getDerivedPosition();

			while( it != e )
			{
				const Vector3 &vPos   = (*it)->getParentNode()->_getDerivedPosition();

				if( firstPos.squaredDistance( vPos ) < firstPos.squaredDistance( closestPos ) )
				{
					closest      = it;
					closestPos   = vPos;
				}

				++it;
			}

			mInstancedEntities.push_back( *closest );
			//Now the custom params
			const size_t idx = closest - usedEntities.begin();	
			for( unsigned char i=0; i<mCreator->getNumCustomParams(); ++i )
			{
				mCustomParams.push_back( usedParams[idx + i] );
			}

			//Remove 'closest' from usedEntities & usedParams using swap and pop_back trick
			*closest = *(usedEntities.end() - 1);
			usedEntities.pop_back();

			for( unsigned char i=1; i<=mCreator->getNumCustomParams(); ++i )
			{
				usedParams[idx + mCreator->getNumCustomParams() - i] = *(usedParams.end() - 1);
				usedParams.pop_back();
			}
		}
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_defragmentBatch( bool optimizeCulling, InstancedEntityVec &usedEntities,
											CustomParamsVec &usedParams )
	{
		//Remove and clear what we don't need
		mInstancedEntities.clear();
		mCustomParams.clear();
		deleteUnusedInstancedEntities();

		if( !optimizeCulling )
			defragmentBatchNoCull( usedEntities, usedParams );
		else
			defragmentBatchDoCull( usedEntities, usedParams );

		//Reassign instance IDs and tell we're the new parent
		uint32 instanceId = 0;
		InstancedEntityVec::const_iterator itor = mInstancedEntities.begin();
		InstancedEntityVec::const_iterator end  = mInstancedEntities.end();

		while( itor != end )
		{
			(*itor)->mInstanceId = instanceId++;
			(*itor)->mBatchOwner = this;
			++itor;
		}

		//Recreate unused entities, if there's left space in our container
		assert( (signed)(mInstancesPerBatch) - (signed)(mInstancedEntities.size()) >= 0 );
		mInstancedEntities.reserve( mInstancesPerBatch );
		mUnusedEntities.reserve( mInstancesPerBatch );
		mAnimatedEntities.reserve( mInstancesPerBatch );
		mCustomParams.reserve( mCreator->getNumCustomParams() * mInstancesPerBatch );
		for( size_t i=mInstancedEntities.size(); i<mInstancesPerBatch; ++i )
		{
			InstancedEntity *instance = generateInstancedEntity(i);
			mInstancedEntities.push_back( instance );
			mUnusedEntities.push_back( instance );
			mCustomParams.push_back( Ogre::Vector4::ZERO );
		}

		//We've potentially changed our bounds
		if( !isBatchUnused() )
			updateStaticDirty();
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_defragmentBatchDiscard(void)
	{
		//Remove and clear what we don't need
		mInstancedEntities.clear();
		deleteUnusedInstancedEntities();
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::setStatic( bool bStatic )
	{
		if( mIsStatic != bStatic )
		{
			mIsStatic = bStatic;
			if( bStatic )
			{
				if( mCreator )
				{
					mCreator->_removeFromDynamicBatchList( this );
					mCreator->_addDirtyStaticBatch( this );
				}
			}
			else
			{
				if( mCreator && mUnusedEntities.size() != mInstancedEntities.size() )
					mCreator->_addToDynamicBatchList( this );
			}
		}
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::updateStaticDirty(void)
	{
		if( mCreator && mIsStatic )
			mCreator->_addDirtyStaticBatch( this );
	}
	//-----------------------------------------------------------------------
	const String& InstanceBatch::getMovableType(void) const
	{
		static String sType = "InstanceBatch";
		return sType;
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_notifyCurrentCamera( Camera* cam )
	{
		mCurrentCamera = cam;

		//See DistanceLodStrategy::getValueImpl()
		//We use our own because our SceneNode is just filled with zeroes, and updating it
		//with real values is expensive, plus we would need to make sure it doesn't get to
		//the shader
		Real depth = Math::Sqrt( getSquaredViewDepth(cam) ) -
					 mMeshReference->getBoundingSphereRadius();
        depth = max( depth, Real(0) );
        Real lodValue = depth * cam->_getLodBiasInverse();

		//Now calculate Material LOD
        /*const LodStrategy *materialStrategy = m_material->getLodStrategy();
        
        //Calculate lod value for given strategy
        Real lodValue = materialStrategy->getValue( this, cam );*/

        //Get the index at this depth
        unsigned short idx = mMaterial->getLodIndex( lodValue );

		//TODO: Replace subEntity for MovableObject
        // Construct event object
        /*EntityMaterialLodChangedEvent subEntEvt;
        subEntEvt.subEntity = this;
        subEntEvt.camera = cam;
        subEntEvt.lodValue = lodValue;
        subEntEvt.previousLodIndex = m_materialLodIndex;
        subEntEvt.newLodIndex = idx;

        //Notify lod event listeners
        cam->getSceneManager()->_notifyEntityMaterialLodChanged(subEntEvt);*/

        //Change lod index
        mMaterialLodIndex = idx;
	}
	//-----------------------------------------------------------------------
	Real InstanceBatch::getSquaredViewDepth( const Camera* cam ) const
	{
		if( mCachedCamera != cam )
		{
			mCachedCameraDist = std::numeric_limits<Real>::infinity();

			InstancedEntityVec::const_iterator itor = mInstancedEntities.begin();
			InstancedEntityVec::const_iterator end  = mInstancedEntities.end();

			while( itor != end )
			{
				if( (*itor)->isVisible() )
					mCachedCameraDist = std::min( mCachedCameraDist, (*itor)->getSquaredViewDepth( cam ) );
				++itor;
			}

			mCachedCamera = cam;
		}

        return mCachedCameraDist;
	}
	//-----------------------------------------------------------------------
	const LightList& InstanceBatch::getLights( void ) const
	{
		return queryLights();
	}
	//-----------------------------------------------------------------------
	Technique* InstanceBatch::getTechnique( void ) const
	{
		return mMaterial->getBestTechnique( mMaterialLodIndex, this );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_updateRenderQueue( RenderQueue* queue, Camera *camera )
	{
		queue->addRenderable( this, mRenderQueueID, mRenderQueuePriority );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::visitRenderables( Renderable::Visitor* visitor, bool debugRenderables )
	{
		visitor->visit( this, 0, false );
	}
	//-----------------------------------------------------------------------
	void InstanceBatch::_setCustomParam( InstancedEntity *instancedEntity, unsigned char idx,
										 const Vector4 &newParam )
	{
		mCustomParams[instancedEntity->mInstanceId * mCreator->getNumCustomParams() + idx] = newParam;
	}
	//-----------------------------------------------------------------------
	const Vector4& InstanceBatch::_getCustomParam( InstancedEntity *instancedEntity, unsigned char idx )
	{
		return mCustomParams[instancedEntity->mInstanceId * mCreator->getNumCustomParams() + idx];
	}
}
