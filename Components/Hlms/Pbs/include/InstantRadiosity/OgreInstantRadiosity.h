/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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
#ifndef _OgreInstantRadiosity_H_
#define _OgreInstantRadiosity_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreHlmsBufferManager.h"
#include "OgreConstBufferPool.h"
#include "OgreRay.h"
#include "OgreRawPtr.h"
#include "Math/Array/OgreArrayRay.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Component
    *  @{
    */
    /** \addtogroup Material
    *  @{
    */

    class RandomNumberGenerator;

    class _OgreHlmsPbsExport InstantRadiosity
    {
        struct MeshData
        {
            float * RESTRICT_ALIAS vertexPos;
            //float * RESTRICT_ALIAS vertexUv[8];
            /// Index data may be directly pointing to IndexBufferPacked's shadow copy.
            /// Don't free the memory in that case!
            union
            {
                uint8 * RESTRICT_ALIAS indexData;
                uint8 const * RESTRICT_ALIAS indexDataConst;
            };
            size_t  numVertices;
            size_t  numIndices;
            bool    useIndices16bit;
        };

        struct RayHit
        {
            Real    distance;
            Real    accumDistance;
            //Vector3 pointOnTri;
            Vector3 materialDiffuse;
            Vector3 triVerts[3];
            Vector3 triNormal;
            Ray ray;
        };
        struct Vpl
        {
            Light *light;
            Vector3 diffuse;
            Vector3 position;
            Vector3 normal;
            Real    numMergedVpls;
        };

        struct SparseCluster
        {
            int32   blockHash[3];
            Vector3 diffuse;
            Vector3 direction;

            SparseCluster();
            SparseCluster( int32 blockX, int32 blockY, int32 blockZ,
                           const Vector3 _diffuse, const Vector3 dir );
            SparseCluster( int32 _blockHash[3] );

            bool operator () ( const SparseCluster &_l, int32 _r[3] ) const;
            bool operator () ( int32 _l[3], const SparseCluster &_r ) const;
            bool operator () ( const SparseCluster &_l, const SparseCluster &_r ) const;
        };

        typedef vector<RayHit>::type RayHitVec;
        typedef vector<Vpl>::type VplVec;
        typedef set<SparseCluster, SparseCluster>::type SparseClusterSet;

        struct OrderRenderOperation
        {
            bool operator () ( const v1::RenderOperation &_l, const v1::RenderOperation &_r ) const;
        };

        SceneManager    *mSceneManager;
        HlmsManager     *mHlmsManager;
    public:
        uint8           mFirstRq;
        uint8           mLastRq;
        uint32          mVisibilityMask;
        uint32          mLightMask;

        /// Number of rays to trace. More usually results in more accuracy. Sometimes really
        /// low values (e.g. 32 rays) may achieve convincing results with high performance, while
        /// high large values (e.g. 10000) achieve more accurate results.
        size_t          mNumRays;
        /// In range [0; inf). Controls how many bounces we'll generate.
        /// Increases the total number of rays (i.e. more than mNumRays).
        size_t          mNumRayBounces;
        /// In range (0; 1]; how many rays that fired in the previous bounce should survive
        /// for a next round of bounces.
        Real            mSurvivingRayFraction;
        /// Controls how we cluster multiple VPLs into one averaged VPL. Smaller values generate
        /// more VPLs (reducing performance but improving quality). Bigger values result in less
        /// VPLs (higher performance, less quality)
        Real            mCellSize;
        /// Value ideally in range (0; 1]
        /// When 1, the VPL is placed at exactly the location where the light ray hits the triangle.
        /// At 0.99 it will be placed at 99% the distance from light to the location (i.e. moves away
        /// from the triangle). Using Bias can help with light bleeding, and also allows reducing
        /// mVplMaxRange (thus increasing performance) at the cost of lower accuracy but still
        /// "looking good".
        Real            mBias;
        uint32          mNumSpreadIterations;
        Real            mSpreadThreshold;

        /// Areas of interest. Only used for directional lights. Normally you don't want to
        /// use this system for empty landscapes because a regular environment map and simple
        /// math can take care of that. You want to focus on a particular building, or
        /// in different cities; but not everything.
        /// If left unfilled, the system will auto-calculate one (not recommended).
        typedef vector<Aabb>::type AabbVec;
        AabbVec         mAoI;

        /// ANY CHANGE TO A mVpl* variable will take effect after calling updateExistingVpls
        /// (or calling build)
        /// How big each VPL should be. Larger ranges leak light more but also are more accurate
        /// in the sections they lit correctly, but they are also get more expensive.
        Real            mVplMaxRange;
        Real            mVplConstAtten;
        Real            mVplLinearAtten;
        Real            mVplQuadAtten;
        /// If all three components of the diffuse colour of a VPL light is below this threshold,
        /// the VPL is removed (useful for improving performance for VPLs that barely contribute
        /// to the scene).
        Real            mVplThreshold;
        /// Tweaks how strong VPL lights should be.
        /// In range (0; inf)
        Real            mVplPowerBoost;
    private:
        size_t          mTotalNumRays; /// Includes bounces. Autogenerated.
        VplVec          mVpls;
        RayHitVec       mRayHits;
        RawSimdUniquePtr<ArrayRay, MEMCATEGORY_GENERAL> mArrayRays;

        FastArray<size_t> mTmpRaysThatHitObject[ARRAY_PACKED_REALS];
        SparseClusterSet  mTmpSparseClusters[3];

        typedef map<VertexArrayObject*, MeshData>::type MeshDataMapV2;
        typedef map<v1::RenderOperation, MeshData, OrderRenderOperation>::type MeshDataMapV1;
        MeshDataMapV2   mMeshDataMapV2;
        MeshDataMapV1   mMeshDataMapV1;

        vector<Item*>::type mDebugMarkers;
        bool                mEnableDebugMarkers;

        /**
        @param lightPos
        @param lightRot
        @param lightType
        @param angle
        @param lightColour
        @param lightRange
        @param attenConst
        @param attenLinear
        @param attenQuad
        @param areaOfInfluence
            Only used for directional light types. See mAoI
        */
        void processLight( Vector3 lightPos, const Quaternion &lightRot, uint8 lightType,
                           Radian angle, Vector3 lightColour, Real lightRange,
                           Real attenConst, Real attenLinear, Real attenQuad,
                           const Aabb &areaOfInfluence );

        /// Generates the ray bounces based on mRayHits[raySrcStart] through
        /// mRayHits[raySrcStart+raySrcCount-1]; generating up to 'raysToGenerate' rays
        /// Returns the number of actually generated rays (which is <= raysToGenerate)
        /// The generated rays are stored between mRayHits[raySrcStart+raySrcCount] &
        /// mRayHits[raySrcStart+raySrcCount+returnValue]
        size_t generateRayBounces( size_t raySrcStart, size_t raySrcCount,
                                   size_t raysToGenerate, RandomNumberGenerator &rng);

        const MeshData* downloadVao( VertexArrayObject *vao );
        const MeshData* downloadRenderOp( const v1::RenderOperation &renderOp );

        void testLightVsAllObjects( uint8 lightType, Real lightRange,
                                    ObjectData objData, size_t numNodes,
                                    const Aabb &areaOfInfluence,
                                    size_t rayStart, size_t numRays );
        void raycastLightRayVsMesh( Real lightRange, const MeshData meshData,
                                    Matrix4 worldMatrix, Vector3 materialDiffuse,
                                    const FastArray<size_t> &raysThatHitObj );

        Vpl convertToVpl( Vector3 lightColour, Vector3 pointOnTri, const RayHit &hit );
        /// Generates the VPLs from a particular lights, and clusters them.
        void generateAndClusterVpls( Vector3 lightColour, Real attenConst,
                                     Real attenLinear, Real attenQuad );
        void spreadSparseClusters( const SparseClusterSet &grid0, SparseClusterSet &inOutGrid1 );
        void createVplsFromSpreadClusters( const SparseClusterSet &spreadCluster );
        /// Clusters the VPL from all lights (these VPLs may have been clustered with other
        /// VPLs from the same light, now we need to do this again with lights from different
        /// clusters)
        void clusterAllVpls(void);
        void autogenerateAreaOfInfluence(void);

        void createDebugMarkers(void);
        void destroyDebugMarkers(void);

    public:
        InstantRadiosity( SceneManager *sceneManager, HlmsManager *hlmsManager );
        ~InstantRadiosity();

        /// Does nothing if build hasn't been called yet.
        /// Updates VPLs with the latest changes made to all mVpl* variables.
        /// May create/remove VPL lights because of mVplThreshold
        void updateExistingVpls(void);

        /// Clears everything, removing our VPLs. Does not freeMemory.
        /// You will have to call build again to get VPLs again.
        void clear(void);

        void build(void);

        /// "build" will download meshes for raycasting. We will not free
        /// them after build (in case you want to build again).
        /// If you wish to free that memory, call this function.
        void freeMemory(void);

        void setEnableDebugMarkers( bool bEnable );
        bool getEnableDebugMarkers(void) const      { return mEnableDebugMarkers; }
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"

#endif
