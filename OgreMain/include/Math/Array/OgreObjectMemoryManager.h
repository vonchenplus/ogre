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
	class ObjectMemoryManager
	{
		typedef vector<ObjectDataArrayMemoryManager>::type ArrayMemoryManagerVec;
		/// ArrayMemoryManagers grouped by hierarchy depth
		ArrayMemoryManagerVec					m_memoryManagers;
		ArrayMemoryManager::RebaseListener		*m_rebaseListener;

		/// Dummy node where to point ObjectData::mParents[i] when they're unused slots.
		SceneNode								*m_dummyNode;
		Transform								m_dummyTransformPtrs;

		/** Makes m_memoryManagers big enough to be able to fulfill m_memoryManagers[newDepth]
		@param newDepth
			Hierarchy level depth we wish to grow to.
		*/
		void growToDepth( size_t newDepth );

	public:
		ObjectMemoryManager( ArrayMemoryManager::RebaseListener *rebaseListener );
		~ObjectMemoryManager();

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

		/** Retrieves the number of render queues that have been created.
		@remarks
			The return value is equal or below m_memoryManagers.size(), you should cache
			the result instead of calling this function too often.
		*/
		size_t getNumRenderQueues() const;

		/** Retrieves a ObjectData pointing to the first MovableObject in the given render queue
		@param outObjectData
			[out] ObjectData with filled pointers to the first MovableObject in this depth
		@param renderQueue
			Current render queue it belongs to.
		@return
			Number of MovableObject in this depth level
		*/
		size_t getFirstObjectData( ObjectData &outObjectData, size_t renderQueue );
	};

	/** @} */
	/** @} */
}

#endif
