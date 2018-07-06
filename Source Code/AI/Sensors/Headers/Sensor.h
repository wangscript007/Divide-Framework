/*�Copyright 2009-2012 DIVIDE-Studio�*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AI_SENSOR_H_
#define _AI_SENSOR_H_

#include "resource.h"
#include <boost/any.hpp>

enum SENSOR_TYPE{
	NONE = 0,
	VISUAL_SENSOR = 1,
	AUDIO_SENSOR = 2,
	COMMUNICATION_SENSOR = 3
};

class Sensor{
public:
	Sensor(SENSOR_TYPE type){_type = type;}
	virtual void updatePosition(const vec3<F32>& newPosition) {_position = newPosition;}
	/// return the coordinates at which the sensor is found (or the entity it's attached to)
	vec3<F32>& getSpatialPosition()                    {return _position;}  
	SENSOR_TYPE getSensorType() {return _type;}
	
protected:

	vec3<F32> _position;
	vec2<F32> _range; ///< min/max
	SENSOR_TYPE _type;
};

#endif 