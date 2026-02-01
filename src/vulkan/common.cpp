#include "common.hpp"
#include <stdexcept>

void vk_check(VkResult result, const std::string& msg)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(msg);
    }
}

int MAX_FRAMES_IN_FLIGHT = 3;
int WIDTH = 570;
int HEIGHT = 1140;