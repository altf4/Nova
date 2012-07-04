//============================================================================
// Name        : SerializationHelper.cpp
// Copyright   : DataSoft Corporation 2011-2012
//	Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Helper functions for serialization
//============================================================================/*

#include "SerializationHelper.h"
#include "Logger.h"

#include <sstream>
#include <string.h>

using namespace std;

namespace Nova
{

bool SerializeChunk(u_char* buf, uint32_t* offset, char* dataToSerialize, uint32_t size, uint32_t maxBufferSize)
{
	if (*offset + size > maxBufferSize)
	{
		stringstream ss;
		ss << "Serialization error. Max buffer size was " << maxBufferSize;
		ss << " but attempting to write " << size << " bytes at offset " << *offset;
		LOG(ERROR, ss.str(), "");

		throw serializationException();
		return false;
	}

	memcpy(buf+*offset, dataToSerialize, size);
	*offset += size;


	// Doing a diff of serialize/deserialize with this uncommented is really useful for debugging
	//cout << "Offset " << *offset << " Size: " << size << endl;

	return true;
}

bool DeserializeChunk(u_char* buf, uint32_t* offset, char* deserializeTo, uint32_t size, uint32_t maxBufferSize)
{
	if (*offset + size > maxBufferSize)
	{
		stringstream ss;
		ss << "Deserialization error. Max buffer size was " << maxBufferSize;
		ss << " but attempting to read " << size << " bytes at offset " << *offset;
		LOG(ERROR, ss.str(), "");

		throw serializationException();
		return false;
	}

	memcpy(deserializeTo, buf+*offset, size);
	*offset += size;

	// Doing a diff of serialize/deserialize with this uncommented is really useful for debugging
	//cout << "Offset " << *offset << " Size: " << size << endl;

	return true;
}

} /* namespace Nova */
