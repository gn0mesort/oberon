#include "oberon/context.hpp"


#include <cstring>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <iterator>

#include "oberon/types.hpp"
#include "oberon/memory.hpp"

#include "oberon/detail/graphics.hpp"

namespace {
  static bool sg_has_init_extension_details{ false };
  static std::unordered_map<std::string, std::unordered_set<std::string>> sg_extension_by_layer{ };

  static std::unordered_set<std::string> get_extensions(
    const std::string& layer,
    const vk::DispatchLoaderDynamic& dl
  ) {
    auto extensions = vk::enumerateInstanceExtensionProperties(layer, dl);
    auto result = std::unordered_set<std::string>{ };
    std::transform(
      std::begin(extensions), std::end(extensions),
      std::inserter(result, std::begin(result)),
      [](const VkExtensionProperties& extension) {
        return extension.extensionName;
      }
    );

    return result;
  }

  static bool init_extension_details() {
    if (sg_has_init_extension_details)
    {
      return true;
    }
    auto dl = vk::DispatchLoaderDynamic{ };
    dl.init(vkGetInstanceProcAddr);
    auto layers = vk::enumerateInstanceLayerProperties(dl);
    sg_extension_by_layer[""] = get_extensions("", dl);
    for (const auto& layer : layers)
    {
      auto name = std::string{ layer.layerName };
      sg_extension_by_layer[name] = get_extensions(name, dl);
    }
    sg_has_init_extension_details = true;
    return true;
  }
}

namespace oberon {
  bool context::has_layer(const std::string_view& layer_name) {
    if (!init_extension_details())
    {
      return false;
    }
    return sg_extension_by_layer.find(layer_name.data()) != std::end(sg_extension_by_layer);
  }

  bool context::has_extension(const std::string_view& extension_name) {
    if (!init_extension_details())
    {
      return false;
    }
    const auto& bucket = sg_extension_by_layer.at("");
    return bucket.find(extension_name.data()) != std::end(bucket);
  }

  bool context::layer_has_extension(const std::string_view& layer_name, const std::string_view& extension_name) {
    if (!init_extension_details())
    {
      return false;
    }
    const auto bucket_itr = sg_extension_by_layer.find(layer_name.data());
    if (bucket_itr != std::end(sg_extension_by_layer))
    {
      return bucket_itr->second.find(extension_name.data()) != std::end(bucket_itr->second);
    }
    return false;
  }
}
