#include "CameraSystem.hpp"
#include "Logger.hpp"
#include <math.h>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

namespace render
{
CameraSystem::CameraSystem(GLFWwindow* window, float aspectRatio, float fov)
    : window(window)
{
    cam.cam_speed = 0.5f;
    cam.sensitivity = 0.5f;
    cam.pos_v.z = 5.0f; // (0,0,5) position
    cam.dir_v.z = -1.0f; // (0,0,-1) direction (opposite of where its looking)
    cam.up_v.y = 1.0f; // (0,1,0) up v of world space;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseMovementCallback);

    // build projection matrix
    proj_matrix = glm::perspective(fov, aspectRatio, 0.1f, 100.0f);
}

void CameraSystem::mouseMovementCallback(GLFWwindow*, double xpos, double ypos)
{
    std::unique_lock lock(cam_mutex);

    static bool first_mouse_input = false;

    if (not first_mouse_input)
    {
        cam.last_x = xpos;
        cam.last_y = ypos;
        first_mouse_input = true;
    }

    float xoffset = xpos - cam.last_x;
    float yoffset = cam.last_y - ypos;
    cam.last_x = xpos;
    cam.last_y = ypos;

    xoffset *= cam.sensitivity;
    yoffset *= cam.sensitivity;

    cam.yaw += xoffset;
    cam.pitch += yoffset;

    // camera pitch clamp to <-89, 89>
    if(cam.pitch > 89.0f)
        cam.pitch = 89.0f;
    if(cam.pitch < -89.0f)
        cam.pitch = -88.0f;

    glm::vec3 dir;
    dir[0] = cos(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    dir[1] = sin(glm::radians(cam.pitch));
    dir[2] = sin(glm::radians(cam.yaw)) * cos(glm::radians(cam.pitch));
    glm::normalize(dir);

    cam.dir_v = dir;
}

// This is done lazily, since camera state changes all the time and we only want to lock the data
// in-place and generate matrices during command buffer submission. So this needs to be done only once
// per frame, while camera callback can happen hundreds of times per second.
//
// Projection matrix also needs generation only once, this will be done in ctor.
CameraSystem::UboData CameraSystem::genCurrentVPMatrices()
{
    std::shared_lock lock(cam_mutex);
    UboData data =
    {
        .view = glm::lookAt(cam.pos_v, cam.pos_v + cam.dir_v, cam.up_v),
        .proj = proj_matrix
    };

    dbgI << "View matrix: " << NEWL;
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; j++)
        {
            std::cout << data.view[i][j] << "  ";
        }
        std::cout << NEWL;
    }

    return data;
}

} // namespace render
