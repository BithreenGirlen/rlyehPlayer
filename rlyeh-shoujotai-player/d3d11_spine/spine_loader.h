#ifndef SPINE_LOADER_H_
#define SPINE_LOADER_H_

#include "spine_shared_ptr.h"

#include <spine/Atlas.h>
#include <spine/SkeletonData.h>

namespace spine_loader
{
	SpineSharedPtr<spine::Atlas> CreateAtlasFromFile(const char* filePath, spine::TextureLoader* textureLoader);
	SpineSharedPtr<spine::Atlas> CreateAtlasFromMemory(const char* atlasFileData, int atlasFileDataLength, const char* textureDirectory, spine::TextureLoader* textureLoader);

	SpineSharedPtr<spine::SkeletonData> ReadTextSkeletonFromFile(const char* filePath, spine::Atlas* atlas);
	SpineSharedPtr<spine::SkeletonData> ReadBinarySkeletonFromFile(const char* filePath, spine::Atlas* atlas);

	SpineSharedPtr<spine::SkeletonData> ReadTextSkeletonFromMemory(const char* skeletonJson, spine::Atlas* atlas);
	SpineSharedPtr<spine::SkeletonData> ReadBinarySkeletonFromMemory(const unsigned char* skeletonBinary, int skeletonLength, spine::Atlas* atlas);
}

#endif
