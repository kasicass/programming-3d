#include <windows.h>
#include <stdio.h>

#include <d3d9.h>
#include <ddraw.h>    // Required for DirectX's DDSURFACEDESC2 structure definition
#include "GutDDS.h"

static int GetPixelSize(D3DFORMAT fmt)
{
	struct sMap
	{
		D3DFORMAT fmt;
		int pixelsize;
	};

	sMap table[] =
	{
		// float16 format
		{D3DFMT_R16F, 2},
		{D3DFMT_G16R16F, 4},
		{D3DFMT_A16B16G16R16F, 8},
		// float32 format
		{D3DFMT_R32F, 4},
		{D3DFMT_G32R32F, 8},
		{D3DFMT_A32B32G32R32F, 16},
		// int8 format
		{D3DFMT_A8R8G8B8, 4},
		{D3DFMT_A8B8G8R8, 4},
		{D3DFMT_X8R8G8B8, 4},
		{D3DFMT_L8, 1},
		{D3DFMT_A8, 1},
		// int16 format
		{D3DFMT_A16B16G16R16, 8},
		{D3DFMT_L8, 1}
	};

	const int size = sizeof(table)/sizeof(sMap);
	for (int i=0; i<size; i++ )
	{
		if ( table[i].fmt==fmt )
		{
			return table[i].pixelsize;
		}
	}

	return 0;
}

/*
 `从NVIDIA网站上下载, 做了一些修改让它也可以读取非压缩的DDS格式, 例如HDR浮点数格式贴图.`
*/
DDS_IMAGE_DATA* loadDDSTextureFile( const char *filename )
{
	DDS_IMAGE_DATA *pDDSImageData = NULL;
	DDSURFACEDESC2 ddsd;
	char filecode[4];
	FILE *pFile = NULL;
	int factor = 1;
	int bufferSize = 0;

	// `打开档案`
	pFile = fopen( filename, "rb" );
	if( pFile == NULL )
	{
	#ifdef _DEBUG
		printf( "loadDDSTextureFile couldn't find, or failed to load \"%s\"\n", filename );
	#endif
		return NULL;
	}

	// `确认它是个DDS档`
	fread( filecode, 1, 4, pFile );

	if( strncmp( filecode, "DDS ", 4 ) != 0 )
	{
		printf( "The file \"%s\" doesn't appear to be a valid .dds file!\n", filename );
		fclose( pFile );
		return NULL;
	}

	// `获得图形格式`
	fread( &ddsd, sizeof(ddsd), 1, pFile );

	pDDSImageData = (DDS_IMAGE_DATA*) malloc(sizeof(DDS_IMAGE_DATA));
	memset( pDDSImageData, 0, sizeof(DDS_IMAGE_DATA) );
	memcpy(&pDDSImageData->ddsd, &ddsd, sizeof(DDSURFACEDESC2));

	pDDSImageData->bBlock = false;
	//
	// This .dds loader supports the loading of compressed formats DXT1, DXT3 and DXT5.
	//
	switch( ddsd.ddpfPixelFormat.dwFourCC )
	{
	case FOURCC_DXT1: // `DXT1压缩比为8:1`
		pDDSImageData->format = D3DFMT_DXT1;
		factor = 2;
		pDDSImageData->bBlock = true;
		break;

	case FOURCC_DXT3: // `DXT3压缩比为4:1`
		pDDSImageData->format = D3DFMT_DXT3;
		factor = 4;
		pDDSImageData->bBlock = true;
		break;

	case FOURCC_DXT5: // `DXT5压缩比为4:1`
		pDDSImageData->format = D3DFMT_DXT5;
		factor = 4;
		pDDSImageData->bBlock = true;
		break;

	default:
		// `DDS也可以存储其它非压缩格式`
		if ( ddsd.ddpfPixelFormat.dwFlags & DDPF_FOURCC )
		{
			pDDSImageData->format = ddsd.ddpfPixelFormat.dwFourCC;
			if ( ddsd.ddpfPixelFormat.dwRGBBitCount )
			{
				pDDSImageData->PixelSize = ddsd.ddpfPixelFormat.dwRGBBitCount/8;
			}
			else
			{
				pDDSImageData->PixelSize = GetPixelSize((D3DFORMAT)ddsd.ddpfPixelFormat.dwFourCC);
			}
		}
		break;
	}

	pDDSImageData->cubeMap = (ddsd.ddsCaps.dwCaps & DDSCAPS_COMPLEX) && (ddsd.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP);
    
	//
	// `预估图片占用内存的大小`
	// 
	if( ddsd.dwLinearSize == 0 )
	{
		int current_pos = ftell(pFile);
		fseek(pFile, 0, SEEK_END);
		int end_pos = ftell(pFile);
		fseek(pFile, current_pos, SEEK_SET);
		bufferSize = end_pos - current_pos;	
	}
	else
	{
		if( ddsd.dwMipMapCount > 1 )
			bufferSize = ddsd.dwLinearSize * factor;
		else
			bufferSize = ddsd.dwLinearSize;
	}

	pDDSImageData->pixels = (unsigned char*)malloc(bufferSize * sizeof(unsigned char));

	fread( pDDSImageData->pixels, 1, bufferSize, pFile );

	// Close the file
	fclose( pFile );

	pDDSImageData->width      = ddsd.dwWidth;
	pDDSImageData->height     = ddsd.dwHeight;
	pDDSImageData->depth	  = ddsd.dwDepth;
	pDDSImageData->BufferSize = bufferSize;
	pDDSImageData->numMipMaps = ddsd.dwMipMapCount==0 ? 1 : ddsd.dwMipMapCount;

	return pDDSImageData;
}
