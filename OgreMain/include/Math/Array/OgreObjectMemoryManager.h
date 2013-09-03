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
#ifndef __ObjectMemoryManager_H__
#define __ObjectMemoryManager_H__

#include "OgreObjectData.h"
#include "OgreArrayMemoryManager.h"

namespace Ogre
{
	enum SceneMemoryMgrTypes;

	/** \addtogroup Core
	*  @{
	*/
	/** \addtogroup Memory
	*  @{
	*/

	/** Wrap-around class that contains multiple ArrayMemoryManager, one per render queue
	@remarks
		This is the main memory manager that actually manages MovableObjects, and has to be called
		when a new MovableObject was created and when a MovableObject changes RenderQueue.
		@par
		Note that some SceneManager implementations (i.e. Octree like) may want to have more
		than one ObjectMemoryManager, for example one per octant.
	*/
	class ObjectMemoryManager : ArrayMemoryManager::RebaseListener
	{
		typedef vector<ObjectDataArrayMemoryManager>::type ArrayMemoryManagerVec;
		/// ArrayMemoryManagers grouped by hierarchy depth
		ArrayMemoryManagerVec					m_memoryManagers;

		/// Tracks total number of objects in all render queues.
		size_t									m_totalObjects;

		/// Dummy node where to point ObjectData::mParents[i] when they're unused slots.
		SceneNode								*m_dummyNode;
		Transform								m_dummyTransformPtrs;
		NullEntity								*m_dummyObject;

		/** Memory managers can have a 'twin' (optional). A twin is used when there
			static and dynamic scene managers, thus caching their pointers here is
			very convenient.
		*/
		SceneMemoryMgrTypes						m_memoryManagerType;
		ObjectMemoryManager						*m_twinMemoryManager;

		/** Makes m_memoryManagers big enough to be able to fulfill m_memoryManagers[newDepth]
		@param newDepth
			Hierarchy level depth we wish to grow to.
		*/
		void growToDepth( size_t newDepth );

	public:
		ObjectMemoryManager();
		~ObjectMemoryManager();

		/// @See m_memoryManagerType
		void _setTwin( SceneMemoryMgrTypes memoryManagerType, ObjectMemoryManager *twinMemoryManager );

		/// Note the return value can be null
		ObjectMemoryManager* getTwin() const						{ return m_twinMemoryManager; }
		SceneMemoryMgrTypes getMemoryManagerType() const			{ return m_memoryManagerType; }

		/** Requests memory for the given ObjectData, initializing values.
		@param outObjectData
			ObjectData with filled pointers
		@param renderQueue
			RenderQueue ID.
		*/
		void objectCreated( ObjectData &outObjectData, size_t renderQueue );

		/** Requests memory for the given ObjectData to be moved to a different render queue,
			transferring existing values inside to the new memory block
		@param inOutObjectData
			ObjectData with filled pointers
		@param oldRenderQueue
			RenderQueue it's living now.
		@param newRenderQueue
			RenderQueue it wants to live in.
		*/
		void objectMoved( ObjectData &inOutObjectData, size_t oldRenderQueue, size_t newRenderQueue );

		/** Releases current memory
		@param outObjectData
			ObjectData whose pointers will be nullified.
		@param renderQueue
			Current render queue it belongs to.
		*/
		void objectDestroyed( ObjectData &outObjectData, size_t renderQueue );

		/** Releases memory belonging to us, not before copying it into another manager.
		@remarks
			This function is useful when implementing multiple Memory Managers in Scene Managers
			or when switching nodes from Static to/from Dynamic.
		@param inOutTransform
			Valid Transform that belongs to us. Output will belong to the other memory mgr.
		@param depth
			Current hierarchy level depth it belongs to.
		@param dstObjectMemoryManager
			ObjectMemoryManager  that will now own the transform.
		*/
		void migrateTo( ObjectData &inOutTransform, size_t renderQueue,
						ObjectMemoryManager *dstObjectMemoryManager );

		/** Retrieves the number of render queues that have been created.
		@remarks
			The return value is equal or below m_memoryManagers.size(), you should cache
			the result instead of calling this function too often.
		*/
		size_t getNumRenderQueues() const;

		size_t _getTotalRenderQueues() const				{ return m_memoryManagers.size(); }

		/** Retrieves the sum of the number of objects in all render queues.
		@remarks
			The value is cached to avoid iterating through all RQ levels.
		*/
		size_t getTotalNumObjects() const					{ return m_totalObjects; }

		/// Returns the pointer to the dummy node (useful when detaching)
		SceneNode* _getDummyNode() const					{ return m_dummyNode; }

		/** Retrieves a ObjectData pointing to the first MovableObject in the given render queue
		@param outObjectData
			[out] ObjectData with filled pointers to the first MovableObject in this depth
		@param renderQueue
			Current render queue it belongs to.
		@return
			Number of MovableObject in this depth level
		*/
		size_t getFirstObjectData( ObjectData &outObjectData, size_t renderQueue );

		//Derived from ArrayMemoryManager::RebaseListener
		virtual void buildDiffList( ArrayMemoryManager::ManagerType managerType, uint16 level,
									const MemoryPoolVec &basePtrs,
									ArrayMemoryManager::PtrdiffVec &outDiffsList );
		virtual void applyRebase( ArrayMemoryManager::ManagerType managerType, uint16 level,
									const MemoryPoolVec &newBasePtrs,
									const ArrayMemoryManager::PtrdiffVec &diffsList );
		virtual void performCleanup( ArrayMemoryManager::ManagerType managerType, uint16 level,
									 const MemoryPoolVec &basePtrs, size_t const *elementsMemSizes,
									 size_t startInstance, size_t diffInstances );
	};

	/** @} */
	/** @} */
}

#endif
