//============================================================================
// Name        : SerializationHelper.h
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

#ifndef SERIALIZATIONHELPER_H_
#define SERIALIZATIONHELPER_H_

#include "sys/types.h"

#include <string>
#include <exception>

namespace Nova
{

class serializationException : public std::exception {
	  virtual const char* what() const throw()
	  {
	    return "Error when serializing or deserializing";
	  }
};

// Generic serialize/deserialize methods
bool SerializeChunk(u_char* buf, uint32_t* offset, char* dataToSerialize, uint32_t size, uint32_t maxBufferSize);
bool DeserializeChunk(u_char* buf, uint32_t* offset, char* deserializeTo, uint32_t size, uint32_t maxBufferSize);


} /* namespace Nova */
#endif /* SERIALIZATIONHELPER_H_ */
