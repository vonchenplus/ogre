
#ifndef _OgreTerra_H_
#define _OgreTerra_H_

#include "OgrePrerequisites.h"
#include "OgreMovableObject.h"
#include "OgreShaderParams.h"

#include "Terra/TerrainCell.h"

namespace Ogre
{
    struct GridPoint
    {
        int32 x;
        int32 z;
    };

    struct GridDirection
    {
        int x;
        int z;
    };

    class ShadowMapper;

    class Terra : public MovableObject
    {
        friend class TerrainCell;

        std::vector<float>          m_heightMap;
        uint32                      m_width;
        uint32                      m_depth; //PNG's Height
        float                       m_depthWidthRatio;
        float                       m_skirtSize;
        float                       m_invWidth;
        float                       m_invDepth;

        Vector2     m_xzDimensions;
        Vector2     m_xzInvDimensions;
        Vector2     m_xzRelativeSize; // m_xzDimensions / [m_width, m_height]
        float       m_height;
        Vector3     m_terrainOrigin;
        uint32      m_basePixelDimension;

        std::vector<TerrainCell>   m_terrainCells;
        std::vector<TerrainCell*>  m_collectedCells[2];
        size_t                     m_currentCell;

        Ogre::TexturePtr    m_heightMapTex;
        Ogre::TexturePtr    m_normalMapTex;

        ShadowMapper        *m_shadowMapper;

        //Ogre stuff
        CompositorManager2      *m_compositorManager;
        Camera                  *m_camera;

        void destroyHeightmapTexture(void);

        /// Creates the Ogre texture based on the image data.
        /// Called by @see createHeightmap
        void createHeightmapTexture( const String &imageName, const Ogre::Image &image );

        /// Calls createHeightmapTexture, loads image data to our CPU-side buffers
        void createHeightmap( const String &imageName );

        void createNormalTexture(void);
        void destroyNormalTexture(void);

        ///	Automatically calculates the optimum skirt size (no gaps with
        /// lowest overdraw possible).
        ///	This is done by taking the heighest delta between two adjacent
        /// pixels in a 4x4 block.
        ///	This calculation may not be perfect, as the block search should
        /// get bigger for higher LODs.
        void calculateOptimumSkirtSize(void);

        inline GridPoint worldToGrid( const Vector3 &vPos ) const;
        inline Vector2 gridToWorld( const GridPoint &gPos ) const;

        bool isVisible( const GridPoint &gPos, const GridPoint &gSize ) const;

        void addRenderable( const GridPoint &gridPos, const GridPoint &cellSize, uint32 lodLevel );

        void optimizeCellsAndAdd(void);

    public:
        Terra( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *sceneManager,
               uint8 renderQueueId, CompositorManager2 *compositorManager, Camera *camera );
        ~Terra();

        void update( const Vector3 &lightDir );

        void load( const String &texName, const Vector3 center, const Vector3 &dimensions );

        /// load must already have been called.
        void setDatablock( HlmsDatablock *datablock );

        //MovableObject overloads
        const String& getMovableType(void) const;

        const ShadowMapper* getShadowMapper(void) const { return m_shadowMapper; }

        Ogre::TexturePtr getHeightMapTex(void) const    { return m_heightMapTex; }
        Ogre::TexturePtr getNormalMapTex(void) const    { return m_normalMapTex; }
        Ogre::TexturePtr _getShadowMapTex(void) const;
    };
}

#endif
