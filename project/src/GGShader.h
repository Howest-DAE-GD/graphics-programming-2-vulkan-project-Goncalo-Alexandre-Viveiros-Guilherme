#include <string>
#include <vulkan/vulkan_core.h>
#include <vector>
#include <fstream>

namespace GG
{
	class Shader
	{
	public:
		Shader(const std::string& shaderPath, VkDevice device)
		{
			m_Device = device;
			CreateShaderModule(ReadFile(shaderPath), device);
		}

		~Shader()
		{
			vkDestroyShaderModule(m_Device, m_ShaderModule, nullptr);
		}
	
		VkShaderModule GetShaderModule()
		{
			return m_ShaderModule;
		}
	
	private:
	
		std::vector<char> ReadFile(const std::string& filename)
		{
			std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
			if (!file.is_open()) {
				throw std::runtime_error("failed to open file!");
			}
	
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);
	
			file.seekg(0);
			file.read(buffer.data(), fileSize);
	
			file.close();
	
			return buffer;
		}
	
		void CreateShaderModule(const std::vector<char>& code, VkDevice& device)
		{
			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	
		
			if (vkCreateShaderModule(device, &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create shader module!");
			}
		}
	
		VkShaderModule m_ShaderModule;
		VkDevice m_Device;
	};
}