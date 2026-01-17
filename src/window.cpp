#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

bool framebufferResized = false;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

GLFWwindow *window;

static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    framebufferResized = true;
}

void initWindow(void *application)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, application);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}
