#include <stdint.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <sys/types.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <string.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const char* requiredDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAMES_IN_FLIGHT = 2;

typedef struct {
	vec3 pos;
	vec3 color;
} Vertex;

#define VERTEX_BUFFER_SIZE 4
#define INDEX_BUFFER_SIZE 6

const Vertex vertices[VERTEX_BUFFER_SIZE] = {
	{{-0.5f, -0.5f, 0}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f, 0}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f, 0}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0}, {0.0f, 0.0f, 0.0f}},
};

const uint32_t indices[INDEX_BUFFER_SIZE] = {
	0, 1, 2, 2, 3, 0,
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != NULL) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != NULL) {
		func(instance, debugMessenger, pAllocator);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

	fprintf(stderr, "validation layer: %s\n", pCallbackData->pMessage);

	return VK_FALSE;
}

// Helper function

int clamp(int value, int min, int max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

typedef struct QueueFamilyIndices {
	int32_t graphicsFamily;
	int32_t presentFamily;
} QueueFamilyIndices;

typedef struct UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 proj;
} UniformBufferObject;

VkShaderModule RECreateShaderModule(VkDevice logicalDevice, char* filename){
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open shader file\n");
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	size_t fileSize = ftell(file);
	rewind(file);

	char* buffer = (char*)malloc(fileSize);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate memory for shader\n");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	if (fread(buffer, 1, fileSize, file) != fileSize) {
		fprintf(stderr, "Failed to read file\n");
		free(buffer);
		fclose(file);
		exit(EXIT_FAILURE);
	}

	fclose(file);

	// Ensure the file size is a multiple of 4 for Vulkan requirements
	if (fileSize % 4 != 0) {
		fprintf(stderr, "File size is not a multiple of 4. Is the shader corrupt?\n");
		free(buffer);
		exit(EXIT_FAILURE);
	}

	VkShaderModuleCreateInfo ShaderModuleInfo = {};
	ShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ShaderModuleInfo.codeSize = fileSize;
	ShaderModuleInfo.pCode =  (uint32_t*)buffer;


	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &ShaderModuleInfo, NULL, &shaderModule) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create GLFW window\n");
		free(buffer);
		exit(EXIT_FAILURE);
	}

	return shaderModule;

}
bool REIsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR vulkanSurface) {
	//VkPhysicalDeviceProperties deviceProperties;
	//VkPhysicalDeviceFeatures deviceFeatures;
	//vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL);

	VkExtensionProperties* availableExtensions = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));

	vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions);

	// Check for required extension (could be improved)

	uint32_t foundExtensions = 0;

	for (uint32_t i = 0; i < extensionCount; i++){
		if (strcmp(availableExtensions[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)){
			foundExtensions++;
		}
	}

	free(availableExtensions);

	if (foundExtensions < 1){
		printf("warning: Device is missing vulkan extensions\n"); // TODO: Show missing extensions
		return false;
	}

	// Check for swap chain support

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkanSurface, &formatCount, NULL);

	if (formatCount <= 0){
		printf("warning: Device is missing surface formats\n");
		return false;
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkanSurface, &presentModeCount, NULL);

	if (presentModeCount <= 0){
		printf("warning: Device is missing present modes\n");
		return false;
	}

	// Check for required queue families

	//if (indices.graphicsFamily == -1){
	//	printf("warning: Device is missing graphics queue family\n");
	//	return false;
	//}

	return true;

}

typedef struct RECreateSwapChainReqs {
	VkPhysicalDevice physicalDevice;
	VkSurfaceKHR surface;
	QueueFamilyIndices queueFamilyIndices;
	VkExtent2D swapChainExtent;
	VkFormat swapChainImageFormat;
	uint32_t swapChainMinImageCount;
	VkColorSpaceKHR imageColorSpace;
	VkSurfaceTransformFlagBitsKHR preTransform;

} RECreateSwapChainReqs;

VkSwapchainKHR createSwapChain(VkDevice device, RECreateSwapChainReqs requirements) {
	VkPhysicalDevice physicalDevice = requirements.physicalDevice;
	VkSurfaceKHR surface = requirements.surface;
	QueueFamilyIndices queueFamilyIndices = requirements.queueFamilyIndices;
	VkExtent2D swapChainExtent = requirements.swapChainExtent;
	VkFormat swapChainImageFormat = requirements.swapChainImageFormat;
	uint32_t swapChainMinImageCount = requirements.swapChainMinImageCount;
	VkColorSpaceKHR imageColorSpace = requirements.imageColorSpace;
	VkSurfaceTransformFlagBitsKHR preTransform = requirements.preTransform;

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);
	assert(presentModeCount >= 0);

	VkPresentModeKHR* presentModes = (VkPresentModeKHR*)malloc(presentModeCount * sizeof(VkPresentModeKHR));    // Needs to be freed
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

	// If possible, choose mailbox khr, otherwise fifo khr is always present

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

	for (uint32_t i = 0; i < presentModeCount; i++){
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = presentModes[i];
		}
	}

	free(presentModes);


	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;

	swapChainCreateInfo.minImageCount = swapChainMinImageCount;
	swapChainCreateInfo.imageFormat = swapChainImageFormat;
	swapChainCreateInfo.imageColorSpace = imageColorSpace;
	swapChainCreateInfo.imageExtent = swapChainExtent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndicesUint[] = {queueFamilyIndices.graphicsFamily, queueFamilyIndices.presentFamily};

	// Use concurrent if queue families are different

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesUint;
	} else {
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = NULL;
	}

	swapChainCreateInfo.preTransform = preTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore alpha channel
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	VkSwapchainKHR swapChain;

	if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, NULL, &swapChain) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create swap chain\n");
		exit(EXIT_FAILURE);
	}

	return swapChain;
}

typedef struct RECleanUpSwapChainReqs {
	uint32_t swapChainImageCount;
	VkFramebuffer* swapChainFramebuffers;
	VkImageView* swapChainImageViews;
	VkSwapchainKHR swapChain;

} RECleanUpSwapChainReqs;

void RECleanUpSwapChain(VkDevice logicalDevice, RECleanUpSwapChainReqs requirements) {
	uint32_t swapChainImageCount = requirements.swapChainImageCount;
	VkFramebuffer* swapChainFramebuffers = requirements.swapChainFramebuffers;
	VkImageView* swapChainImageViews = requirements.swapChainImageViews;
	VkSwapchainKHR swapChain = requirements.swapChain;

	for (size_t i = 0; i < swapChainImageCount; i++) {
		vkDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], NULL);
		vkDestroyImageView(logicalDevice, swapChainImageViews[i], NULL);

	}

	vkDestroySwapchainKHR(logicalDevice, swapChain, NULL);
}

typedef struct RECreateAllocateBufferReqs {
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	VkDeviceSize size;
	VkBufferUsageFlags usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags;

} RECreateAllocateBufferReqs;

VkBuffer RECreateAndAllocateBuffer(VkDevice device, VkDeviceMemory* bufferMemory, RECreateAllocateBufferReqs requirements){
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = requirements.physicalDeviceMemoryProperties;
	VkDeviceSize size = requirements.size;
	VkBufferUsageFlags usageFlags = requirements.usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags = requirements.memoryPropertyFlags;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;

	if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &buffer) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create vertex buffer\n");
		exit(EXIT_FAILURE);
	}

	// Find correct memory type given the flags and memory requirements

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);


	uint32_t memoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
		if ((memoryRequirements.memoryTypeBits & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == UINT32_MAX) {
		fprintf(stderr, "Failed to find a suitable memory type\n");
		exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(device, &memoryAllocateInfo, NULL, bufferMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate buffer memory\n");
		exit(EXIT_FAILURE);
	}
	vkBindBufferMemory(device, buffer, *bufferMemory, 0);


	return buffer;
}

void createTextureImage(VkDevice logicalDevice, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		fprintf(stderr, "Failed to load texture image!\n");
		exit(EXIT_FAILURE);
	}

	RECreateAllocateBufferReqs stagingBufferReqs = {};
	stagingBufferReqs.physicalDeviceMemoryProperties = physicalDeviceMemoryProperties;
	stagingBufferReqs.size = imageSize;
	stagingBufferReqs.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferReqs.memoryPropertyFlags =  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkDeviceMemory stagingBufferMemory;

	VkBuffer stagingBuffer = RECreateAndAllocateBuffer(logicalDevice, &stagingBufferMemory, stagingBufferReqs);

	void* data;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, imageSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	stbi_image_free(pixels);

	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = texWidth;
	imageCreateInfo.extent.height = texHeight;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	if (vkCreateImage(logicalDevice, &imageCreateInfo, NULL, &textureImage) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create image\n");
		exit(EXIT_FAILURE);
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(logicalDevice, textureImage, &memoryRequirements);

	uint32_t memoryTypeIndex = UINT32_MAX;
	for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
		if ((memoryRequirements.memoryTypeBits & (1 << i)) && (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
			memoryTypeIndex = i;
			break;
		}
	}

	if (memoryTypeIndex == UINT32_MAX) {
		fprintf(stderr, "Failed to find a suitable memory type\n");
		exit(EXIT_FAILURE);
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	if (vkAllocateMemory(logicalDevice, &memoryAllocateInfo, NULL, &textureImageMemory) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate image memory\n");
		exit(EXIT_FAILURE);
	}

	vkBindImageMemory(logicalDevice, textureImage, textureImageMemory, 0);

}



typedef struct RECreateSwapChainFrameBuffersReqs {
	VkSwapchainKHR swapChain;
	uint32_t swapChainImageCount;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass;

} RECreateSwapChainFrameBuffersReqs;

void RECreateSwapChainFrameBuffers(VkDevice logicalDevice, VkImageView* swapChainImageViews, VkFramebuffer* swapChainFramebuffers, RECreateSwapChainFrameBuffersReqs requirements){
	VkSwapchainKHR swapChain = requirements.swapChain;
	uint32_t swapChainImageCount = requirements.swapChainImageCount;
	VkFormat swapChainImageFormat = requirements.swapChainImageFormat;
	VkExtent2D swapChainExtent = requirements.swapChainExtent;
	VkRenderPass renderPass = requirements.renderPass;


	VkImage* swapChainImages = malloc(swapChainImageCount * sizeof(VkImage));
	if (!swapChainImages) {
		fprintf(stderr, "Failed to allocate memory for swapchain images.\n");
		exit(EXIT_FAILURE);
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages);

	for (uint32_t i = 0; i < swapChainImageCount; i++) {
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = swapChainImages[i];

		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapChainImageFormat;

		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(logicalDevice, &imageViewCreateInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create image view\n");
			exit(EXIT_FAILURE);
		}
	}

	free(swapChainImages);

	for (size_t i = 0; i < swapChainImageCount; i++) {
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = swapChainExtent.width;
		framebufferCreateInfo.height = swapChainExtent.height;
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to create framebuffer\n");
			exit(EXIT_FAILURE);
		}
	}

}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	// Retrieve the user pointer
	bool* framebufferResized = (bool*)glfwGetWindowUserPointer(window);

	if (framebufferResized) {
		*framebufferResized = true;
	}
}


VkVertexInputBindingDescription* REgetVertexInputBindingDescription() {
	static VkVertexInputBindingDescription bindingDescription[1];
	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

VkVertexInputAttributeDescription* REgetVertexInputAttributeDescription() {
	static VkVertexInputAttributeDescription attributeDescriptions[2];

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	return attributeDescriptions;

}

int main() {
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	//
	// Window creationg
	//


	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	printf("Creating Window...\n");

	GLFWwindow* window = NULL;

	window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	bool framebufferResized = false;
	glfwSetWindowUserPointer(window, &framebufferResized);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	//
	// Initialize Vulkan
	//

	printf("Initializing Vulkan...\n");

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	// Add appInfo to createInfo

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	createInfo.pApplicationInfo = &appInfo;

	// Add extentions to createInfo

	// Get glfwExtensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	if (!glfwExtensions) {
		fprintf(stderr, "Failed to get vulkan extensions\n");
		return -1;
	}

	// Add validation layer extension
	uint32_t extensionsCount = glfwExtensionCount + (enableValidationLayers ? 1 : 0);

	const char* extensions[10]; // hotfix. 10 is the max amount of extensions

	//const char** extensions = malloc(extensionsCount * sizeof(char*));
	//if (!extensions) {
	//    fprintf(stderr, "Memory allocation for extensions failed\n");
	//    return -1;
	//}

	memmove(extensions, glfwExtensions, glfwExtensionCount * sizeof(char*));

	if (enableValidationLayers) {
		extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	}

	// Confirm if extensions are availible

	uint32_t vulkanExtensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &vulkanExtensionCount, NULL);

	VkExtensionProperties* vulkanExtensions = (VkExtensionProperties*)malloc(vulkanExtensionCount * sizeof(VkExtensionProperties));
	if (!vulkanExtensions) {
		fprintf(stderr, "Memory allocation for availableLayers failed\n");
		return -1;
	}

	if (vkEnumerateInstanceExtensionProperties(NULL, &vulkanExtensionCount, vulkanExtensions) != VK_SUCCESS) {
		fprintf(stderr, "Failed to enumerate instance extension properties\n");
		free(vulkanExtensions);
		return false;
	}

	bool extensionsPresent = true;

	for (uint32_t i = 0; i < extensionsCount; i++){
		bool extensionFound = false;
		for (uint32_t j = 0; j < vulkanExtensionCount; j++) {
			if (strcmp(extensions[i], vulkanExtensions[j].extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound){
			extensionsPresent = false;
			break;
		}
	}

	free(vulkanExtensions);

	if (!extensionsPresent){
		fprintf(stderr, "Extensions requested but not found\n"); // TODO: Show missing layers
		return -1;
	}

	createInfo.enabledExtensionCount = extensionsCount;
	createInfo.ppEnabledExtensionNames = extensions;

	// Add layers to createInfo, and confirm their availablity

	const char* layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	uint32_t layersSize = sizeof(layers) / sizeof(layers[0]);

	uint32_t vulkanLayerCount;
	if(vkEnumerateInstanceLayerProperties(&vulkanLayerCount, NULL) != VK_SUCCESS){
		fprintf(stderr, "Failed to get vulkan layers\n");
		return -1;
	};

	VkLayerProperties* vulkanLayers = (VkLayerProperties*)malloc(vulkanLayerCount * sizeof(VkLayerProperties));
	if (!vulkanLayers) {
		fprintf(stderr, "Memory allocation for availableLayers failed\n");
		return -1;
	}

	if(vkEnumerateInstanceLayerProperties(&vulkanLayerCount, vulkanLayers) != VK_SUCCESS){
		fprintf(stderr, "Failed to enumerate instance layer properties\n");
		free(vulkanLayers);
		return -1;
	};

	// TODO: make this O(n)
	bool layersPresent = true;
	size_t validationLayersLength = layersSize;
	for (uint32_t i = 0; i < validationLayersLength; i++){
		bool layerFound = false;
		for (uint32_t j = 0; j < vulkanLayerCount; j++) {
			if (strcmp(layers[i], vulkanLayers[j].layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound){
			layersPresent = false;
			break;
		}
	}

	free(vulkanLayers);

	if (enableValidationLayers && !layersPresent){
		fprintf(stderr, "Validation layers requested but not found"); // TODO: Show missing layers
		return -1;
	}
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = layersSize;
		createInfo.ppEnabledLayerNames = layers;
	} else {
		createInfo.enabledLayerCount = 0;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = {};
	debugUtilsCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsCreateInfo.pfnUserCallback = debugCallback;
	debugUtilsCreateInfo.pUserData = NULL;


	// Add VkDebugUtilsMessengerCreateInfoEXT to pNext if using validation layers

	if (enableValidationLayers) {
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugUtilsCreateInfo;
	}
	else {
		createInfo.pNext = NULL;
	}

	// Create instance with createInfo

	VkInstance vulkanInstance;

	VkResult resultInstance = vkCreateInstance(&createInfo, NULL, &vulkanInstance);
	if (resultInstance != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan instance, errno: %d\n", resultInstance);
		return -1;
	};

	VkDebugUtilsMessengerEXT debugMessenger;
	if (CreateDebugUtilsMessengerEXT(vulkanInstance, &debugUtilsCreateInfo, NULL, &debugMessenger) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create debug utils messager\n");
		return -1;
	}

	//
	// Surface
	//


	VkSurfaceKHR vulkanSurface;

	if (glfwCreateWindowSurface(vulkanInstance, window, NULL, &vulkanSurface) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create window surface\n");
		return -1;
	}

	//
	// Pick  Physical device
	//


	printf("Choosing physical device...\n");

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, NULL);

	if (deviceCount == 0) {
		fprintf(stderr, "Failed to find devices with Vulkan support\n");
		exit(EXIT_FAILURE);
	}

	VkPhysicalDevice* vulkanDevices = (VkPhysicalDevice*)malloc(deviceCount * sizeof(VkPhysicalDevice));
	if (!vulkanDevices) {
		fprintf(stderr, "Failed to allocate memory for Vulkan devices\n");
		exit(EXIT_FAILURE);
	}

	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, vulkanDevices);

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;


	// Pick the best device.

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice currentDevice = vulkanDevices[i];
		if (REIsDeviceSuitable(currentDevice, vulkanSurface)) {
			physicalDevice = currentDevice;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		fprintf(stderr, "Failed to find a suitable device\n");
		exit(EXIT_FAILURE);
	}

	free(vulkanDevices);

	//
	// Initialize logical device
	//

	// Find queue family indices

	QueueFamilyIndices queueFamilyIndices = {-1,-1}; // -1 is set if a family is NOT AVAILIBLE

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

	VkQueueFamilyProperties* queueFamilies = (VkQueueFamilyProperties*)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

	for (int32_t i = 0; i < queueFamilyCount; i++) {

		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamilyIndices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, vulkanSurface, &presentSupport);

		if (presentSupport) {
			queueFamilyIndices.presentFamily = i;
		}

		if (queueFamilyIndices.graphicsFamily != -1 && queueFamilyIndices.presentFamily != -1) { // && indices.second != -1 && indices.oth ... // check that all families are set
			break;
		}
	}

	free(queueFamilies);

	// Create logical device with queues

	VkDevice logicalDevice;

	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceQueueCreateInfo queueCreateInfos[2]; // graphicsFamily, presentFamily

	VkDeviceQueueCreateInfo graphicsFamilyQueueCreateInfo = {};
	graphicsFamilyQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphicsFamilyQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	graphicsFamilyQueueCreateInfo.queueCount = 1;
	float graphicsFamilyQueuePriority = 1.0f;
	graphicsFamilyQueueCreateInfo.pQueuePriorities = &graphicsFamilyQueuePriority;


	queueCreateInfos[0] = graphicsFamilyQueueCreateInfo;

	VkDeviceQueueCreateInfo presentFamilyQueueCreateInfo = {};
	presentFamilyQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	presentFamilyQueueCreateInfo.queueFamilyIndex = queueFamilyIndices.presentFamily;
	presentFamilyQueueCreateInfo.queueCount = 1;
	float presentFamilyQueuePriority = 1.0f;
	presentFamilyQueueCreateInfo.pQueuePriorities = &presentFamilyQueuePriority;

	queueCreateInfos[1] = presentFamilyQueueCreateInfo;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.queueCreateInfoCount = 1;

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	deviceCreateInfo.enabledExtensionCount = sizeof(requiredDeviceExtensions) / sizeof(requiredDeviceExtensions[0]);;
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensions;

	deviceCreateInfo.enabledLayerCount = 0;

	//^  TODO: Layers backwards compatibility
	// Add extensions?

	VkResult resultDevice = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &logicalDevice);
	if (resultDevice != VK_SUCCESS) {
		fprintf(stderr, "Failed to create Vulkan devic: %d\n", resultDevice);
		return -1;
	}

	//
	// Queues
	//

	VkQueue graphicsQueue;

	vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily, 0, &graphicsQueue);

	VkQueue presentQueue;

	vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily, 0, &presentQueue);

	//
	// Create swap chain
	//

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanSurface, &formatCount, NULL);

	VkSurfaceFormatKHR* formats = (VkSurfaceFormatKHR*)malloc(formatCount * sizeof(VkSurfaceFormatKHR));  // Needs to be freed
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, vulkanSurface, &formatCount, formats);

	// TODO: Format rating system, instead of taking the first one
	VkSurfaceFormatKHR surfaceFormat = formats[0];

	// Find the ideal format
	for (uint32_t i = 0; i < formatCount; i++){
		if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = formats[i];
		}
	}

	VkFormat swapChainImageFormat = surfaceFormat.format;
	VkColorSpaceKHR imageColorSpace = surfaceFormat.colorSpace;

	free(formats);

	VkSurfaceCapabilitiesKHR  surfaceCapabilities;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vulkanSurface, &surfaceCapabilities) != VK_SUCCESS){
		fprintf(stderr, "Failed to get surface capabilities\n");
		exit(EXIT_FAILURE);
	};

	VkExtent2D swapChainExtent;

	int framebufferWidth, framebufferHeight;
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

	if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		swapChainExtent = surfaceCapabilities.currentExtent;
	} else {
		swapChainExtent.width = clamp(framebufferWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		swapChainExtent.height = clamp(framebufferHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	}


	uint32_t swapChainMinImageCount = surfaceCapabilities.minImageCount + 1;

	// Avoid exceeding maximum image count
	if (surfaceCapabilities.maxImageCount > 0 && swapChainMinImageCount > surfaceCapabilities.maxImageCount) {
		swapChainMinImageCount = surfaceCapabilities.maxImageCount;
	}

	RECreateSwapChainReqs createSwapChainReqs = {};
	createSwapChainReqs.physicalDevice = physicalDevice;
	createSwapChainReqs.surface = vulkanSurface;
	createSwapChainReqs.queueFamilyIndices = queueFamilyIndices;
	createSwapChainReqs.swapChainExtent = swapChainExtent;
	createSwapChainReqs.swapChainImageFormat = swapChainImageFormat;
	createSwapChainReqs.swapChainMinImageCount = swapChainMinImageCount;
	createSwapChainReqs.imageColorSpace = imageColorSpace;
	createSwapChainReqs.preTransform = surfaceCapabilities.currentTransform; // Do not transform (same as current)

	VkSwapchainKHR swapChain = createSwapChain(logicalDevice, createSwapChainReqs);


	//
	// Create render passes
	//

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VkRenderPass renderPass;

	if (vkCreateRenderPass(logicalDevice, &renderPassCreateInfo, NULL, &renderPass) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create render pass\n");
		return -1;
	}

	//
	// Create shader modules
	//

	printf("Loading shaders...\n");

	VkShaderModule vertShaderModule = RECreateShaderModule(logicalDevice, "shaders/vert.spv");
	VkShaderModule fragShaderModule = RECreateShaderModule(logicalDevice, "shaders/frag.spv");

	//
	// Create graphics pipeline
	//

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";


	// Use malloc ?

	int32_t dynamicStateCount = 2;
	VkDynamicState requestedDynamicStates[2] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	//VkDynamicState* dynamicStates = malloc(dynamicStateCount * sizeof(VkDynamicState));

	//memmove(dynamicStates, requestedDynamicStates, sizeof(requestedDynamicStates));


	VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo, fragShaderStageInfo};

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
	dynamicStateCreateInfo.pDynamicStates = requestedDynamicStates;

	//free(dynamicStates);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1; // Fixed value
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2; // Fixed value

	vertexInputStateCreateInfo.pVertexBindingDescriptions = REgetVertexInputBindingDescription();
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = REgetVertexInputAttributeDescription();


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) swapChainExtent.width;
	viewport.height = (float) swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D) {0, 0};
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo  = {};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pSampleMask = NULL;
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = NULL;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create descriptor set layout\n");
		return -1;
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	VkPipelineLayout pipelineLayout;

	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pipeline layout\n");
		return -1;
	}

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = NULL;
	pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	VkPipeline graphicsPipeline;
	if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create graphics pipeline\n");
		return -1;
	}

	//
	// Create frame buffers
	//

	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, NULL);

	VkFramebuffer* swapChainFramebuffers = malloc(swapChainImageCount * sizeof(VkFramebuffer));
	VkImageView* swapChainImageViews = malloc(swapChainImageCount * sizeof(VkImageView));

	RECreateSwapChainFrameBuffersReqs createSwapChainFrameBuffersReqs = {};
	createSwapChainFrameBuffersReqs.swapChain = swapChain;
	createSwapChainFrameBuffersReqs.swapChainImageCount = swapChainImageCount;
	createSwapChainFrameBuffersReqs.swapChainImageFormat = swapChainImageFormat;
	createSwapChainFrameBuffersReqs.swapChainExtent = swapChainExtent;
	createSwapChainFrameBuffersReqs.renderPass = renderPass;

	RECreateSwapChainFrameBuffers(logicalDevice, swapChainImageViews, swapChainFramebuffers, createSwapChainFrameBuffersReqs);


	//
	// Command buffers
	//

	VkCommandPool commandPool;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	if (vkCreateCommandPool(logicalDevice, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create command pool\n");
		return -1;
	}

	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, commandBuffers) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate command buffers\n");
		return -1;
	}

	//
	// Staging buffer and vertex buffer creation
	//

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

	//
	// Create staging buffer

	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * VERTEX_BUFFER_SIZE;
	VkDeviceSize indexBufferSize = sizeof(indices[0]) * INDEX_BUFFER_SIZE;
	VkDeviceSize stagingBufferSize = indexBufferSize + vertexBufferSize;

	RECreateAllocateBufferReqs stagingBufferReqs = {};
	stagingBufferReqs.physicalDeviceMemoryProperties = physicalDeviceMemoryProperties;
	stagingBufferReqs.size = stagingBufferSize;
	stagingBufferReqs.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferReqs.memoryPropertyFlags =  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkDeviceMemory stagingBufferMemory;

	VkBuffer stagingBuffer = RECreateAndAllocateBuffer(logicalDevice, &stagingBufferMemory, stagingBufferReqs);

	// Copy vertex and index data to the staging buffer

	void* stagingData;
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, stagingBufferSize, 0, &stagingData);
	memcpy(stagingData, vertices, (size_t) vertexBufferSize);
	memcpy(stagingData + vertexBufferSize, indices, (size_t) indexBufferSize);
	vkUnmapMemory(logicalDevice, stagingBufferMemory);

	// Create vertex buffer

	RECreateAllocateBufferReqs vertexBufferReqs = {};
	vertexBufferReqs.physicalDeviceMemoryProperties = physicalDeviceMemoryProperties;
	vertexBufferReqs.size = vertexBufferSize;
	vertexBufferReqs.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexBufferReqs.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkDeviceMemory vertexBufferMemory;

	VkBuffer vertexBuffer = RECreateAndAllocateBuffer(logicalDevice, &vertexBufferMemory, vertexBufferReqs);

	// Create index buffer

	RECreateAllocateBufferReqs indexBufferReqs = {};
	indexBufferReqs.physicalDeviceMemoryProperties = physicalDeviceMemoryProperties;
	indexBufferReqs.size = indexBufferSize;
	indexBufferReqs.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	indexBufferReqs.memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	VkDeviceMemory indexBufferMemory;

	VkBuffer indexBuffer = RECreateAndAllocateBuffer(logicalDevice, &indexBufferMemory, indexBufferReqs);

	// Copy staging buffer to index and vertex buffer

	VkCommandBufferAllocateInfo copyCommandBufferAllocateInfo = {};
	copyCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	copyCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	copyCommandBufferAllocateInfo.commandPool = commandPool;
	copyCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer copyCommandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &copyCommandBufferAllocateInfo, &copyCommandBuffer);

	VkCommandBufferBeginInfo copyCommandBufferBeginInfo = {};
	copyCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	copyCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(copyCommandBuffer, &copyCommandBufferBeginInfo);

	VkBufferCopy vertexCopyRegion = {};
	vertexCopyRegion.srcOffset = 0;
	vertexCopyRegion.dstOffset = 0;
	vertexCopyRegion.size = vertexBufferSize;

	vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer, vertexBuffer, 1, &vertexCopyRegion);

	VkBufferCopy indexCopyRegion = {};
	indexCopyRegion.srcOffset = vertexBufferSize;
	indexCopyRegion.dstOffset = 0;
	indexCopyRegion.size = indexBufferSize;

	vkCmdCopyBuffer(copyCommandBuffer, stagingBuffer, indexBuffer, 1, &indexCopyRegion);

	vkEndCommandBuffer(copyCommandBuffer);

	VkSubmitInfo copySubmitInfo = {};
	copySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	copySubmitInfo.commandBufferCount = 1;
	copySubmitInfo.pCommandBuffers = &copyCommandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &copySubmitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkDestroyBuffer(logicalDevice, stagingBuffer, NULL);
	vkFreeMemory(logicalDevice, stagingBufferMemory, NULL);

	//
	// Create uniform buffers
	//

	VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
	void* uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDeviceSize uniformBufferSize = sizeof(UniformBufferObject);

		RECreateAllocateBufferReqs uniformBufferReqs = {};
		uniformBufferReqs.physicalDeviceMemoryProperties = physicalDeviceMemoryProperties;
		uniformBufferReqs.size = uniformBufferSize;
		uniformBufferReqs.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		uniformBufferReqs.memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		uniformBuffers[i] = RECreateAndAllocateBuffer(logicalDevice, &uniformBuffersMemory[i], uniformBufferReqs);

		vkMapMemory(logicalDevice, uniformBuffersMemory[i], 0, uniformBufferSize, 0, &uniformBuffersMapped[i]);
	}

	//
	// CreateDescriptorPool
	//

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorPoolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;
	descriptorPoolCreateInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
	descriptorPoolCreateInfo.flags = 0;

	VkDescriptorPool descriptorPool;

	if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolCreateInfo, NULL, &descriptorPool) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create descriptor pool\n");
		return -1;
	}

	VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		layouts[i] = descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
	descriptorSetAllocateInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
	if (vkAllocateDescriptorSets(logicalDevice, &descriptorSetAllocateInfo, descriptorSets) != VK_SUCCESS) {
		fprintf(stderr, "Failed to allocate descriptor sets\n");
		return -1;
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo descriptorBufferInfo = {};
		descriptorBufferInfo.buffer = uniformBuffers[i];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &descriptorBufferInfo;
		descriptorWrite.pImageInfo = NULL;
		descriptorWrite.pTexelBufferView = NULL;

		vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, NULL);
	}


	//
	// Create sync objects
	//

	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(logicalDevice, &fenceInfo, NULL, &inFlightFences[i]) != VK_SUCCESS
		) {
			fprintf(stderr, "Failed to create sempaphores\n");
			return -1;
		}
	}

	//
	// Main loop
	//

	RECleanUpSwapChainReqs cleanUpSwapChainRequirements = {};
	cleanUpSwapChainRequirements.swapChainImageCount = swapChainImageCount;

	uint32_t currentFrame = 0;
	float rotationMultiplier = 0;

	printf("Running the main loop...\n");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;

		VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		//printf("Resized %d \n", framebufferResized);
		//  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			framebufferResized = false;

			// Recreate the swapchain

			int framebufferWidth = 0, framebufferHeight = 0;
			glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
			while (framebufferWidth == 0 || framebufferHeight == 0) {
				glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
				glfwWaitEvents();
			}

			vkDeviceWaitIdle(logicalDevice);

			cleanUpSwapChainRequirements.swapChainFramebuffers = swapChainFramebuffers;
			cleanUpSwapChainRequirements.swapChainImageViews = swapChainImageViews;
			cleanUpSwapChainRequirements.swapChain = swapChain;

			RECleanUpSwapChain(logicalDevice, cleanUpSwapChainRequirements);

			if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, vulkanSurface, &surfaceCapabilities) != VK_SUCCESS){
				fprintf(stderr, "Failed to get surface capabilities\n");
				exit(EXIT_FAILURE);
			};

			if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
				swapChainExtent = surfaceCapabilities.currentExtent;
			} else {
				swapChainExtent.width = clamp(framebufferWidth, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
				swapChainExtent.height = clamp(framebufferHeight, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
			}

			createSwapChainReqs.swapChainExtent = swapChainExtent;

			swapChain = createSwapChain(logicalDevice, createSwapChainReqs);

			createSwapChainFrameBuffersReqs.swapChain = swapChain;
			createSwapChainFrameBuffersReqs.swapChainExtent = swapChainExtent;

			RECreateSwapChainFrameBuffers(logicalDevice, swapChainImageViews, swapChainFramebuffers, createSwapChainFrameBuffersReqs);
			continue;

		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			fprintf(stderr, "Failed to acquire swap chain image!");
			return -1;
		}

		vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);

		VkCommandBufferBeginInfo commandBufferBeginInfo = {};

		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = 0;
		commandBufferBeginInfo.pInheritanceInfo = NULL;

		if (vkBeginCommandBuffer(commandBuffers[currentFrame], &commandBufferBeginInfo) != VK_SUCCESS) {
			fprintf(stderr, "Failed to begin command buffer\n");
			return -1;
		}

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassBeginInfo.renderArea.offset = (VkOffset2D){0, 0};
		renderPassBeginInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = swapChainExtent.width;
		viewport.height = swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset = (VkOffset2D){0, 0};
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffers[currentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, NULL);
		vkCmdDrawIndexed(commandBuffers[currentFrame], INDEX_BUFFER_SIZE, 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffers[currentFrame]);

		if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to end command buffer\n");
			return -1;
		}

		static struct timespec startTime;
		clock_gettime(CLOCK_MONOTONIC, &startTime);
		struct timespec currentTime;
		clock_gettime(CLOCK_MONOTONIC, &currentTime);


		float time = (currentTime.tv_sec - startTime.tv_sec) +
			(currentTime.tv_nsec - startTime.tv_nsec) / 1e9f;

		//printf("Elapsed time: %f seconds\n", time);
		UniformBufferObject ubo;

		// Set all matrices to the identity matrix
		glm_mat4_copy(GLM_MAT4_IDENTITY, ubo.model);
		glm_mat4_copy(GLM_MAT4_IDENTITY, ubo.view);
		glm_mat4_copy(GLM_MAT4_IDENTITY, ubo.proj);

		glm_rotate(ubo.model, rotationMultiplier * glm_rad(90.0f), (vec3) { 0.0f, 0.0f, 1.0f });
		//glm_rotate(ubo.model, glm_rad(90.0f), (vec3){0.0f, 0.0f, 1.0f});
		glm_lookat((vec3) { 2.0f, 2.0f, 2.0f }, (vec3) { 0.0f, 0.0f, 0.0f }, (vec3) { 0.0f, 0.0f, 1.0f }, ubo.view);
		float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
		glm_perspective(glm_rad(45.0f), aspect, 0.1f, 10.0f, ubo.proj );
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			fprintf(stderr, "Failed to submit draw command buffer\n");
			return -1;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		presentInfo.pResults = NULL;

		vkQueuePresentKHR(presentQueue, &presentInfo);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		rotationMultiplier += 0.01f;
	}

	vkDeviceWaitIdle(logicalDevice);

	//
	// Cleanup
	//

	printf("Cleaning up resources...\n");

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], NULL);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], NULL);
		vkDestroyFence(logicalDevice, inFlightFences[i], NULL);
	}

	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(vulkanInstance, debugMessenger, NULL);
	}

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &copyCommandBuffer);

	vkDestroyCommandPool(logicalDevice, commandPool, NULL);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(logicalDevice, uniformBuffers[i], NULL);
		vkFreeMemory(logicalDevice, uniformBuffersMemory[i], NULL);
	}

	vkDestroyDescriptorPool(logicalDevice, descriptorPool, NULL);

	vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, NULL);

	vkDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);



	vkDestroyRenderPass(logicalDevice, renderPass, NULL);

	vkDestroyShaderModule(logicalDevice, fragShaderModule, NULL);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, NULL);

	cleanUpSwapChainRequirements.swapChainFramebuffers = swapChainFramebuffers;
	cleanUpSwapChainRequirements.swapChainImageViews = swapChainImageViews;
	cleanUpSwapChainRequirements.swapChain = swapChain;

	RECleanUpSwapChain(logicalDevice, cleanUpSwapChainRequirements);

	free(swapChainImageViews);

	vkDestroyBuffer(logicalDevice, indexBuffer, NULL);
	vkFreeMemory(logicalDevice, indexBufferMemory, NULL);

	vkDestroyBuffer(logicalDevice, vertexBuffer, NULL);
	vkFreeMemory(logicalDevice, vertexBufferMemory, NULL);

	vkDestroyDevice(logicalDevice, NULL);
	vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, NULL);
	vkDestroyInstance(vulkanInstance, NULL);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

