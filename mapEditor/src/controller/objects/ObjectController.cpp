#include <stdexcept>
#include <memory>
#include "Urchin3dEngine.h"

#include "ObjectController.h"
#include "panel/objects/bodyshape/support/DefaultBodyShapeCreator.h"

namespace urchin
{

	ObjectController::ObjectController() :
            AbstractController()
	{

	}

	std::list<const SceneObject *> ObjectController::getSceneObjects() const
	{
		const std::list<SceneObject *> &sceneObjects = getMapHandler()->getMap()->getSceneObjects();
		std::list<const SceneObject *> constSceneObjects;
		constSceneObjects.insert(constSceneObjects.begin(), sceneObjects.begin(), sceneObjects.end());

		return constSceneObjects;
	}

	const SceneObject *ObjectController::findSceneObjectByBodyId(const std::string &bodyId) const
	{
		for(auto sceneObject : getSceneObjects())
		{
			if(sceneObject->getRigidBody() && sceneObject->getRigidBody()->getId()==bodyId)
			{
				return sceneObject;
			}
		}

		return nullptr;
	}

	void ObjectController::addSceneObject(SceneObject *sceneObject)
	{
		getMapHandler()->getMap()->addSceneObject(sceneObject);

		markModified();
	}

	void ObjectController::removeSceneObject(const SceneObject *constSceneObject)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		getMapHandler()->getMap()->removeSceneObject(sceneObject);

		markModified();
	}

	void ObjectController::cloneSceneObject(SceneObject *newSceneObject, const SceneObject *toCloneSceneObject)
	{
        Model *toCloneModel = toCloneSceneObject->getModel();
        auto *model = new Model(*toCloneModel);
        Point3<float> shiftPosition(0.5f, 0.0f, 0.0f);
        model->setPosition(model->getTransform().getPosition() + shiftPosition);
        newSceneObject->setModel(model);

        RigidBody *toCloneRigidBody = toCloneSceneObject->getRigidBody();
        if(toCloneRigidBody)
        {
            auto *rigidBody = new RigidBody(*toCloneRigidBody);
			rigidBody->setId(newSceneObject->getName());
			rigidBody->setTransform(model->getTransform());
            newSceneObject->setupInteractiveBody(rigidBody);
        }

        addSceneObject(newSceneObject);
	}

	void ObjectController::createDefaultBody(const SceneObject *constSceneObject)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);

		std::string bodyId = constSceneObject->getName();
		Transform<float> modelTransform = constSceneObject->getModel()->getTransform();
		std::shared_ptr<const CollisionShape3D> bodyShape = DefaultBodyShapeCreator(constSceneObject).createDefaultBodyShape(CollisionShape3D::ShapeType::BOX_SHAPE, false);

		auto *rigidBody = new RigidBody(bodyId, modelTransform, bodyShape);
		sceneObject->setupInteractiveBody(rigidBody);

		markModified();
	}

	void ObjectController::changeBodyShape(const SceneObject *constSceneObject, CollisionShape3D::ShapeType shapeType)
	{
		std::shared_ptr<const CollisionShape3D> newCollisionShape = DefaultBodyShapeCreator(constSceneObject).createDefaultBodyShape(shapeType, false);

		updateSceneObjectPhysicsShape(constSceneObject, newCollisionShape);
	}

	void ObjectController::removeBody(const SceneObject *constSceneObject)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		sceneObject->setupInteractiveBody(nullptr);

		markModified();
	}

	const SceneObject *ObjectController::updateSceneObjectTransform(const SceneObject *constSceneObject, const Transform<float> &transform)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		Model *model = sceneObject->getModel();

		model->setTransform(transform);
		if(sceneObject->getRigidBody())
		{
			sceneObject->getRigidBody()->setTransform(transform);
		}

		markModified();
		return sceneObject;
	}

	const SceneObject *ObjectController::updateSceneObjectFlags(const SceneObject *constSceneObject, bool produceShadow)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		Model *model = sceneObject->getModel();

		model->setProduceShadow(produceShadow);

		markModified();
		return sceneObject;
	}

	const SceneObject *ObjectController::updateSceneObjectPhysicsProperties(const SceneObject *constSceneObject, float mass, float restitution,
			float friction, float rollingFriction, float linearDamping, float angularDamping, const Vector3<float> &linearFactor,
			const Vector3<float> &angularFactor)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		RigidBody *rigidBody = sceneObject->getRigidBody();

		rigidBody->setMass(mass);
		rigidBody->setRestitution(restitution);
		rigidBody->setFriction(friction);
		rigidBody->setRollingFriction(rollingFriction);

		rigidBody->setDamping(linearDamping, angularDamping);

		rigidBody->setLinearFactor(linearFactor);
		rigidBody->setAngularFactor(angularFactor);

		markModified();
		return sceneObject;
	}

	const SceneObject *ObjectController::updateSceneObjectPhysicsShape(const SceneObject *constSceneObject, const std::shared_ptr<const CollisionShape3D>& newCollisionShape)
	{
		SceneObject *sceneObject = findSceneObject(constSceneObject);
		RigidBody *rigidBody = sceneObject->getRigidBody();

		std::string bodyId = constSceneObject->getName();
		Transform<float> modelTransform = constSceneObject->getModel()->getTransform();
		auto *newRigidBody = new RigidBody(bodyId, modelTransform, newCollisionShape);

		newRigidBody->setMass(rigidBody->getMass());
		newRigidBody->setRestitution(rigidBody->getRestitution());
		newRigidBody->setFriction(rigidBody->getFriction());
		newRigidBody->setRollingFriction(rigidBody->getRollingFriction());

		sceneObject->setupInteractiveBody(newRigidBody);

		markModified();
		return sceneObject;
	}

	SceneObject *ObjectController::findSceneObject(const SceneObject *constSceneObject)
	{
		const std::list<SceneObject *> &sceneObjects = getMapHandler()->getMap()->getSceneObjects();
		auto it = std::find(sceneObjects.begin(), sceneObjects.end(), constSceneObject);

		if(it!=sceneObjects.end())
		{
			return *it;
		}

		throw std::invalid_argument("Impossible to find scene object: " + constSceneObject->getName());
	}

}
