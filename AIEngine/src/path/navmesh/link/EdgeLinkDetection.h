#ifndef URCHINENGINE_EDGELINKDETECTION_H
#define URCHINENGINE_EDGELINKDETECTION_H

#include "UrchinCommon.h"

#include "path/navmesh/link/EdgeLinkResult.h"

namespace urchin
{

    class EdgeLinkDetection
    {
        public:
            explicit EdgeLinkDetection(float);

            EdgeLinkResult detectLink(const LineSegment3D<float> &, const LineSegment3D<float> &) const;

        private:
            float jumpMaxLength;
            float jumpMaxSquareLength;

            bool pointsAreEquals(const Point3<float> &, const Point3<float> &) const;
            bool isCollinearLines(const Line3D<float> &, const Line3D<float> &) const;
            bool isTouchingCollinearEdges(const LineSegment3D<float> &, const LineSegment3D<float> &, float &, float &) const;

            bool canJumpThatFar(const Point3<float> &, const Point3<float> &) const;
            bool isProperJumpDirection(const LineSegment3D<float> &, const LineSegment3D<float> &, const Point3<float> &, const Point3<float> &) const;
    };

}

#endif