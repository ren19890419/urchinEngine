#include <GL/glew.h>
#include <stdexcept>
#include <string>

#include "SceneManager.h"

#define START_FPS 1000.0f //high number of FPS to avoid pass through the ground at startup
#define RENDERER_3D 0
#define GUI_RENDERER 1
#define REFRESH_RATE_FPS 0.35f

namespace urchin
{
	
	SceneManager::SceneManager() :
			sceneWidth(500),
			sceneHeight(500),
            activeRenderers(),
            previousFps(),
			fps(START_FPS),
			fpsForDisplay(START_FPS)
	{
		//initialize GL
		initializeGl();

		//initialize fps
		previousFps.fill(START_FPS);
        indexFps = previousFps.size();
		previousTime = std::chrono::high_resolution_clock::now();

		//renderer
		for (auto &activeRenderer : activeRenderers)
		{
			activeRenderer = nullptr;
		}
	}

	SceneManager::~SceneManager()
	{
		for (auto &renderer3d : renderers3d)
		{
			delete renderer3d;
		}

		for (auto &guiRenderer : guiRenderers)
		{
			delete guiRenderer;
		}

		Profiler::getInstance("3d")->log();
	}

	void SceneManager::initializeGl()
	{
		//initialization Glew
		GLenum err = glewInit();
		if(err != GLEW_OK)
		{
			throw std::runtime_error((char *)glewGetErrorString(err));
		}

		//check OpenGL version supported
		if(!glewIsSupported("GL_VERSION_4_4"))
		{
			throw std::runtime_error("OpenGL version 4.4 is required but it's not supported on this environment.");
		}

		//check OpenGL context version
		int majorVersionContext = 0, minorVersionContext = 0;
		glGetIntegerv(GL_MAJOR_VERSION, &majorVersionContext);
		glGetIntegerv(GL_MINOR_VERSION, &minorVersionContext);

		if((majorVersionContext*100 + minorVersionContext*10) < 440)
		{
			std::ostringstream ossMajorVersionContext;
			ossMajorVersionContext << majorVersionContext;

			std::ostringstream ossMinorVersionContext;
			ossMinorVersionContext << minorVersionContext;

			throw std::runtime_error("OpenGL context version required: 4.4, current version: " + ossMajorVersionContext.str() + "." + ossMinorVersionContext.str() + ".");
		}

		//initialization OpenGl
		glEnable(GL_DEPTH_TEST);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}

	void SceneManager::onResize(unsigned int sceneWidth, unsigned int sceneHeight)
	{
		if(sceneWidth!=0 && sceneHeight!=0)
		{
			//scene properties
			this->sceneWidth = sceneWidth;
			this->sceneHeight = sceneHeight;
			glViewport(0, 0, sceneWidth, sceneHeight);

			//renderer
			for (auto &activeRenderer : activeRenderers)
			{
				if(activeRenderer)
				{
					activeRenderer->onResize(sceneWidth, sceneHeight);
				}
			}
		}
	}

	unsigned int SceneManager::getSceneWidth() const
	{
		return sceneWidth;
	}

	unsigned int SceneManager::getSceneHeight() const
	{
		return sceneHeight;
	}

	float SceneManager::getFps() const
	{
		return (fps==0.0f ? START_FPS : fps);
	}

	unsigned int SceneManager::getFpsForDisplay()
	{
		static float timeElapse = 0.0f;
		timeElapse += getDeltaTime();

		if(timeElapse > REFRESH_RATE_FPS)
		{ //refresh fps every REFRESH_RATE_FPS_MS
			fpsForDisplay = std::lround(fps);
			timeElapse = 0.0f;
		}

		return fpsForDisplay;
	}

	float SceneManager::getDeltaTime() const
	{
		return 1.0f / getFps();
	}

	void SceneManager::computeFps()
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float timeInterval = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(currentTime - previousTime).count());

        previousFps[indexFps%3] = 1000000.0f / timeInterval;
		fps = (previousFps.at(indexFps%3)*9 + previousFps.at((indexFps-1)%3)*3 + previousFps.at((indexFps-2)%3)) / 13.0f;

        indexFps++;
        previousTime = currentTime;
	}

	Renderer3d *SceneManager::newRenderer3d(bool enable)
	{
		auto *renderer3d = new Renderer3d();
		renderers3d.push_back(renderer3d);

		if(enable)
		{
			enableRenderer3d(renderer3d);
		}
		return renderer3d;
	}

	void SceneManager::enableRenderer3d(Renderer3d *renderer3d)
	{
		if(activeRenderers[RENDERER_3D] && activeRenderers[RENDERER_3D]!=renderer3d)
		{
			activeRenderers[RENDERER_3D]->onDisable();
		}

		activeRenderers[RENDERER_3D] = renderer3d;
		if(renderer3d)
		{
			renderer3d->onResize(sceneWidth, sceneHeight);
		}
	}

	void SceneManager::removeRenderer3d(Renderer3d *renderer3d)
	{
		if(activeRenderers[RENDERER_3D] == renderer3d)
		{
			activeRenderers[RENDERER_3D] = nullptr;
		}

		renderers3d.erase(std::remove(renderers3d.begin(), renderers3d.end(), renderer3d), renderers3d.end());
		delete renderer3d;
	}

	Renderer3d *SceneManager::getActiveRenderer3d() const
	{
		return dynamic_cast<Renderer3d *>(activeRenderers[RENDERER_3D]);
	}

	GUIRenderer *SceneManager::newGUIRenderer(bool enable)
	{
		auto *guiRenderer = new GUIRenderer();
		guiRenderers.push_back(guiRenderer);

		if(enable)
		{
			enableGUIRenderer(guiRenderer);
		}
		return guiRenderer;
	}

	void SceneManager::enableGUIRenderer(GUIRenderer *guiRenderer)
	{
		if(activeRenderers[GUI_RENDERER] && activeRenderers[GUI_RENDERER]!=guiRenderer)
		{
			activeRenderers[GUI_RENDERER]->onDisable();
		}

		activeRenderers[GUI_RENDERER] = guiRenderer;
		if(guiRenderer)
		{
			guiRenderer->onResize(sceneWidth, sceneHeight);
		}
	}

	void SceneManager::removeGUIRenderer(GUIRenderer *guiRenderer)
	{
		if(activeRenderers[GUI_RENDERER] == guiRenderer)
		{
			activeRenderers[GUI_RENDERER] = nullptr;
		}

		guiRenderers.erase(std::remove(guiRenderers.begin(), guiRenderers.end(), guiRenderer), guiRenderers.end());
		delete guiRenderer;
	}

	GUIRenderer *SceneManager::getActiveGUIRenderer() const
	{
		return dynamic_cast<GUIRenderer *>(activeRenderers[GUI_RENDERER]);
	}

	TextureManager *SceneManager::getTextureManager() const
	{
		return TextureManager::instance();
	}

	bool SceneManager::onKeyDown(unsigned int key)
	{
		for(int i=NUM_RENDERER-1; i>=0; --i)
		{
			if(activeRenderers[i] && !activeRenderers[i]->onKeyDown(key))
			{
				return false;
			}
		}
		return true;
	}

	bool SceneManager::onKeyUp(unsigned int key)
	{
		for(int i=NUM_RENDERER-1; i>=0; --i)
		{
			if(activeRenderers[i] && !activeRenderers[i]->onKeyUp(key))
			{
				return false;
			}
		}
		return true;
	}

	bool SceneManager::onChar(unsigned int character)
	{
		for(int i=NUM_RENDERER-1; i>=0; --i)
		{
			if(activeRenderers[i] && !activeRenderers[i]->onChar(character))
			{
				return false;
			}
		}
		return true;
	}

	bool SceneManager::onMouseMove(int mouseX, int mouseY)
	{
		for(int i=NUM_RENDERER-1; i>=0; --i)
		{
			if(activeRenderers[i] && !activeRenderers[i]->onMouseMove(mouseX, mouseY))
			{
				return false;
			}
		}
		return true;
	}

	void SceneManager::display()
	{
		ScopeProfiler profiler("3d", "sceneMgrDisplay");

		//fps
		computeFps();
		float dt = getDeltaTime();

		//renderer
		for (auto &activeRenderer : activeRenderers)
		{
			if(activeRenderer)
			{
				activeRenderer->display(dt);
			}
		}

		GLenum err;
		while((err = glGetError()) != GL_NO_ERROR)
		{
		    Logger::logger().logError("OpenGL error detected: " + std::to_string(err));
		}
	}

}
