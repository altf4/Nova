//============================================================================
// Name        : SerializationHelper.h
// Copyright   : DataSoft Corporation 2011-2013
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
//============================================================================

#ifndef SERIALIZATIONHELPER_H_
#define SERIALIZATIONHELPER_H_

#include <string>
#include <string.h>
#include <exception>
#include <arpa/inet.h>

namespace Nova
{

class serializationException : public std::exception
{
	  virtual const char *what() const throw()
	  {
	    return "Error when serializing or deserializing";
	  }
};

// Serializes (simple memcpy) a chunk of data with checking on buffer bounds
//    buf             : Pointer to buffer location where serialized data should go
//    offset          : Offset from the buffer (will be incremented by SerializeChunk)
//    dataToSerialize : Pointer to data to serialize
//    size            : Size of the data to serialize
//   maxBufferSize   : Max size of the buffer, throw exception if serialize goes past this
inline bool SerializeChunk(u_char* buf, uint32_t *offset, char const* dataToSerialize, uint32_t size, uint32_t maxBufferSize)
{
	if(*offset + size > maxBufferSize)
	{
		throw serializationException();
		return false;
	}
	::memcpy(buf+*offset, dataToSerialize, size);
	*offset += size;
	return true;
}

inline bool SerializeString(u_char* buf, uint32_t *offset, std::string stringToSerialize, uint32_t maxBufferSize)
{
	uint32_t sizeOfString = stringToSerialize.size();
	SerializeChunk(buf, offset, (char*)&sizeOfString, sizeof sizeOfString, maxBufferSize);
	return SerializeChunk(buf, offset, stringToSerialize.c_str(), sizeOfString, maxBufferSize);
}

// Deserializes (simple memcpy) a chunk of data with checking on buffer bounds
//    buf             : Pointer to buffer location where serialized data should go
//    offset          : Offset from the buffer (will be incremented by DeserializeChunk)
//    dataToSerialize : Pointer to data to deserialize
//    size            : Size of the data to deserialize
//   maxBufferSize   : Max size of the buffer, throw exception if deserialize goes past this
inline bool DeserializeChunk(u_char *buf, uint32_t *offset, char *deserializeTo, uint32_t size, uint32_t maxBufferSize)
{
	if(*offset + size > maxBufferSize)
	{
		throw serializationException();
		return false;
	}

	::memcpy(deserializeTo, buf+*offset, size);
	*offset += size;
	return true;
}

inline std::string DeserializeString(u_char *buf, uint32_t *offset, uint32_t maxBufferSize)
{
	uint32_t sizeOfString;
	DeserializeChunk(buf, offset,(char*)&sizeOfString, sizeof sizeOfString, maxBufferSize);

	std::string str;

	if (sizeOfString != 0) {
		char *stringBytes = new char[sizeOfString + 1];
		DeserializeChunk(buf, offset, stringBytes, sizeOfString, maxBufferSize);
		stringBytes[sizeOfString] = '\0';
		str = std::string(stringBytes);
		delete[] stringBytes;
	}
	return str;
}

// Serializes a hash map
//    buf             : Pointer to buffer location where serialized data should go
//    offset          : Offset from the buffer (will be incremented by SerializeChunk)
//    dataToSerialize : Reference to the hash map
//    nullValue       : Any map entries with this value will be skipped (eg, 0 for bins that store a count)
//   maxBufferSize    : Max size of the buffer, throw exception if serialize goes past this
template <typename TableType, typename KeyType, typename ValueType>
inline void SerializeHashTable(u_char *buf, uint32_t *offset, TableType& dataToSerialize, KeyType nullValue, uint32_t maxBufferSize)
{
	typename TableType::iterator it = dataToSerialize.begin();
	typename TableType::iterator last = dataToSerialize.end();

	uint32_t count = 0;
	while(it != last)
	{
		if(it->first != nullValue)
		{
			count++;
		}
		it++;
	}

	it = dataToSerialize.begin();

	//The size of the Table
	SerializeChunk(buf, offset, (char*)&count, sizeof count, maxBufferSize);

	while(it != last)
	{
		if(it->first != nullValue)
		{
			SerializeChunk(buf, offset, (char*)&it->first, sizeof it->first, maxBufferSize);
			SerializeChunk(buf, offset, (char*)&it->second, sizeof it->second, maxBufferSize);
		}
		it++;
	}
}

template <typename TableType, typename KeyType, typename ValueType>
inline uint32_t GetSerializeHashTableLength(TableType& dataToSerialize, KeyType nullValue)
{
	typename TableType::iterator it = dataToSerialize.begin();
	typename TableType::iterator last = dataToSerialize.end();

	uint32_t count = 0;
	uint32_t length = 0;

	while(it != last)
	{
		if(it->first != nullValue)
		{
			length += sizeof it->first;
			length += sizeof it->second;
			count++;
		}
		it++;
	}

	//The size of the Table
	length += sizeof count;

	return length;
}

// Deserializes a hash map
//    buf             : Pointer to buffer location where deserialized data should go
//    offset          : Offset from the buffer (will be incremented by DeserializeChunk)
//    dataToSerialize : Reference to the hash map
//    nullValue       : Any map entries with this value will be skipped (eg, 0 for bins that store a count)
//   maxBufferSize    : Max size of the buffer, throw exception if deserialize goes past this
template <typename TableType, typename KeyType, typename ValueType>
inline void DeserializeHashTable(u_char *buf, uint32_t *offset, TableType& deserializeTo, uint32_t maxBufferSize)
{
	uint32_t tableSize = 0;
	ValueType value;
	KeyType key;

	DeserializeChunk(buf, offset, (char*)&tableSize, sizeof tableSize, maxBufferSize);

	for(uint32_t i = 0; i < tableSize;)
	{
		DeserializeChunk(buf, offset, (char*)&key, sizeof key, maxBufferSize);
		DeserializeChunk(buf, offset, (char*)&value, sizeof value, maxBufferSize);

		deserializeTo[key] = value;
		i++;
	}
}

}
#endif /* SERIALIZATIONHELPER_H_ */
