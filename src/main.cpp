#include <SDL3/SDL_events.h>
#include <cmath>
#include <glm/ext/matrix_float4x4.hpp>
#include <ostream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#elif __linux__
    #include <sched.h>
#endif
#include <thread>

#include <volk.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

#include <GL/glew.h>

#include <cstdlib>
#include <vector>
#include <cstdint>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <chrono>

#include "vulkan/common.hpp"
#include "vulkan/vk_debug.hpp"
#include "vulkan/vk_device.hpp"
#include "vulkan/vk_frames.hpp"
#include "vulkan/vk_objects.hpp"
#include "vulkan/vk_command.hpp"
#include "vulkan/vk_sync.hpp"

#include "openGL/gl_shader.hpp"
#include "openGL/gl_loadGLTF.hpp"

class EngineBase
{
    protected:
        const bool FORCE_OPENGL = true;
        const bool EAT_MOUSE = false;

        using clock = std::chrono::steady_clock;
        clock::time_point songStartTime = clock::now();

        const float TPS = 180.0f;
        const float TICK_RATE = 1.0f / TPS;
        const clock::duration UPDATE_DELTA = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(TICK_RATE));

        std::string modelPath = "models/vedal987/vedal987.gltf";

        SDL_Window* window;
        std::atomic<bool> running = true;

        std::atomic<uint32_t> frameCount = 0;
        char titleBuffer[64];

        void setThreadAffinityAndPriority(const int core_id)
        {
            #ifdef _WIN32
                HANDLE hThread = GetCurrentThread();
                SetThreadAffinityMask(hThread, 1 << 1);
                SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
            #elif __linux__
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(core_id, &cpuset);

                const pthread_t thread = pthread_self();
                pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

                sched_param sch_params;
                sch_params.sched_priority = sched_get_priority_max(SCHED_RR);
                pthread_setschedparam(thread, SCHED_RR, &sch_params);
            #endif
        }
        void pollEvents()
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                    break;
                }
            }
        }
        void calculateFramerate(clock::time_point currentTime)
        {
            static clock::time_point lastTime = clock::now();
            double delta = std::chrono::duration<double>(currentTime - lastTime).count();

            if (delta >= 1.0)
            {
                float fps = (float)frameCount / delta;

                std::snprintf(titleBuffer, 64, "FPS: %f", fps);
                SDL_SetWindowTitle(window, titleBuffer);
                frameCount = 0;
                lastTime = currentTime;
            }
        }
};
/*
class VulkanEngine: EngineBase
{
    public:
        bool initVulkan()
        {
            if (FORCE_OPENGL) return false;

            initWindow();
            volkInitialize();

            if (enableValidationLayers) debugManager.init();
            deviceManager.createInstance(window, debugManager.debugCreateInfo);
            if (enableValidationLayers) debugManager.setupDebugMessenger(deviceManager.instance);
            if (!deviceManager.init())
            {
                cleanInstance();
                return false;
            }

            objectManager.init(deviceManager.physicalDevice, deviceManager.device, deviceManager.indices, deviceManager.graphicsQueue, modelPaths, playerModelFile, skyboxVertices, skyboxPaths);
            frameManager.init(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport, objectManager.descriptorSetLayouts);

            commandManager.init(deviceManager.device, deviceManager.indices.graphicsFamily.value(), frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.object3DGraphicsPipeline, frameManager.object3DPipelineLayout, frameManager.skyboxGraphicsPipeline, frameManager.skyboxPipelineLayout, objectManager.skyboxPositionBuffer, frameManager.renderPass, objectManager.descriptorSets, objectManager.models, objectManager.player);
            syncManager.createSyncObjects(deviceManager.device);

            createRenderthread();
            mainLoop();
            cleanAll();
            cleanInstance();
            return true;
        }

    private:
        DebugManager debugManager;
        DeviceManager deviceManager;
        FrameManager frameManager;
        ObjectManager objectManager;
        CommandManager commandManager;
        SyncManager syncManager;

        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);
            window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_VULKAN);

            if (EAT_MOUSE) SDL_SetWindowRelativeMouseMode(window, true);
        }
        void recreateSwapChain()
        {
            vkDeviceWaitIdle(deviceManager.device);

            deviceManager.reinit();
            frameManager.reinit(deviceManager.physicalDevice, deviceManager.device, window, deviceManager.surface, deviceManager.indices, deviceManager.graphicsQueue, deviceManager.swapChainSupport);

            vkFreeCommandBuffers(deviceManager.device, commandManager.commandPool, static_cast<uint32_t>(commandManager.commandBuffers.size()), commandManager.commandBuffers.data());
            commandManager.createCommandBuffers(deviceManager.device, frameManager.swapChainImages.size(), frameManager.swapChainFramebuffers, frameManager.swapChainExtent, frameManager.object3DGraphicsPipeline, frameManager.object3DPipelineLayout, frameManager.skyboxGraphicsPipeline, frameManager.skyboxPipelineLayout, objectManager.skyboxPositionBuffer, frameManager.renderPass, objectManager.descriptorSets, objectManager.models, objectManager.player);
        }

        std::thread renderThread;

        void mainLoop()
        {
            setThreadAffinityAndPriority(0);
            clock::time_point nextTime = clock::now();

            while (running)
            {
                clock::time_point now = clock::now();
                nextTime += UPDATE_DELTA;

                pollEvents();

                while (nextTime <= now)
                {
                    //update(TICK_RATE);
                    nextTime += UPDATE_DELTA;
                }

                calculateFramerate(now);
                std::this_thread::sleep_until(nextTime);
            }
        }

        VkFence fence;
        VkSemaphore imgAvailable;
        VkSemaphore imgRendered = VK_NULL_HANDLE;
        uint32_t imageIndex;
        uint32_t currentFrame = 0;

        VkSemaphoreSubmitInfo waitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkCommandBufferSubmitInfo cmdBufInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO
        };
        VkSemaphoreSubmitInfo signalInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT
        };
        const VkSubmitInfo2 submitInfo
        {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount   = 1,
            .pWaitSemaphoreInfos      = &waitInfo,
            .commandBufferInfoCount   = 1,
            .pCommandBufferInfos      = &cmdBufInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos    = &signalInfo,
        };
        const VkPresentInfoKHR presentInfo
        {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imgRendered,
            .swapchainCount = 1,
            .pSwapchains = &frameManager.swapChain,
            .pImageIndices = &imageIndex,
            .pResults = nullptr
        };
        void createRenderthread()
        {
            renderThread = std::thread([this]()
            {
                setThreadAffinityAndPriority(1);

                while (running)
                {
                    fence = syncManager.inFlightFences[currentFrame];
                    imgAvailable = syncManager.imageAvailableSemaphores[currentFrame];

                    vkWaitForFences(deviceManager.device, 1, &fence, VK_TRUE, UINT64_MAX);
                    if (!acquireImage()) continue;;
                    vkResetFences(deviceManager.device, 1, &fence);

                    submitQueue();
                    presentImg();

                    currentFrame = ++currentFrame % MAX_FRAMES_IN_FLIGHT;
                    frameCount++;
                }
            });
        }
        bool acquireImage()
        {
            VkResult result = vkAcquireNextImageKHR(deviceManager.device, frameManager.swapChain, UINT64_MAX, imgAvailable, VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                recreateSwapChain();
                return false;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("failed to acquire swap chain image!");
            }
            return true;
        }
        void submitQueue()
        {
            imgRendered = syncManager.renderFinishedSemaphores[imageIndex];
            cmdBufInfo.commandBuffer = commandManager.commandBuffers[imageIndex];
            waitInfo.semaphore = imgAvailable;
            signalInfo.semaphore = imgRendered;
            vkQueueSubmit2(deviceManager.graphicsQueue, 1, &submitInfo, fence);
        }
        void presentImg()
        {
            VkResult result = vkQueuePresentKHR(deviceManager.presentQueue, &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
            {
                recreateSwapChain();
            }
            else
            {
                vk_check(result, "failed to present swap chain image!");
            }
        }

        void cleanAll()
        {
            vkDeviceWaitIdle(deviceManager.device);
            renderThread.join();

            syncManager.cleanupSyncObjects(deviceManager.device);
            frameManager.cleanupSwapChain(deviceManager.device);
            frameManager.cleanupPipeline(deviceManager.device);
            objectManager.destroyAll(deviceManager.device);

            vkDestroyCommandPool(deviceManager.device, commandManager.commandPool, nullptr);
            vkDestroyDevice(deviceManager.device, nullptr);
        }
        void cleanInstance()
        {
            vkDestroySurfaceKHR(deviceManager.instance, deviceManager.surface, nullptr);
            if (enableValidationLayers) debugManager.destroyDebugUtilsMessengerEXT(deviceManager.instance, nullptr);
            vkDestroyInstance(deviceManager.instance, nullptr);

            SDL_DestroyWindow(window);
            SDL_Quit();
        }
};
*/

class OpenGLEngine: EngineBase
{
    public:
        OpenGLEngine()
        {
            initWindow();
            createRenderthread();
            mainLoop();
        }
        ~OpenGLEngine()
        {
            renderThread.join();
        }

    private:
        void initWindow()
        {
            SDL_Init(SDL_INIT_VIDEO);

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            window = SDL_CreateWindow("...", WIDTH, HEIGHT, SDL_WINDOW_OPENGL);

            if (EAT_MOUSE) SDL_SetWindowRelativeMouseMode(window, true);

            glContext = SDL_GL_CreateContext(window);
            SDL_GL_MakeCurrent(window, NULL);
        }

        SDL_GLContext glContext;
        std::thread renderThread;

        GLuint voxelSSBO;
        GLuint triangleSSBO;
        uint32_t numTriangles;

        void mainLoop()
        {
            setThreadAffinityAndPriority(0);
            clock::time_point nextTime = clock::now();

            while (running)
            {
                clock::time_point now = clock::now();
                nextTime += UPDATE_DELTA;

                pollEvents();

                calculateFramerate(now);
                std::this_thread::sleep_until(nextTime);
            }
        }

        void createRenderthread()
        {
            renderThread = std::thread([this]()
            {
                setThreadAffinityAndPriority(1);
                if (!SDL_GL_MakeCurrent(window, glContext)) {
                    std::cerr << "Failed to make context current: " << SDL_GetError() << std::endl;
                    return;
                }
                createGLEWContext();

                int shader = makeShader("shaders/openGL/shader3D.vs", "shaders/openGL/shader3D.fs");
                GLint loc = glGetUniformLocation(shader, "helixAngle");
                GLint DeltaLoc = glGetUniformLocation(shader, "deltaAngle");
                int computeShader = makeComputeShader("shaders/openGL/computeShader.comp");
                GLint triCountLoc = glGetUniformLocation(computeShader, "numTriangles");

                GlModel model = GlModel(modelPath.c_str());
                setupSSBO(model.triangles);
                
                glUseProgram(shader);
                glUniform1f(DeltaLoc, 0.02230716677f);
                GLuint dummyVAO;
                glGenVertexArrays(1, &dummyVAO);

                glUseProgram(computeShader);
                glUniform1ui(triCountLoc, numTriangles);

                while (running)
                {
                    uint32_t zero = 0;
                    glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelSSBO);
                    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

                    glUseProgram(computeShader);
                    glDispatchCompute(ceil(570/32.0), ceil(1140/32.0), 1);
                    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                    for (int i = 0; i < 5; ++i)
                    {
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        glBindVertexArray(dummyVAO);
                        glUseProgram(shader);
                        float elapsedSeconds = SDL_GetTicks() / 1000.0f;
                        glUniform1f(loc, 15.0f * 2.0f * 3.1415926535f * elapsedSeconds);
                        glDrawArrays(GL_TRIANGLES, 0, 3);

                        SDL_GL_SwapWindow(window);
                        frameCount++;
                    }
                }
            });
        }
        void setupSSBO(const std::vector<GpuTriangle>& tris)
        {
            size_t totalWords = 570 * 1140 * ((160 + 31) / 32);
            glGenBuffers(1, &voxelSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, voxelSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER, totalWords * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, voxelSSBO);

            numTriangles = static_cast<uint32_t>(tris.size());
            glGenBuffers(1, &triangleSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER, tris.size() * sizeof(GpuTriangle), tris.data(), GL_STATIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleSSBO);

        }
        void createGLEWContext()
        {
            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK)
            {
                std::cerr << "Failed to initialize GLEW\n";
                return;
            }

            SDL_GL_SetSwapInterval(1);
            glEnable(GL_FRAMEBUFFER_SRGB);
            glViewport(0, 0, WIDTH, HEIGHT);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            SDL_GL_SwapWindow(window);
        }
};

int main(int argc, char* argv[])
{
    #ifdef __EMSCRIPTEN__
        pass
    #else
        //VulkanEngine vulkanEngine;
        /*if (!vulkanEngine.initVulkan())
        {*/
            std::cout << "Failed to create vulkan instance\n" << std::endl;
            OpenGLEngine openglEngine;
        //}
    #endif

    return EXIT_SUCCESS;
}