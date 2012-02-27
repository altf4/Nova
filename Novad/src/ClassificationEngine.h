//============================================================================
// Name        : ClassificationEngine.h
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
// Description : Classifies suspects as either hostile or benign then takes appropriate action
//============================================================================

#ifndef CLASSIFICATIONENGINE_H_
#define CLASSIFICATIONENGINE_H_

namespace Nova{
//The main thread for the Classification Engine
// ptr - Unused pointer required by pthread
// returns - Does not return (main loop)
void *ClassificationEngineMain(void *ptr);
}

#endif /* CLASSIFICATIONENGINE_H_ */
