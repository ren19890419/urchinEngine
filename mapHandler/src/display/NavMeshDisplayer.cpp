#include "NavMeshDisplayer.h"

namespace urchin
{

    NavMeshDisplayer::NavMeshDisplayer(AIManager *aiManager, Renderer3d *renderer3d) :
        aiManager(aiManager),
        renderer3d(renderer3d),
        loadedNavMeshId(std::numeric_limits<unsigned int>::max())
    {
            
    }
    
    NavMeshDisplayer::~NavMeshDisplayer()
    {
        clearDisplay();
    }

    void NavMeshDisplayer::display()
    {
        NavMesh navMesh = aiManager->getNavMeshGenerator()->copyLastGeneratedNavMesh();

        if(loadedNavMeshId != navMesh.getUpdateId())
        {
            clearDisplay();

            std::vector<Point3<float>> triangleMeshPoints;
            std::vector<Point3<float>> quadJumpPoints;

            for (const auto &navPolygon : navMesh.getPolygons())
            {
                for (const auto &triangle : navPolygon->getTriangles())
                {
                    triangleMeshPoints.emplace_back(navPolygon->getPoints()[triangle->getIndex(0)]);
                    triangleMeshPoints.emplace_back(navPolygon->getPoints()[triangle->getIndex(1)]);
                    triangleMeshPoints.emplace_back(navPolygon->getPoints()[triangle->getIndex(2)]);

                    for (const auto &link : triangle->getLinks())
                    {
                        if (link->getLinkType() == NavLinkType::JUMP)
                        {
                            LineSegment3D<float> constrainedStartEdge = link->getLinkConstraint()->computeSourceJumpEdge(triangle->computeEdge(link->getSourceEdgeIndex()));
                            quadJumpPoints.emplace_back(constrainedStartEdge.getA());
                            quadJumpPoints.emplace_back(constrainedStartEdge.getB());

                            LineSegment3D<float> endEdge = link->getTargetTriangle()->computeEdge(link->getLinkConstraint()->getTargetEdgeIndex());
                            LineSegment3D<float> constrainedEndEdge(endEdge.closestPoint(constrainedStartEdge.getA()), endEdge.closestPoint(constrainedStartEdge.getB()));
                            quadJumpPoints.emplace_back(constrainedEndEdge.getA());
                            quadJumpPoints.emplace_back(constrainedEndEdge.getB());
                        }
                    }
                }
            }

            auto *meshModel = new TrianglesModel(toDisplayPoints(triangleMeshPoints, 0.02f));
            addNavMeshModel(meshModel, GeometryModel::FILL, Vector3<float>(0.0, 0.0, 1.0));

            auto *meshWireframeModel = new TrianglesModel(toDisplayPoints(triangleMeshPoints, 0.025f));
            addNavMeshModel(meshWireframeModel, GeometryModel::WIREFRAME, Vector3<float>(0.5, 0.5, 1.0));

            auto *jumpModel = new QuadsModel(quadJumpPoints);
            addNavMeshModel(jumpModel, GeometryModel::FILL, Vector3<float>(0.5, 0.0, 0.5));

            loadedNavMeshId = navMesh.getUpdateId();
        }
    }

    void NavMeshDisplayer::clearDisplay()
    {
        for (auto navMeshModel : navMeshModels)
        {
            renderer3d->getGeometryManager()->removeGeometry(navMeshModel);
            delete navMeshModel;
        }
        navMeshModels.clear();
    }

    std::vector<Point3<float>> NavMeshDisplayer::toDisplayPoints(const std::vector<Point3<float>> &points, float yElevation) const
    {
        std::vector<Point3<float>> displayPoints;
        displayPoints.reserve(points.size());

        for(const auto &point : points)
        {
            displayPoints.emplace_back(Point3<float>(point.X, point.Y + yElevation, point.Z));
        }

        return displayPoints;
    }

    void NavMeshDisplayer::addNavMeshModel(GeometryModel *model, GeometryModel::PolygonMode polygonMode, const Vector3<float> &color)
    {
        model->setLineSize(4.0);
        model->setBlendMode(GeometryModel::ONE_MINUS_SRC_ALPHA);
        model->setColor(color.X, color.Y, color.Z, 0.5);
        model->setPolygonMode(polygonMode);
        navMeshModels.push_back(model);
        renderer3d->getGeometryManager()->addGeometry(model);
    }
}
