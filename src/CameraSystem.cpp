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
    cam.cam_speed = 0.05f;
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

    //float xoffset = xpos - cam.last_x;
    float xoffset = cam.last_x - xpos;
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

    return data;
}

// This will someday go into some keybind class. Not today!
// A dirty hack for now.
void CameraSystem::processKeyboardMovement()
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveCamera(CameraDir::FORWARD);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveCamera(CameraDir::BACKWARD);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveCamera(CameraDir::LEFT);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveCamera(CameraDir::RIGHT);
}

void CameraSystem::moveCamera(CameraDir dir)
{
    std::unique_lock lock(cam_mutex);
    glm::vec3 scaled = {0.0f, 0.0f, 0.0f};

    switch(dir)
    {
        case CameraDir::FORWARD:
            scaled = cam.dir_v * cam.cam_speed;
            cam.pos_v = cam.pos_v + scaled;
            break;
        case CameraDir::BACKWARD:
            scaled = cam.dir_v * cam.cam_speed;
            cam.pos_v = cam.pos_v - scaled;
            break;
        case CameraDir::LEFT:
            scaled = glm::cross(cam.dir_v, cam.up_v);
            scaled = glm::normalize(scaled);
            scaled *= cam.cam_speed;
            cam.pos_v += scaled;
            break;
        case CameraDir::RIGHT:
            scaled = glm::cross(cam.dir_v, cam.up_v);
            scaled = glm::normalize(scaled);
            scaled *= cam.cam_speed;
            cam.pos_v -= scaled;
            break;
    }
}

} // namespace render
