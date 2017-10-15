#include <vector>

#include "AIWorldGenerator.h"

namespace urchin
{

	std::shared_ptr<AIWorld> AIWorldGenerator::generate(const std::list<SceneObject *> &sceneObjects)
	{
		std::shared_ptr<AIWorld> aiWorld = std::make_shared<AIWorld>();
		for(auto sceneObject : sceneObjects)
		{
			RigidBody *rigidBody = sceneObject->getRigidBody();
			std::vector<LocalizedShape> localizedShapes = extractionLocalizedShapes(rigidBody);

			unsigned int localizedShapesId = 0;
			for(const auto &localizedShape : localizedShapes)
			{
                std::string aiObjectName = sceneObject->getName();
				if(localizedShapes.size()>1)
                {
                    aiObjectName = sceneObject->getName() + "[" + std::to_string(localizedShapesId++) + "]";
				}
                AIObject aiObject(aiObjectName, localizedShape.shape, localizedShape.worldTransform);
                aiWorld->addObject(aiObject);
			}
		}

		return aiWorld;
	}

	std::vector<LocalizedShape> AIWorldGenerator::extractionLocalizedShapes(RigidBody *rigidBody)
	{
		std::vector<LocalizedShape> localizedShapes;
		if(rigidBody!=nullptr)
		{
			std::shared_ptr<const CollisionShape3D> scaledCollisionShape3D = rigidBody->getScaledShape();
			if(scaledCollisionShape3D->getShapeCategory()==CollisionShape3D::COMPOUND)
			{
				std::shared_ptr<const CollisionCompoundShape> collisionCompoundShape = std::dynamic_pointer_cast<const CollisionCompoundShape>(scaledCollisionShape3D);
				for(const auto &collisionLocalizedShape : collisionCompoundShape->getLocalizedShapes())
				{
					PhysicsTransform worldPhysicsTransform = PhysicsTransform(rigidBody->getTransform().getPosition(), rigidBody->getTransform().getOrientation()) * collisionLocalizedShape->transform;
					LocalizedShape localizedShape;
					localizedShape.shape = collisionLocalizedShape->shape->getSingleShape();
					localizedShape.worldTransform = worldPhysicsTransform.toTransform();

					localizedShapes.push_back(localizedShape);
				}
			}else if(scaledCollisionShape3D->getShapeCategory()==CollisionShape3D::CONVEX)
			{
				Transform<float> unscaledTransform = rigidBody->getTransform();
				unscaledTransform.setScale(1.0); //scale not needed because shape is already scaled.

				LocalizedShape localizedShape;
				localizedShape.shape = scaledCollisionShape3D->getSingleShape();
				localizedShape.worldTransform = unscaledTransform;

				localizedShapes.push_back(localizedShape);
			} else
			{
				throw std::invalid_argument("Unknown shape category: " + std::to_string(scaledCollisionShape3D->getShapeCategory()));
			}
		}

		return localizedShapes;
	}

}
