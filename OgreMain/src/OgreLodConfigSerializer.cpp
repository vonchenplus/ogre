#include "OgreLodConfigSerializer.h"

namespace Ogre
{

	LodConfigSerializer::LodConfigSerializer()
	{
		mVersion = "[LodConfigSerializer_v0.1]";
	}

	void LodConfigSerializer::importLodConfig(Ogre::LodConfig* config, const Ogre::String& filename)
	{
		std::fstream *f = OGRE_NEW_T(std::fstream, MEMCATEGORY_GENERAL)();
		f->open(filename.c_str(), std::ios::binary | std::ios::in);
		if(f->is_open()){
			Ogre::DataStreamPtr stream(OGRE_NEW Ogre::FileStreamDataStream(f));

			importLodConfig(config, stream);

			stream->close();
		}
	}

	void LodConfigSerializer::importLodConfig(Ogre::LodConfig* config, DataStreamPtr& stream)
	{
		// Determine endianness (must be the first thing we do!)
		determineEndianness(stream);

		// Check header
		readFileHeader(stream);

		mStream = stream;
		mLodConfig = config;

		// Reset config file
		cleanup();


		unsigned short streamID;
		while(!stream->eof())
		{
			streamID = readChunk(stream);
			switch (streamID)
			{
			case LCCID_LOD_CONFIG:
				readLodConfig();
				break;
			}
		}
	}

	void LodConfigSerializer::cleanup(){
		mLodConfig->levels.clear();
		mLodConfig->advanced.profile.clear();
	}

	void LodConfigSerializer::readLodConfig()
	{
		while(!mStream->eof())
		{
			unsigned short streamID = readChunk(mStream);
			switch(streamID)
			{
			case LCCID_BASIC_INFO:
				readLodBasicInfo();
				break;
			case LCCID_LOD_LEVELS:
				readLodLevels();
				break;
			case LCCID_ADVANCED_INFO:
				readLodAdvancedInfo();
				break;
			case LCCID_PROFILE:
				readLodProfile();
				break;
			default:
				// Backpedal back to start of stream
				mStream->skip(-(int)calcChunkHeaderSize());
				return;
			}
		}
	}

	void LodConfigSerializer::readLodBasicInfo()
	{
		String group = readString(mStream);
		String name = readString(mStream);
		mLodConfig->mesh = MeshManager::getSingleton().load(name, group);

		String strategyName = readString(mStream);
		mLodConfig->strategy = LodStrategyManager::getSingleton().getStrategy(strategyName);
	}

	void LodConfigSerializer::readLodLevels()
	{
		uint32 size = 0;
		readInts(mStream, &size, 1);

		while(size--){
			LodLevel level;
			readFloats(mStream, &level.distance, 1);
			readInts(mStream, (Ogre::uint32*)&level.reductionMethod, 1);
			readFloats(mStream, &level.reductionValue, 1);
			mLodConfig->levels.push_back(level);
		}
	}

	void LodConfigSerializer::readLodAdvancedInfo()
	{
		readBools(mStream, &mLodConfig->advanced.useCompression, 1);
		readBools(mStream, &mLodConfig->advanced.useVertexNormals, 1);
	}

	void LodConfigSerializer::readLodProfile()
	{
		uint32 size = 0;
		readInts(mStream, &size, 1);
		mLodConfig->advanced.profile.clear();
		while(size--){
			ProfiledEdge pv;
			readObject(mStream, pv.src);
			readObject(mStream, pv.dst);
			readFloats(mStream, &pv.cost, 1);
			mLodConfig->advanced.profile.push_back(pv);
		}
	}

	void LodConfigSerializer::exportLodConfig(Ogre::LodConfig& config, const Ogre::String& filename, Endian endianMode /*= ENDIAN_NATIVE*/ )
	{
		std::fstream *f = OGRE_NEW_T(std::fstream, MEMCATEGORY_GENERAL)();
		f->open(filename.c_str(), std::ios::binary | std::ios::out);
		Ogre::DataStreamPtr stream(OGRE_NEW Ogre::FileStreamDataStream(f));

		exportLodConfig(config, stream, endianMode);

		stream->close();
	}

	void LodConfigSerializer::exportLodConfig(Ogre::LodConfig& config, Ogre::DataStreamPtr stream, Endian endianMode /*= ENDIAN_NATIVE*/ )
	{
		Ogre::LogManager::getSingleton().logMessage("MeshSerializer writing mesh data to stream " + stream->getName() + "...");

		// Decide on endian mode
		determineEndianness(endianMode);
		mLodConfig = &config;
		mStream = stream;
		if (!stream->isWriteable())
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"Unable to use stream " + stream->getName() + " for writing",
				"LodConfigSerializer::export");
		}

		writeFileHeader();
		LogManager::getSingleton().logMessage("File header written.");


		LogManager::getSingleton().logMessage("Writing Lod Config...");

		writeLodConfig();

		LogManager::getSingleton().logMessage("LodConfigSerializer export successful.");
	}

	void LodConfigSerializer::writeLodConfig()
	{
		writeChunkHeader(LCCID_LOD_CONFIG, calcLodConfigSize());
		writeLodBasicInfo();
		writeLodLevels();
		writeLodAdvancedInfo();
		writeLodProfile();
	}

	size_t LodConfigSerializer::calcLodConfigSize()
	{
		size_t size = calcChunkHeaderSize();

		size += calcLodBasicInfoSize();

		size += calcLodLevelsSize();

		size += calcLodAdvancedInfoSize();

		size += calcLodProfileSize();

		return size;
	}

	void LodConfigSerializer::writeLodBasicInfo()
	{
		writeChunkHeader(LCCID_BASIC_INFO, calcLodBasicInfoSize());
		writeString(mLodConfig->mesh->getGroup());
		writeString(mLodConfig->mesh->getName());
		writeString(mLodConfig->strategy->getName());
	}

	size_t LodConfigSerializer::calcLodBasicInfoSize()
	{
		size_t size = calcChunkHeaderSize();

		size += calcStringSize(mLodConfig->mesh->getGroup());
		size += calcStringSize(mLodConfig->mesh->getName());
		size += calcStringSize(mLodConfig->strategy->getName());

		return size;
	}

	void LodConfigSerializer::writeLodLevels()
	{
		writeChunkHeader(LCCID_LOD_LEVELS, calcLodLevelsSize());
		uint32 size = mLodConfig->levels.size();
		writeInts(&size, 1);

		LodConfig::LodLevelList::iterator it = mLodConfig->levels.begin();
		LodConfig::LodLevelList::iterator itEnd = mLodConfig->levels.end();
		for(;it != itEnd; it++){
			writeFloats(&it->distance, 1);
			writeInts((Ogre::uint32*)&it->reductionMethod, 1);
			writeFloats(&it->reductionValue, 1);
		}
	}

	size_t LodConfigSerializer::calcLodLevelsSize()
	{
		//lodLevel.distance
		size_t levelSize = sizeof(float);

		//lodLevel.reductionMethod;
		levelSize += sizeof(Ogre::uint32);

		//lodLevel.reductionValue;
		levelSize += sizeof(float);

		size_t size = calcChunkHeaderSize();

		// mLodConfig->levels.size()
		size += sizeof(Ogre::uint32);

		size += levelSize * mLodConfig->levels.size();

		return size;
	}

	void LodConfigSerializer::writeLodAdvancedInfo()
	{
		writeChunkHeader(LCCID_ADVANCED_INFO, calcLodAdvancedInfoSize());
		writeBools(&mLodConfig->advanced.useCompression, 1);
		writeBools(&mLodConfig->advanced.useVertexNormals, 1);
	}

	size_t LodConfigSerializer::calcLodAdvancedInfoSize()
	{
		size_t size = calcChunkHeaderSize();

		// mLodConfig->advanced.useCompression
		size += sizeof(bool);
		// mLodConfig->advanced.useVertexNormals
		size += sizeof(bool);

		return size;
	}

	void LodConfigSerializer::writeLodProfile()
	{
		if(mLodConfig->advanced.profile.empty()){
			return;
		}
		writeChunkHeader(LCCID_PROFILE, calcLodProfileSize());
		uint32 size = mLodConfig->advanced.profile.size();
		writeInts(&size, 1);
		LodProfile::iterator it = mLodConfig->advanced.profile.begin();
		LodProfile::iterator itEnd = mLodConfig->advanced.profile.end();
		for(;it != itEnd; it++){
			writeObject(it->src);
			writeObject(it->dst);
			writeFloats(&it->cost, 1);
		}
	}

	size_t LodConfigSerializer::calcLodProfileSize()
	{
		if(mLodConfig->advanced.profile.empty()){
			return 0;
		}
		// Vector3, LodProfile::ProfiledVertex::position
		size_t profiledVertexSize = sizeof(float) * 3;
		
		// LodProfile::ProfiledVertex::cost
		profiledVertexSize += sizeof(float);


		size_t size = calcChunkHeaderSize();

		// mLodConfig->advanced.profile->data.size()
		size += sizeof(uint32);

		size += profiledVertexSize * mLodConfig->advanced.profile.size();

		return size;
	}

}