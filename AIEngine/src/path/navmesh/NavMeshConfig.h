#ifndef ENGINE_NAVMESHCONFIG_H
#define ENGINE_NAVMESHCONFIG_H

#include "UrchinCommon.h"

namespace urchin
{

	class NavMeshConfig
	{
		public:
			NavMeshConfig(float, float);

			Cylinder<float> getAgent() const;

			void setMaxSlope(float);
			float getMaxSlope() const;

		private:
			float agentHeight;
			float agentRadius;

			float maxSlopeInRadian;
	};

}

#endif