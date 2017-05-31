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


#include "IVertex.h"
#include "IGraph.h"

RTTI_IMPL( IVertex, ISerializable );

bool IVertex::SortEdges( const EdgeSP & a1, const EdgeSP & a2 )
{
	return a1->GetTime() > a2->GetTime();
}

bool IVertex::FindOutEdges( const std::string & a_EdgeLabel, 
	EdgeList & a_Edges )
{
	for( size_t i=0;i<m_OutEdges.size(); ++i)
	{
		const IEdge::SP & spEdge = m_OutEdges[i];
		if ( spEdge->GetLabel().compare( a_EdgeLabel ) != 0 )
			continue;
		a_Edges.push_back( spEdge );
	}

	std::sort( a_Edges.begin(), a_Edges.end(), SortEdges );
	return a_Edges.size() > 0;
}

bool IVertex::FindInEdges( const std::string & a_EdgeLabel,
	EdgeList & a_Edges )
{
	for( size_t i=0;i<m_InEdges.size(); ++i)
	{
		const IEdge::SP & spEdge = m_InEdges[i];
		if ( spEdge->GetLabel().compare( a_EdgeLabel ) != 0 )
			continue;
		a_Edges.push_back( spEdge );
	}

	std::sort( a_Edges.begin(), a_Edges.end(), SortEdges );
	return a_Edges.size() > 0;
}
