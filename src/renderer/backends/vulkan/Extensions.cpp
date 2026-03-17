#include "Extensions.h"
#include <spdlog/spdlog.h>

std::vector<const char *>
SYN::VK::findExtensions(std::span<const char *> requiredExtensions,
                    std::span<VkExtensionProperties> availableExtensionProps) {
    std::vector<const char *> foundExtensions{};

    for (const auto &extension : requiredExtensions) {
        // could be sped up, but doesn't matter since were not using many
        // extensions and this only happens once
        auto itt{std::find_if(
            availableExtensionProps.begin(), availableExtensionProps.end(),
            [&](VkExtensionProperties extensionProp) {
                return strcmp(extension, extensionProp.extensionName) == 0;
            })};

        if (itt != availableExtensionProps.end()) {
            // this is the .rodata string, safer for lifetimes since it cant go
            // out of scope
            foundExtensions.emplace_back(itt->extensionName);
        } else {
            spdlog::warn("Could not find Vulkan extension {}", extension);
        }
    }

    return foundExtensions;
}
