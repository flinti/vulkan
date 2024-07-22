#include "GraphicsPipeline.h"
#include "Device.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "Material.h"
#include "RenderObject.h"
#include "Vertex.h"
#include "RenderPass.h"
#include "VkHelpers.h"

#include <spdlog/fmt/fmt.h>
#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>

GraphicsPipeline::GraphicsPipeline(
        Device &device,
        const RenderPass &renderPass,
		const Material &material
) : device(device),
	renderPass(renderPass),
	material(material),
	materialDescriptorSetLayout(
		device.getObjectCache().getDescriptorSetLayout(material.getDescriptorSetLayoutBindings())
	)
{
	createPipelineLayout();
	createPipeline(
		material.getVertexShaderResource(),
		material.getFragmentShaderResource()
	);
}

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline &&other)
	: device(other.device),
	renderPass(other.renderPass),
	material(other.material),
	pipelineLayout(other.pipelineLayout),
	pipeline(other.pipeline),
	materialDescriptorSetLayout(other.materialDescriptorSetLayout)
{
	other.pipelineLayout = VK_NULL_HANDLE;
	other.pipeline = VK_NULL_HANDLE;
}

GraphicsPipeline::~GraphicsPipeline()
{
	vkDestroyPipeline(device.getDeviceHandle(), pipeline, nullptr);
	vkDestroyPipelineLayout(device.getDeviceHandle(), pipelineLayout, nullptr);
}

const Material &GraphicsPipeline::getMaterial() const
{
	return material;
}

const DescriptorSetLayout &GraphicsPipeline::getMaterialDescriptorSetLayout() const
{
	return materialDescriptorSetLayout;
}

void GraphicsPipeline::bind(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void GraphicsPipeline::bindDescriptorSet(
	VkCommandBuffer commandBuffer,
	DescriptorSetIndex index,
	const DescriptorSet &set
) {
	VkDescriptorSet setHandle = set.getHandle();
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pipelineLayout, 
		static_cast<uint32_t>(index), 
		1, 
		&setHandle, 
		0, 
		nullptr
	);
}

void GraphicsPipeline::pushConstants(VkCommandBuffer commandBuffer, const void *data, size_t size)
{
	vkCmdPushConstants(commandBuffer, 
		pipelineLayout, 
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
		0,
		size, 
		data
	);
}


std::vector<VkDescriptorSetLayoutBinding> GraphicsPipeline::createGlobalUniformDataLayoutBindings()
{
	return std::vector<VkDescriptorSetLayoutBinding>{
		VkDescriptorSetLayoutBinding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		},
	};
}

void GraphicsPipeline::createPipelineLayout()
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PushConstants);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = {
		device.getObjectCache().getDescriptorSetLayout(
			RenderObject::getGlobalUniformDataLayoutBindings()
		).getHandle(),
		materialDescriptorSetLayout.getHandle()
	};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	VK_ASSERT(vkCreatePipelineLayout(
		device.getDeviceHandle(), 
		&pipelineLayoutInfo, 
		nullptr, 
		&pipelineLayout
	));
}

VkShaderModule GraphicsPipeline::createShaderModule(const std::vector<std::byte> &shader)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shader.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shader.data());

	VkShaderModule shaderModule = VK_NULL_HANDLE;
	VkResult result = vkCreateShaderModule(device.getDeviceHandle(), &createInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS) {
		throw std::runtime_error(fmt::format("vkCreateShaderModule failed with code {}", (int32_t) result));
	}

	return shaderModule;
}

void GraphicsPipeline::createPipeline(const ShaderResource &vertexShader, const ShaderResource &fragmentShader)
{
	// shaders
	VkShaderModule vertShaderModule = createShaderModule(vertexShader);
    VkShaderModule fragShaderModule = createShaderModule(fragmentShader);

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfos = {};
	shaderStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageInfos[0].module = vertShaderModule;
	shaderStageInfos[0].pName = "main";

	shaderStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageInfos[1].module = fragShaderModule;
	shaderStageInfos[1].pName = "main";


	// fixed function setup
	std::array<VkDynamicState, 2> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.f;
	rasterizer.depthBiasClamp = 0.f;
	rasterizer.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.f;
	colorBlending.blendConstants[1] = 0.f;
	colorBlending.blendConstants[2] = 0.f;
	colorBlending.blendConstants[3] = 0.f;

	// depth and stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStageInfos.size();
	pipelineInfo.pStages = shaderStageInfos.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass.getHandle();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	VK_ASSERT(vkCreateGraphicsPipelines(
		device.getDeviceHandle(), 
		VK_NULL_HANDLE, 
		1, 
		&pipelineInfo, 
		nullptr, 
		&pipeline
	));

	vkDestroyShaderModule(device.getDeviceHandle(), vertShaderModule, nullptr);
	vkDestroyShaderModule(device.getDeviceHandle(), fragShaderModule, nullptr);
}