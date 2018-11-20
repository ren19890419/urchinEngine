#ifndef URCHINENGINE_COLLISIONHEIGHTFIELDSHAPE_H
#define URCHINENGINE_COLLISIONHEIGHTFIELDSHAPE_H

#include <memory>
#include <vector>
#include "UrchinCommon.h"

#include "shape/CollisionShape3D.h"
#include "shape/CollisionConcaveShape.h"
#include "utils/pool/FixedSizePool.h"

namespace urchin
{

    class CollisionHeightfieldShape : public CollisionShape3D, public CollisionConcaveShape
    {
        public:
            CollisionHeightfieldShape(std::vector<Point3<float>>, unsigned int, unsigned int);
            ~CollisionHeightfieldShape() override;

            CollisionShape3D::ShapeType getShapeType() const override;
            std::shared_ptr<ConvexShape3D<float>> getSingleShape() const override;
            const std::vector<Point3<float>> &getVertices() const;
            unsigned int getXLength() const;
            unsigned int getZLength() const;

            std::shared_ptr<CollisionShape3D> scale(float) const override;

            AABBox<float> toAABBox(const PhysicsTransform &) const override;
            CollisionConvexObject3D *toConvexObject(const PhysicsTransform &) const override;

            Vector3<float> computeLocalInertia(float) const override;
            float getMaxDistanceToCenter() const override;
            float getMinDistanceToCenter() const override;

            CollisionShape3D *clone() const override;

            const std::vector<CollisionTriangleShape> &findTrianglesInAABBox(const AABBox<float> &) const override;

        private:
            class TriangleShapeDeleter
            {
                public:
                    explicit TriangleShapeDeleter(FixedSizePool<TriangleShape3D<float>> *);
                    void operator()(TriangleShape3D<float> *);

                private:
                    FixedSizePool<TriangleShape3D<float>> *const triangleShapesPool;
            };

            std::unique_ptr<BoxShape<float>> buildLocalAABBox() const;

            std::vector<Point3<float>> vertices;
            unsigned int xLength;
            unsigned int zLength;

            std::unique_ptr<BoxShape<float>> localAABBox;

            mutable AABBox<float> lastAABBox;
            mutable PhysicsTransform lastTransform;

            mutable std::vector<CollisionTriangleShape> trianglesInAABBox;
            FixedSizePool<TriangleShape3D<float>> *triangleShapesPool;
    };

}

#endif
