#include <GL/glew.h>
#include <GL/gl.h>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <limits>
#include <map>
#include <string>

#include "ShadowManager.h"
#include "scene/renderer3d/model/octreefilter/ProduceShadowFilter.h"
#include "scene/renderer3d/light/omnidirectional/OmnidirectionalLight.h"
#include "scene/renderer3d/light/sun/SunLight.h"
#include "scene/renderer3d/filter/TextureFilter.h"
#include "scene/renderer3d/filter/TextureFilterBuilder.h"
#include "utils/shader/ShaderManager.h"
#include "utils/shader/TokenReplacerShader.h"

#define DEFAULT_NUMBER_SHADOW_MAPS 2
#define DEFAULT_SHADOW_MAP_RESOLUTION 2048
#define DEFAULT_VIEWING_SHADOW_DISTANCE 75.0

namespace urchin
{

	ShadowManager::ShadowManager(LightManager *lightManager, OctreeManager<Model> *modelOctreeManager) :
			sceneWidth(0),
			sceneHeight(0),
			percentageUniformSplit(ConfigService::instance()->getFloatValue("shadow.frustumUniformSplitAgainstLogSplit")),
			shadowMapResolution(DEFAULT_SHADOW_MAP_RESOLUTION),
			nbShadowMaps(DEFAULT_NUMBER_SHADOW_MAPS),
			viewingShadowDistance(DEFAULT_VIEWING_SHADOW_DISTANCE),
			isInitialized(false),
			shadowModelDisplayer(nullptr),
			lightManager(lightManager),
			modelOctreeManager(modelOctreeManager),
			shadowUniform(nullptr),
			shadowModelUniform(nullptr),
			frustumDistance(0.0),
			depthSplitDistanceLoc(0),
			lightsLocation(nullptr)
	{
		switch(ConfigService::instance()->getIntValue("shadow.depthComponent"))
		{
			case 16:
				depthComponent = GL_DEPTH_COMPONENT16;
				break;
			case 24:
				depthComponent = GL_DEPTH_COMPONENT24;
				break;
			case 32:
				depthComponent = GL_DEPTH_COMPONENT32;
				break;
			default:
				throw std::domain_error("Unsupported value for parameter 'shadow.depthComponent'.");
		}
	}

	ShadowManager::~ShadowManager()
	{
		for(std::map<const Light *, ShadowData *>::iterator it = shadowDatas.begin(); it!=shadowDatas.end(); ++it)
		{
			removeShadowMaps(it->first);

			delete it->second;
		}

		if(lightsLocation!=nullptr)
		{
			for(unsigned int i=0;i<lightManager->getMaxLights();++i)
			{
				delete [] lightsLocation[i].mLightProjectionViewLoc;
			}
			delete [] lightsLocation;
		}

		delete shadowModelDisplayer;
		delete shadowUniform;
		delete shadowModelUniform;
	}

	void ShadowManager::initialize(unsigned int shaderID)
	{
		if(isInitialized)
		{
			throw std::runtime_error("Shadow manager is already initialized.");
		}

		//scene information
		std::map<std::string, std::string> geometryTokens, fragmentTokens;
		geometryTokens["MAX_VERTICES"] = std::to_string(3*nbShadowMaps);
		geometryTokens["NUMBER_SHADOW_MAPS"] = std::to_string(nbShadowMaps);
		shadowModelDisplayer = new ModelDisplayer(ModelDisplayer::DEPTH_ONLY_MODE);
		shadowModelDisplayer->setCustomGeometryShader("modelShadowMap.geo", geometryTokens);
		shadowModelDisplayer->setCustomFragmentShader("modelShadowMap.frag", fragmentTokens);
		shadowModelDisplayer->initialize();

		shadowUniform = new ShadowUniform();
		shadowUniform->setProjectionMatricesLocation(shadowModelDisplayer->getUniformLocation("projectionMatrix"));
		shadowModelDisplayer->setCustomUniform(shadowUniform);

		shadowModelUniform = new ShadowModelUniform();
		shadowModelUniform->setFirstSplitLocation(shadowModelDisplayer->getUniformLocation("firstSplit"));
		shadowModelUniform->setLastSplitLocation(shadowModelDisplayer->getUniformLocation("lastSplit"));
		shadowModelDisplayer->setCustomModelUniform(shadowModelUniform);

		lightManager->addObserver(this, LightManager::ADD_LIGHT);
		lightManager->addObserver(this, LightManager::REMOVE_LIGHT);

		//shadow information
		depthSplitDistanceLoc = glGetUniformLocation(shaderID, "depthSplitDistance");

		//light information
		lightsLocation = new LightLocation[lightManager->getMaxLights()];
		std::ostringstream shadowMapTextureLocName, mLightProjectionViewLocName;
		for(unsigned int i=0;i<lightManager->getMaxLights();++i)
		{
			//texture unit
			lightsLocation[i].shadowMapTextureUnits = i + 3; //shadow map texture start to GL_TEXTURE3

			//depth shadow texture
			shadowMapTextureLocName.str("");
			shadowMapTextureLocName << "lightsInfo[" << i << "].shadowMapTex";
			lightsLocation[i].shadowMapTexLoc = glGetUniformLocation(shaderID, shadowMapTextureLocName.str().c_str());

			//light projection matrices
			lightsLocation[i].mLightProjectionViewLoc = new int[nbShadowMaps];
			for(unsigned int j=0; j<nbShadowMaps; ++j)
			{
				mLightProjectionViewLocName.str("");
				mLightProjectionViewLocName << "lightsInfo[" << i << "].mLightProjectionView[" << j << "]";
				lightsLocation[i].mLightProjectionViewLoc[j] = glGetUniformLocation(shaderID, mLightProjectionViewLocName.str().c_str());
			}
		}

		isInitialized=true;
	}

	void ShadowManager::onResize(int width, int height)
	{
		sceneWidth = width;
		sceneHeight = height;

		for(std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.begin(); it!=shadowDatas.end(); ++it)
		{
			updateViewMatrix(it->first);
		}
	}

	void ShadowManager::onCameraProjectionUpdate(const Camera *const camera)
	{
		this->projectionMatrix = camera->getProjectionMatrix();

		onFrustumUpdate(camera->getFrustum());
	}

	void ShadowManager::notify(Observable *observable, int notificationType)
	{
		if(dynamic_cast<LightManager *>(observable))
		{
			Light *light = lightManager->getLastUpdatedLight();
			switch(notificationType)
			{
				case LightManager::ADD_LIGHT:
				{
					light->addObserver(this, Light::PRODUCE_SHADOW);

					if(light->isProduceShadow())
					{
						addShadowLight(light);
					}
					break;
				}
				case LightManager::REMOVE_LIGHT:
				{
					light->removeObserver(this, Light::PRODUCE_SHADOW);

					if(light->isProduceShadow())
					{
						removeShadowLight(light);
					}
					break;
				}
			}
		}else if(Light *light = dynamic_cast<Light *>(observable))
		{
			switch(notificationType)
			{
				case Light::MOVE:
				{
					updateViewMatrix(light);
					break;
				}
				case Light::PRODUCE_SHADOW:
				{
					if(light->isProduceShadow())
					{
						addShadowLight(light);
					}else
					{
						removeShadowLight(light);
					}
					break;
				}
			}
		}
	}

	void ShadowManager::setShadowMapResolution(unsigned int shadowMapResolution)
	{
		this->shadowMapResolution = shadowMapResolution;

		updateShadowLights();
	}

	unsigned int ShadowManager::getShadowMapResolution() const
	{
		return shadowMapResolution;
	}

	void ShadowManager::setNumberShadowMaps(unsigned int nbShadowMaps)
	{
		if(isInitialized)
		{
			throw std::runtime_error("Impossible to change number of shadow maps once the scene initialized.");
		}

		this->nbShadowMaps = nbShadowMaps;
	}

	unsigned int ShadowManager::getNumberShadowMaps() const
	{
		return nbShadowMaps;
	}

	/**
	 * @param viewingShadowDistance Viewing shadow distance. If negative, shadow will be displayed until the far plane.
	 */
	void ShadowManager::setViewingShadowDistance(float viewingShadowDistance)
	{
		this->viewingShadowDistance = viewingShadowDistance;
	}

	/**
	 * @return Viewing shadow distance. If negative, shadow will be displayed until the far plane.
	 */
	float ShadowManager::getViewingShadowDistance() const
	{
		return viewingShadowDistance;
	}

	const ShadowData &ShadowManager::getShadowData(const Light *const light) const
	{
		std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.find(light);
		if(it==shadowDatas.end())
		{
			throw std::runtime_error("No shadow data found for this light.");
		}

		const ShadowData *shadowData = it->second;
		return *shadowData;
	}

	/**
	 * @return All visible models from all lights
	 */
	std::set<Model *> ShadowManager::getVisibleModels()
	{
		std::set<Model *> visibleModels;
		for(std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.begin(); it!=shadowDatas.end(); ++it)
		{
			for(unsigned int i=0; i<nbShadowMaps; ++i)
			{
				const std::set<Model *> &visibleModelsForLightInFrustumSplit = it->second->getFrustumShadowData(i)->getModels();
				visibleModels.insert(visibleModelsForLightInFrustumSplit.begin(), visibleModelsForLightInFrustumSplit.end());
			}
		}

		return visibleModels;
	}

	void ShadowManager::addShadowLight(Light *const light)
	{
		light->addObserver(this, Light::MOVE);

		shadowDatas[light] = new ShadowData(light, nbShadowMaps);

		createShadowMaps(light);
		updateViewMatrix(light);
	}

	void ShadowManager::removeShadowLight(Light *const light)
	{
		light->removeObserver(this, Light::MOVE);

		removeShadowMaps(light);

		delete shadowDatas[light];
		shadowDatas.erase(light);
	}

	/**
	 * Updates lights data which producing shadows
	 */
	void ShadowManager::updateShadowLights()
	{
		std::vector<Light *> visibleLights = lightManager->getVisibleLights();
		for(std::vector<Light *>::const_iterator itLights = visibleLights.begin(); itLights!=visibleLights.end(); ++itLights)
		{
			if((*itLights)->isProduceShadow())
			{
				removeShadowLight(*itLights);
				addShadowLight(*itLights);
			}
		}
	}

	void ShadowManager::onFrustumUpdate(const Frustum<float> &frustum)
	{
		frustumDistance = frustum.computeFarDistance() + frustum.computeNearDistance();

		splitFrustum(frustum);
		updateShadowLights();
	}

	void ShadowManager::updateViewMatrix(const Light *const light)
	{
		ShadowData *shadowData = shadowDatas[light];

		if(light->hasParallelBeams())
		{ //sun light
			Vector3<float> lightDirection = light->getDirections()[0];

			const Vector3<float> &f = lightDirection.normalize();
			const Vector3<float> &s = f.crossProduct(Vector3<float>(0.0, 1.0, 0.0)).normalize();
			const Vector3<float> &u = s.crossProduct(f).normalize();
			Matrix4<float> M(
				s[0],	s[1],	s[2],	0,
				u[0],	u[1],	u[2],	0,
				-f[0],	-f[1],	-f[2],	0,
				0,		0,		0,		1);

			Matrix4<float> eye;
			eye.buildTranslation(lightDirection.X, lightDirection.Y, lightDirection.Z);
			Matrix4<float> mViewShadow = M * eye;

			shadowData->setLightViewMatrix(mViewShadow);
		}else
		{
			throw std::runtime_error("Shadow currently not supported on omnidirectional light.");
		}
	}

	/**
	 * Updates frustum shadow data (models, shadow caster/receiver box, projection matrix)
	 */
	void ShadowManager::updateFrustumShadowData(const Light *const light, ShadowData *const shadowData)
	{
		if(light->hasParallelBeams())
		{ //sun light
			for(unsigned int i=0; i<splittedFrustum.size(); ++i)
			{
				AABBox<float> aabboxSceneIndependent = createSceneIndependentBox(splittedFrustum[i], light, shadowData->getLightViewMatrix());
				OBBox<float> obboxViewSpace = shadowData->getLightViewMatrix().inverse() * OBBox<float>(aabboxSceneIndependent);

				const std::set<Model *> models = modelOctreeManager->getOctreeablesIn(obboxViewSpace, ProduceShadowFilter());
				shadowData->getFrustumShadowData(i)->setModels(models);

				//TODO train support shadow not always display
				AABBox<float> aabboxSceneDependent = createSceneDependentBox(aabboxSceneIndependent, models, shadowData->getLightViewMatrix());
				shadowData->getFrustumShadowData(i)->setShadowCasterReceiverBox(aabboxSceneDependent);
				shadowData->getFrustumShadowData(i)->setLightProjectionMatrix(aabboxSceneDependent.toProjectionMatrix());
			}
		}else
		{
			throw std::runtime_error("Shadow not supported on omnidirectional light.");
		}
	}

	/**
	 * @return Box in light space containing shadow caster and receiver (scene independent)
	 */
	AABBox<float> ShadowManager::createSceneIndependentBox(const Frustum<float> &splittedFrustum, const Light *const light,
			const Matrix4<float> &lightViewMatrix) const
	{
		const Frustum<float> &frustumLightSpace = lightViewMatrix * splittedFrustum;

		//determine point belonging to shadow caster/receiver box
		std::vector<Point3<float>> shadowReceiverAndCasterVertex;
		shadowReceiverAndCasterVertex.reserve(16);
		float nearCapZ = computeNearZForSceneIndependentBox(frustumLightSpace);
		for(std::vector<Point3<float>>::const_iterator it = frustumLightSpace.getFrustumPoints().begin(); it != frustumLightSpace.getFrustumPoints().end(); ++it)
		{
			//add shadow receiver points
			shadowReceiverAndCasterVertex.push_back(*it);

			//add shadow caster points
			shadowReceiverAndCasterVertex.push_back(Point3<float>(it->X, it->Y, nearCapZ));
		}

		//build shadow receiver/caster bounding box from points
		Point3<float> min(shadowReceiverAndCasterVertex[0]);
		Point3<float> max(shadowReceiverAndCasterVertex[0]);
		for(unsigned int i=1; i<shadowReceiverAndCasterVertex.size(); ++i)
		{
			if(min.X > shadowReceiverAndCasterVertex[i].X)
				min.X = shadowReceiverAndCasterVertex[i].X;

			if(min.Y > shadowReceiverAndCasterVertex[i].Y)
				min.Y = shadowReceiverAndCasterVertex[i].Y;

			if(min.Z > shadowReceiverAndCasterVertex[i].Z)
				min.Z = shadowReceiverAndCasterVertex[i].Z;

			if(max.X < shadowReceiverAndCasterVertex[i].X)
				max.X = shadowReceiverAndCasterVertex[i].X;

			if(max.Y < shadowReceiverAndCasterVertex[i].Y)
				max.Y = shadowReceiverAndCasterVertex[i].Y;

			if(max.Z < shadowReceiverAndCasterVertex[i].Z)
				max.Z = shadowReceiverAndCasterVertex[i].Z;
		}

		return AABBox<float>(min, max);
	}

	float ShadowManager::computeNearZForSceneIndependentBox(const Frustum<float> &splittedFrustumLightSpace) const
	{
		float nearestPointFromLight = splittedFrustumLightSpace.getFrustumPoints()[0].Z;
		for(unsigned int i=1; i<splittedFrustumLightSpace.getFrustumPoints().size(); ++i)
		{
			if(splittedFrustumLightSpace.getFrustumPoints()[i].Z > nearestPointFromLight)
			{
				nearestPointFromLight = splittedFrustumLightSpace.getFrustumPoints()[i].Z;
			}
		}
		return nearestPointFromLight + frustumDistance;
	}

	/**
	 * @return Box in light space containing shadow caster and receiver (scene dependent)
	 */
	AABBox<float> ShadowManager::createSceneDependentBox(const AABBox<float> &aabboxSceneIndependent, const std::set<Model *> &models,
			const Matrix4<float> &lightViewMatrix) const
	{
		std::set<Model *>::iterator it = models.begin();
		AABBox<float> aabboxSceneDependent;
		if(it!=models.end())
		{
			aabboxSceneDependent = lightViewMatrix * (*it)->getAABBox();
			++it;

			for(;it!=models.end(); ++it)
			{
				aabboxSceneDependent = aabboxSceneDependent.merge(lightViewMatrix * (*it)->getAABBox());
			}
		}

		Point3<float> cutMin(
			aabboxSceneDependent.getMin().X<aabboxSceneIndependent.getMin().X ? aabboxSceneIndependent.getMin().X : aabboxSceneDependent.getMin().X,
			aabboxSceneDependent.getMin().Y<aabboxSceneIndependent.getMin().Y ? aabboxSceneIndependent.getMin().Y : aabboxSceneDependent.getMin().Y,
			aabboxSceneIndependent.getMin().Z); //shadow can be projected outside the box: value cannot be capped
//TODO try to reduce this box...

		Point3<float> cutMax(
			aabboxSceneDependent.getMax().X>aabboxSceneIndependent.getMax().X ? aabboxSceneIndependent.getMax().X : aabboxSceneDependent.getMax().X,
			aabboxSceneDependent.getMax().Y>aabboxSceneIndependent.getMax().Y ? aabboxSceneIndependent.getMax().Y : aabboxSceneDependent.getMax().Y,
			aabboxSceneDependent.getMax().Z>aabboxSceneIndependent.getMax().Z ? aabboxSceneIndependent.getMax().Z : aabboxSceneDependent.getMax().Z);

		float step = 3.0f;
		cutMin.X = (cutMin.X<0.0f) ? cutMin.X-(step+fmod(cutMin.X, step)) : cutMin.X-fmod(cutMin.X, step);
		cutMin.Y = (cutMin.Y<0.0f) ? cutMin.Y-(step+fmod(cutMin.Y, step)) : cutMin.Y-fmod(cutMin.Y, step);
		cutMin.Z = (cutMin.Z<0.0f) ? cutMin.Z-(step+fmod(cutMin.Z, step)) : cutMin.Z-fmod(cutMin.Z, step);
		cutMax.X = (cutMax.X<0.0f) ? cutMax.X-fmod(cutMax.X, step) : cutMax.X+(step-fmod(cutMax.X, step));
		cutMax.Y = (cutMax.Y<0.0f) ? cutMax.Y-fmod(cutMax.Y, step) : cutMax.Y+(step-fmod(cutMax.Y, step));
		cutMax.Z = (cutMax.Z<0.0f) ? cutMax.Z-fmod(cutMax.Z, step) : cutMax.Z+(step-fmod(cutMax.Z, step));

		return AABBox<float>(cutMin, cutMax);
	}

	void ShadowManager::splitFrustum(const Frustum<float> &frustum)
	{
		splittedDistance.clear();
		splittedFrustum.clear();

		float near = frustum.computeNearDistance();
		float far = viewingShadowDistance;
		if(viewingShadowDistance < 0.0f)
		{
			far = frustum.computeFarDistance();
		}

		float previousSplitDistance = near;

		for(unsigned int i=1; i<=nbShadowMaps; ++i)
		{
			float uniformSplit = near + (far - near) * (i/static_cast<float>(nbShadowMaps));
			float logarithmicSplit = near * pow(far/near, i/static_cast<float>(nbShadowMaps));

			float splitDistance = (percentageUniformSplit * uniformSplit) + ((1.0 - percentageUniformSplit) * logarithmicSplit);

			splittedDistance.push_back(splitDistance);
			splittedFrustum.push_back(frustum.splitFrustum(previousSplitDistance, splitDistance));

			previousSplitDistance = splitDistance;
		}
	}

	void ShadowManager::createShadowMaps(const Light *const light)
	{
		//frame buffer object
		unsigned int fboID;
		glGenFramebuffers(1, &fboID);
		glBindFramebuffer(GL_FRAMEBUFFER, fboID);

		shadowDatas[light]->setFboID(fboID);

		//textures for shadow map: depth texture && shadow map texture (variance shadow map)
		GLenum fboAttachments[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, fboAttachments);
		glReadBuffer(GL_NONE);

		unsigned int textureIDs[2];
		glGenTextures(2, &textureIDs[0]);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textureIDs[0]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, depthComponent, shadowMapResolution, shadowMapResolution, nbShadowMaps, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, textureIDs[0], 0);

		glBindTexture(GL_TEXTURE_2D_ARRAY, textureIDs[1]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG32F, shadowMapResolution, shadowMapResolution, nbShadowMaps, 0, GL_RG, GL_FLOAT, 0);
		glFramebufferTexture(GL_FRAMEBUFFER, fboAttachments[0], textureIDs[1], 0);

		shadowDatas[light]->setDepthTextureID(textureIDs[0]);
		shadowDatas[light]->setShadowMapTextureID(textureIDs[1]);

		//shadow map filters
		std::shared_ptr<TextureFilter> downSample2xFilter = std::make_shared<TextureFilterBuilder>()
				->filterType(TextureFilterBuilder::DOWN_SAMPLE)
				->textureSize(shadowMapResolution/2, shadowMapResolution/2)
				->textureType(GL_TEXTURE_2D_ARRAY)
				->textureNumberLayer(nbShadowMaps)
				->textureInternalFormat(GL_RG32F)  //TODO use 24 bits ??
				->textureAnisotropy(1.0f)
				->textureFormat(GL_RG)
				->build();
		shadowDatas[light]->setDownSample2xFilter(downSample2xFilter);

		std::shared_ptr<TextureFilter> downSample4xFilter = std::make_shared<TextureFilterBuilder>()
				->filterType(TextureFilterBuilder::DOWN_SAMPLE)
				->textureSize(shadowMapResolution/4, shadowMapResolution/4)
				->textureType(GL_TEXTURE_2D_ARRAY)
				->textureAnisotropy(1.0f)
				->textureNumberLayer(nbShadowMaps)
				->textureInternalFormat(GL_RG32F)
				->textureFormat(GL_RG)
				->build();
		shadowDatas[light]->setDownSample4xFilter(downSample4xFilter);

		std::shared_ptr<TextureFilter> blurFilter = std::make_shared<TextureFilterBuilder>()
				->filterType(TextureFilterBuilder::BLUR)
				->textureSize(shadowMapResolution/2, shadowMapResolution/2)
				->textureType(GL_TEXTURE_2D_ARRAY)
				->textureAnisotropy(1.0f) //TODO review anisotropy
				->textureNumberLayer(nbShadowMaps)
				->textureInternalFormat(GL_RG32F)
				->textureFormat(GL_RG)
				->build();
		shadowDatas[light]->setBlurFilter(blurFilter);
	}

	void ShadowManager::removeShadowMaps(const Light *const light)
	{
		unsigned int depthTextureID = shadowDatas[light]->getDepthTextureID();
		glDeleteTextures(1, &depthTextureID);

		unsigned int shadowMapTextureID = shadowDatas[light]->getShadowMapTextureID();
		glDeleteTextures(1, &shadowMapTextureID);

		unsigned int frameBufferObjectID = shadowDatas[light]->getFboID();
		glDeleteFramebuffers(1, &frameBufferObjectID);
	}

	void ShadowManager::updateVisibleModels(const Frustum<float> &frustum)
	{
		splitFrustum(frustum);

		for(std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.begin(); it!=shadowDatas.end(); ++it)
		{
			updateFrustumShadowData(it->first, it->second);
		}
	}

	void ShadowManager::updateShadowMaps()
	{
		glBindTexture(GL_TEXTURE_2D, 0);

		for(std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.begin(); it!=shadowDatas.end(); ++it)
		{
			ShadowData *shadowData = it->second;

			glViewport(0, 0, shadowMapResolution, shadowMapResolution);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowData->getFboID());
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

			shadowUniform->setUniformData(shadowData);
			shadowModelUniform->setModelUniformData(shadowData);

			shadowModelDisplayer->setModels(shadowData->retrieveModels());
			shadowModelDisplayer->display(shadowData->getLightViewMatrix());

			shadowData->getDownSample2xFilter()->applyOn(shadowData->getShadowMapTextureID());
			//shadowData->getDownSample4xFilter()->applyOn(shadowData->getDownSample2xFilter()->getTextureID());
			shadowData->getBlurFilter()->applyOn(shadowData->getDownSample2xFilter()->getTextureID());
		}

		glViewport(0, 0, sceneWidth, sceneHeight);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}

	void ShadowManager::loadShadowMaps(const Matrix4<float> &viewMatrix)
	{
		int i = 0;
		std::vector<Light *> visibleLights = lightManager->getVisibleLights();
		for(std::vector<Light *>::const_iterator itLights = visibleLights.begin(); itLights!=visibleLights.end(); ++itLights)
		{
			if((*itLights)->isProduceShadow())
			{
				std::map<const Light *, ShadowData *>::const_iterator it = shadowDatas.find(*itLights);
				const ShadowData *shadowData = it->second;

				glActiveTexture(GL_TEXTURE0 + lightsLocation[i].shadowMapTextureUnits);
				//glBindTexture(GL_TEXTURE_2D_ARRAY, shadowData->getBlurFilter()->getTextureID());
				glBindTexture(GL_TEXTURE_2D_ARRAY, shadowData->getShadowMapTextureID());
				glUniform1i(lightsLocation[i].shadowMapTexLoc, lightsLocation[i].shadowMapTextureUnits);

				for(unsigned int j=0; j<nbShadowMaps; ++j)
				{
					glUniformMatrix4fv(lightsLocation[i].mLightProjectionViewLoc[j], 1, false, (float *)(shadowData->getFrustumShadowData(j)->getLightProjectionMatrix() * shadowData->getLightViewMatrix()));
				}
			}
			++i;
		}

		float depthSplitDistance[nbShadowMaps];
		for(unsigned int i=0; i<nbShadowMaps; ++i)
		{
			float currSplitDistance = splittedDistance[i];
			depthSplitDistance[i] = (projectionMatrix(2, 2)*-currSplitDistance + projectionMatrix(2, 3)) / (currSplitDistance);
		}

		glUniform1fv(depthSplitDistanceLoc, nbShadowMaps, depthSplitDistance);
	}

}
