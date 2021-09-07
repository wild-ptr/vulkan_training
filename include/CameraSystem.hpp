#pragma once
#include <glm/vec3.hpp>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <shared_mutex>

namespace render
{
class CameraSystem
{
public:
    struct UboData
    {
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    CameraSystem(GLFWwindow* window, float aspectRatio, float fov);

    // static because of glfwSetCursorPosCallback and its C api. It cannot capture this pointer...
    // Im too lazy to make this more abstract for now. Static it is.
    static void mouseMovementCallback(GLFWwindow*, double xpos, double ypos);
    UboData genCurrentVPMatrices();

    // I dont have any keyboard processing class for now so... lets just throw it into camera.
    void processKeyboardMovement();

private:
    enum class CameraDir
    {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT
    };

    void moveCamera(CameraDir);

    // since camera callbacks can happen all the time in async, we need to properly lock the
    // state of camera in place while generating VP matrices to avoid torn reads.
    inline static std::shared_mutex cam_mutex {};
    inline static struct Camera
    {
        glm::vec3 pos_v;
        glm::vec3 dir_v;
        glm::vec3 up_v;
        float cam_speed;
        float sensitivity;
        float last_x;
        float last_y;
        float pitch;
        float yaw;
    } cam{};

    GLFWwindow* window;
    glm::mat4 proj_matrix{};
};

} // namespace render
