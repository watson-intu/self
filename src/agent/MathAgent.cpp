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


#include "MathAgent.h"
#include "SelfInstance.h"
#include "blackboard/BlackBoard.h"
#include "blackboard/Say.h"

REG_SERIALIZABLE(MathAgent);
RTTI_IMPL(MathAgent, IAgent);

void MathAgent::Serialize(Json::Value & json)
{
	IAgent::Serialize(json);
}

void MathAgent::Deserialize(const Json::Value & json)
{
	IAgent::Deserialize(json);
}

bool MathAgent::OnStart()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->SubscribeToType( "Calculate",
		DELEGATE(MathAgent, OnCalculate, const ThingEvent &, this), TE_ADDED);

	return true;
}

bool MathAgent::OnStop()
{
	SelfInstance * pInstance = SelfInstance::GetInstance();
	if ( pInstance == NULL )
		return false;
	BlackBoard * pBlackboard = pInstance->GetBlackBoard();
	assert( pBlackboard != NULL );

	pBlackboard->UnsubscribeFromType( "Calculate", this);
	return true;
}

void MathAgent::OnCalculate(const ThingEvent & a_ThingEvent)
{
	Calculate::SP spCalculate = DynamicCast<Calculate>(a_ThingEvent.GetIThing());
	if (spCalculate)
	{
		std::vector<double> someVector;
		PopulateVectorFromData(spCalculate->GetData(), spCalculate->GetKey(), someVector);
		if (someVector.size() > 0)
		{
			double calculate = PerformArithmetic(spCalculate->GetArithmetic(), someVector);
			spCalculate->AddChild(Say::SP(new Say(StringUtil::Format("%d", (int)calculate))));
			spCalculate->SetState("COMPLETED");
		}
		else
			Log::Error("MathAgent", "Could not find key specified in data, can not compute!");
	}
}

void MathAgent::PopulateVectorFromData(const std::string & a_Data, const std::string & a_Key, std::vector<double> & a_Vector)
{
	Json::Value root;
	Json::Reader reader;
	bool parsingSuccessful = reader.parse(a_Data, root);
	if (parsingSuccessful)
	{
		if (root.isArray())
		{
			for (unsigned int i = 0; i < root.size(); ++i)
			{
				Json::Value json = root[i];
				if (json.isObject() && json[a_Key].isDouble())
				{
					a_Vector.push_back(json[a_Key].asDouble());
				}
			}
		}
	}
}

double MathAgent::PerformArithmetic(const std::string & a_Arithmetic, std::vector<double> & a_Vector)
{
	double total = 0.0;
	switch (GetMathOp(a_Arithmetic))
	{
	case SUM:
		for (unsigned int i = 0; i < a_Vector.size(); ++i)
			total += a_Vector[i];
		break;
	case SUBTRACT:
		total = a_Vector[0];
		for (unsigned int i = 1; i < a_Vector.size(); ++i)
			total -= a_Vector[i];
		break;
	case DIVIDE:
		total = a_Vector[0];
		for (unsigned int i = 1; i < a_Vector.size(); ++i)
			total /= a_Vector[i];
		break;
	case MULTIPLY:
		total = 1.0;
		for (unsigned int i = 0; i < a_Vector.size(); ++i)
			total *= a_Vector[i];
		break;
	}

	return total;

}

const char * MathAgent::MathOpText(MathOp a_Op)
{
	static const char * TEXT[] =
	{
		"SUM",			// +
		"SUBTRACT",		// -
		"DIVIDE",		// /
		"MULTIPLY"		// *
	};
	int index = (int)a_Op;
	if (index < 0 || index >= sizeof(TEXT) / sizeof(TEXT[0]))
		return "?";
	return TEXT[index];
}

MathAgent::MathOp MathAgent::GetMathOp(const std::string & a_Op)
{
	for (int i = 0; i < LAST_EO; ++i)
		if (a_Op.compare(MathOpText((MathOp)i)) == 0)
			return (MathOp)i;

	// default to EQ
	return SUM;
}