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
#include "OgreD3D11DepthBuffer.h"
#include "OgreD3D11HardwarePixelBuffer.h"
#include "OgreD3D11Texture.h"
#include "OgreD3D11Mappings.h"

namespace Ogre
{
    D3D11DepthBuffer::D3D11DepthBuffer( uint16 poolId, D3D11RenderSystem *renderSystem,
                                        ID3D11DepthStencilView *depthBufferView,
                                        ID3D11ShaderResourceView *depthTextureView,
                                        uint32 width, uint32 height, uint32 fsaa,
                                        uint32 multiSampleQuality, PixelFormat pixelFormat,
                                        bool isDepthTexture, bool isManual ) :
                DepthBuffer( poolId, 0, width, height, fsaa, "", pixelFormat,
                             isDepthTexture, isManual ),
                mDepthStencilView( depthBufferView ),
                mDepthTextureView( depthTextureView ),
                mMultiSampleQuality( multiSampleQuality),
                mRenderSystem(renderSystem)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC pDesc;
        mDepthStencilView->GetDesc( &pDesc );
        // Unknown PixelFormat at the moment
        PixelFormat format = D3D11Mappings::_getPF(pDesc.Format);
		mBitDepth = PixelUtil::getNumElemBytes(format) * 8;
    }

    D3D11DepthBuffer::~D3D11DepthBuffer()
    {
        if( !mManual )
            mDepthStencilView->Release();
        mDepthStencilView = 0;
    }
    //---------------------------------------------------------------------
    bool D3D11DepthBuffer::isCompatible( RenderTarget *renderTarget, bool exactFormatMatch ) const
    {
        ID3D11Texture2D *D3D11texture = NULL;
        renderTarget->getCustomAttribute( "First_ID3D11Texture2D", &D3D11texture );
        D3D11_TEXTURE2D_DESC BBDesc;
        D3D11texture->GetDesc( &BBDesc );

        //RenderSystem will determine if bitdepths match (i.e. 32 bit RT don't like 16 bit Depth)
        //This is the same function used to create them. Note results are usually cached so this should
        //be quick

        if( mFsaa == BBDesc.SampleDesc.Count &&
            mMultiSampleQuality == BBDesc.SampleDesc.Quality &&
            this->getWidth() == renderTarget->getWidth() &&
            this->getHeight() == renderTarget->getHeight() &&
            mDepthTexture == renderTarget->prefersDepthTexture() &&
            ((!exactFormatMatch && mFormat == PF_D24_UNORM_S8_UINT) ||
             mFormat == renderTarget->getDesiredDepthBufferFormat()) )
        {
            return true;
        }

        return false;
    }
    //---------------------------------------------------------------------
    ID3D11DepthStencilView* D3D11DepthBuffer::getDepthStencilView() const
    {
        return mDepthStencilView;
    }
    //---------------------------------------------------------------------
    ID3D11ShaderResourceView* D3D11DepthBuffer::getDepthTextureView() const
    {
        return mDepthTextureView;
    }
    //---------------------------------------------------------------------
    void D3D11DepthBuffer::_resized(ID3D11DepthStencilView *depthBufferView, uint32 width, uint32 height)
    {
        mHeight = height;
        mWidth = width;

        mDepthStencilView = depthBufferView;
    }
}
