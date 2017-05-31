/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/


#ifndef RECTANGULAR_WINDOW_H
#define RECTANGULAR_WINDOW_H

#include "IWindowFunction.h"

class RectangularWindow : public IWindowFunction
{
public:
	float Value(int length, int index) 
	{
		return 1.0f;
	}
};

#endif
