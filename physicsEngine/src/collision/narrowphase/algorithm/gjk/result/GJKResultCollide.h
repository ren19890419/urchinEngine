#ifndef URCHINENGINE_GJKRESULTCOLLIDE_H
#define URCHINENGINE_GJKRESULTCOLLIDE_H

#include <stdexcept>
#include "UrchinCommon.h"

#include "GJKResult.h"
#include "collision/narrowphase/algorithm/utils/Simplex.h"

namespace urchin
{

	template<class T> class GJKResultCollide : public GJKResult<T>
	{
		public:
			explicit GJKResultCollide(const Simplex<T> &);

			bool isValidResult() const override;

			bool isCollide() const override;
			T getSeparatingDistance() const override;
			const Point3<T> &getClosestPointA() const override;
			const Point3<T> &getClosestPointB() const override;

			const Simplex<T> &getSimplex() const override;

		private:
			Simplex<T> simplex;
	};

}

#endif
