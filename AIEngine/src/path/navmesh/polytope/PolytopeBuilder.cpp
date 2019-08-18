#include <cassert>
#include "UrchinCommon.h"

#include "PolytopeBuilder.h"
#include "path/navmesh/polytope/PolytopePlaneSurface.h"
#include "path/navmesh/polytope/PolytopeTerrainSurface.h"
#include "path/navmesh/polytope/services/TerrainObstacleService.h"

namespace urchin
{

    //static
    const unsigned int PolytopeBuilder::POINT_INDEX_TO_PLANES[6][4] = {
            {0, 2, 3, 1}, //right
            {4, 5, 7, 6}, //left
            {0, 1, 5, 4}, //top
            {3, 2, 6, 7}, //bottom
            {0, 4, 6, 2}, //front
            {1, 3, 7, 5} //back
    };
    const unsigned int PolytopeBuilder::PLANE_INDEX_TO_POINTS[8][3] = {
            {0, 2, 4}, //NTR
            {0, 2, 5}, //FTR
            {0, 3, 4}, //NBR
            {0, 3, 5}, //FBR
            {1, 2, 4}, //NTL
            {1, 2, 5}, //FTL
            {1, 3, 4}, //NBL
            {1, 3, 5} //FBL
    };

    std::vector<std::unique_ptr<Polytope>> PolytopeBuilder::buildExpandedPolytopes(const std::shared_ptr<AIObject> &aiObject, const NavMeshAgent &agent)
    {
        std::vector<std::unique_ptr<Polytope>> expandedPolytopes;

        unsigned int aiShapeIndex = 0;
        for (auto &aiShape : aiObject->getShapes())
        {
            std::string shapeName = aiObject->getShapes().size()==1 ? aiObject->getName() : aiObject->getName() + "[" + std::to_string(aiShapeIndex++) + "]";
            Transform<float> shapeTransform = aiShape->hasLocalTransform() ? aiObject->getTransform() * aiShape->getLocalTransform() : aiObject->getTransform();
            std::unique_ptr<ConvexObject3D<float>> object = aiShape->getShape()->toConvexObject(shapeTransform);
            std::unique_ptr<Polytope> expandedPolytope;

            if (auto box = dynamic_cast<OBBox<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, box, agent);
            } else if (auto capsule = dynamic_cast<Capsule<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, capsule, agent);
            } else if (auto cone = dynamic_cast<Cone<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, cone, agent);
            } else if (auto convexHull = dynamic_cast<ConvexHull3D<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, convexHull, agent);
            } else if (auto cylinder = dynamic_cast<Cylinder<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, cylinder, agent);
            } else if (auto sphere = dynamic_cast<Sphere<float> *>(object.get()))
            {
                expandedPolytope = createExpandedPolytopeFor(shapeName, sphere, agent);
            } else
            {
                throw std::invalid_argument("Shape type not supported by navigation mesh generator: " + std::string(typeid(*object).name()));
            }

            expandedPolytope->setObstacleCandidate(aiObject->isObstacleCandidate());
            expandedPolytopes.push_back(std::move(expandedPolytope));
        }

        return expandedPolytopes;
    }

    std::unique_ptr<Polytope> PolytopeBuilder::buildExpandedPolytope(const std::shared_ptr<AITerrain> &aiTerrain, const std::shared_ptr<NavMeshConfig> &navMeshConfig)
    {
        #ifdef _DEBUG
            assert(MathAlgorithm::isOne(aiTerrain->getTransform().getScale()));
            assert(MathAlgorithm::isOne(aiTerrain->getTransform().getOrientationMatrix().determinant()));
        #endif

        std::vector<std::unique_ptr<PolytopeSurface>> expandedSurfaces;
        TerrainObstacleService terrainObstacleService(aiTerrain->getName(), aiTerrain->getTransform().getPosition(), aiTerrain->getVertices(),
                                                      aiTerrain->getXLength(), aiTerrain->getZLength());
        std::vector<CSGPolygon<float>> selfObstacles = terrainObstacleService.computeSelfObstacles(navMeshConfig->getMaxSlope());
        auto expandedSurface = std::make_unique<PolytopeTerrainSurface>(aiTerrain->getTransform().getPosition(), aiTerrain->getVertices(),
                                                                        aiTerrain->getXLength(), aiTerrain->getZLength(), selfObstacles);
        expandedSurface->setWalkableCandidate(true);
        expandedSurfaces.emplace_back(std::move(expandedSurface));

        auto expandedPolytope = std::make_unique<Polytope>(aiTerrain->getName(), expandedSurfaces);
        expandedPolytope->setWalkableCandidate(true);
        expandedPolytope->setObstacleCandidate(aiTerrain->isObstacleCandidate());
        return expandedPolytope;
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, OBBox<float> *box, const NavMeshAgent &agent) const
    {
        std::vector<Point3<float>> expandedPoints = createExpandedPoints(box, agent);
        std::vector<std::unique_ptr<PolytopeSurface>> expandedPolytopeSurfaces = createExpandedPolytopeSurfaces(expandedPoints);
        return std::make_unique<Polytope>(name, expandedPolytopeSurfaces);
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, Capsule<float> *capsule, const NavMeshAgent &agent) const
    {
        Vector3<float> boxHalfSizes(capsule->getRadius(), capsule->getRadius(), capsule->getRadius());
        boxHalfSizes[capsule->getCapsuleOrientation()] += capsule->getCylinderHeight() / 2.0f;

        OBBox<float> capsuleBox(boxHalfSizes, capsule->getCenterOfMass(), capsule->getOrientation());
        std::unique_ptr<Polytope> polytope = createExpandedPolytopeFor(name, &capsuleBox, agent);
        polytope->setWalkableCandidate(false);

        return polytope;
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, Cone<float> *cone, const NavMeshAgent &agent) const
    {
        Vector3<float> boxHalfSizes(cone->getRadius(), cone->getRadius(), cone->getRadius());
        boxHalfSizes[cone->getConeOrientation()/2] = cone->getHeight() / 2.0f;

        OBBox<float> coneBox(boxHalfSizes, cone->getCenter(), cone->getOrientation());
        std::unique_ptr<Polytope> polytope = createExpandedPolytopeFor(name, &coneBox, agent);
        polytope->setWalkableCandidate(false);

        return polytope;
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, ConvexHull3D<float> *convexHull, const NavMeshAgent &agent) const
    {
        std::map<unsigned int, Plane<float>> expandedPlanes;
        for(const auto &itTriangles : convexHull->getIndexedTriangles())
        {
            const Point3<float> &point1 = convexHull->getConvexHullPoints().at(itTriangles.second.getIndex(0)).point;
            const Point3<float> &point2 = convexHull->getConvexHullPoints().at(itTriangles.second.getIndex(1)).point;
            const Point3<float> &point3 = convexHull->getConvexHullPoints().at(itTriangles.second.getIndex(2)).point;
            expandedPlanes.insert(std::pair<unsigned int, Plane<float>>(itTriangles.first, createExpandedPlane(point1, point2, point3, agent)));
        }

        std::unique_ptr<ConvexHull3D<float>> expandedConvexHull = ResizeConvexHull3DService<float>::instance()->resizeConvexHull(*convexHull, expandedPlanes);

        std::vector<std::unique_ptr<PolytopeSurface>> expandedSurfaces;
        expandedSurfaces.reserve(expandedConvexHull->getIndexedTriangles().size() * 3);
        for(const auto &indexedTriangle : expandedConvexHull->getIndexedTriangles())
        {
            const unsigned int *indices = indexedTriangle.second.getIndices();

            expandedSurfaces.emplace_back(std::make_unique<PolytopePlaneSurface>(std::initializer_list<Point3<float>>({
                expandedConvexHull->getConvexHullPoints().find(indices[0])->second.point,
                expandedConvexHull->getConvexHullPoints().find(indices[1])->second.point,
                expandedConvexHull->getConvexHullPoints().find(indices[2])->second.point
            })));
        }

        std::unique_ptr<Polytope> polytope = std::make_unique<Polytope>(name, expandedSurfaces);
        polytope->setWalkableCandidate(false);

        return polytope;
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, Cylinder<float> *cylinder, const NavMeshAgent &agent) const
    {
        Vector3<float> boxHalfSizes(cylinder->getRadius(), cylinder->getRadius(), cylinder->getRadius());
        boxHalfSizes[cylinder->getCylinderOrientation()] = cylinder->getHeight() / 2.0f;

        OBBox<float> cylinderBox(boxHalfSizes, cylinder->getCenterOfMass(), cylinder->getOrientation());
        std::vector<Point3<float>> expandedPoints = createExpandedPoints(&cylinderBox, agent);

        std::vector<std::unique_ptr<PolytopeSurface>> expandedSurfaces = createExpandedPolytopeSurfaces(expandedPoints);
        for(std::size_t i=0; i<expandedSurfaces.size(); ++i)
        {
            expandedSurfaces[i]->setWalkableCandidate(cylinder->getCylinderOrientation()==i/2);
        }

        return std::make_unique<Polytope>(name, expandedSurfaces);
    }

    std::unique_ptr<Polytope> PolytopeBuilder::createExpandedPolytopeFor(const std::string &name, Sphere<float> *sphere, const NavMeshAgent &agent) const
    {
        OBBox<float> sphereBox(*sphere);
        std::unique_ptr<Polytope> polytope = createExpandedPolytopeFor(name, &sphereBox, agent);
        polytope->setWalkableCandidate(false);

        return polytope;
    }

    /**
     * Return box points. Points are in the following order: NTR, FTR, NBR, FBR, NTL, FTL, NBL, FBL
     */
    std::vector<Point3<float>> PolytopeBuilder::createExpandedPoints(OBBox<float> *box, const NavMeshAgent &navMeshAgent) const
    {
        std::vector<Point3<float>> sortedPoints = box->getPoints();
        std::vector<Plane<float>> sortedExpandedPlanes = createExpandedBoxPlanes(sortedPoints, navMeshAgent);
        return expandBoxPoints(sortedExpandedPlanes);
    }

    /**
     * Returns expanded planes.
     * @param sortedPoints Points in the following order: NTR, FTR, NBR, FBR, NTL, FTL, NBL, FBL
     * @return Expanded planes in the following order: right, left, top, bottom, front, back
     */
    std::vector<Plane<float>> PolytopeBuilder::createExpandedBoxPlanes(const std::vector<Point3<float>> &sortedPoints, const NavMeshAgent &navMeshAgent) const
    {
        std::vector<Plane<float>> expandedPlanes;
        expandedPlanes.reserve(6);

        for(auto pointIndex : POINT_INDEX_TO_PLANES)
        {
            expandedPlanes.emplace_back(createExpandedPlane(sortedPoints[pointIndex[0]], sortedPoints[pointIndex[1]], sortedPoints[pointIndex[2]], navMeshAgent));
        }

        return expandedPlanes;
    }

    Plane<float> PolytopeBuilder::createExpandedPlane(const Point3<float> &p1, const Point3<float> &p2, const Point3<float> &p3, const NavMeshAgent &navMeshAgent) const
    {
        Plane<float> plane(p1, p2, p3);

        float expandDistance = navMeshAgent.computeExpandDistance(plane.getNormal());
        plane.setDistanceToOrigin(plane.getDistanceToOrigin() - expandDistance);

        return plane;
    }

    std::vector<Point3<float>> PolytopeBuilder::expandBoxPoints(const std::vector<Plane<float>> &sortedExpandedPlanes) const
    {
        std::vector<Point3<float>> expandedPoints;
        expandedPoints.reserve(8);

        for(auto planeIndex : PLANE_INDEX_TO_POINTS)
        {
            const Plane<float> &plane0 = sortedExpandedPlanes[planeIndex[0]];
            const Plane<float> &plane1 = sortedExpandedPlanes[planeIndex[1]];
            const Plane<float> &plane2 = sortedExpandedPlanes[planeIndex[2]];

            Vector3<float> n1CrossN2 = plane0.getNormal().crossProduct(plane1.getNormal());
            Vector3<float> n2CrossN3 = plane1.getNormal().crossProduct(plane2.getNormal());
            Vector3<float> n3CrossN1 = plane2.getNormal().crossProduct(plane0.getNormal());

            Point3<float> newPoint = Point3<float>(n2CrossN3 * plane0.getDistanceToOrigin());
            newPoint += Point3<float>(n3CrossN1 * plane1.getDistanceToOrigin());
            newPoint += Point3<float>(n1CrossN2 * plane2.getDistanceToOrigin());
            newPoint *= -1.0 / plane0.getNormal().dotProduct(n2CrossN3);

            expandedPoints.emplace_back(newPoint);
        }

        return expandedPoints;
    }

    /**
     * Return expanded box surfaces. Surfaces are guaranteed to be in the following order: right, left, top, bottom, front, back
     * @param expandedPoints Points in the following order: NTR, FTR, NBR, FBR, NTL, FTL, NBL, FBL
     */
    std::vector<std::unique_ptr<PolytopeSurface>> PolytopeBuilder::createExpandedPolytopeSurfaces(const std::vector<Point3<float>> &expandedPoints) const
    {
        std::vector<std::unique_ptr<PolytopeSurface>> expandedSurfaces;
        expandedSurfaces.reserve(6);

        for(auto pointIndex : POINT_INDEX_TO_PLANES)
        {
            expandedSurfaces.push_back(std::make_unique<PolytopePlaneSurface>(std::initializer_list<Point3<float>>({
                expandedPoints[pointIndex[0]],
                expandedPoints[pointIndex[1]],
                expandedPoints[pointIndex[2]],
                expandedPoints[pointIndex[3]]
            })));
        }

        return expandedSurfaces;
    }
}
