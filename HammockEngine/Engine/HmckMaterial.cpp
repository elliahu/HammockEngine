#include "HmckMaterial.h"

#ifndef MATERIALS_DIR
#define MATERIALS_DIR "../../Resources/Materials/"
#endif // !MATERIALS_DIR

Hmck::HmckMaterial::HmckMaterial(HmckDevice& device): hmckDevice{device} {}

std::unique_ptr<Hmck::HmckMaterial> Hmck::HmckMaterial::createMaterial(HmckDevice& hmckDevice, HmckCreateMaterialInfo& materialInfo)
{
	std::unique_ptr<HmckMaterial> material = std::make_unique<HmckMaterial>(hmckDevice);
	material->createMaterial(materialInfo);
	return material;
}

void Hmck::HmckMaterial::createMaterial(HmckCreateMaterialInfo& materialInfo)
{
	// TODO check if paths are provided
	// TODO load default value if not
	// TODO make this load once and reuse
	HmckCreateMaterialInfo defaultInfo{
		std::string(MATERIALS_DIR) + "empty_white.jpg", // color
		std::string(MATERIALS_DIR) + "empty_normal.jpg", // normal
		std::string(MATERIALS_DIR) + "empty_black.jpg", // roughness
		std::string(MATERIALS_DIR) + "empty_black.jpg", // metalness
		std::string(MATERIALS_DIR) + "empty_white.jpg", // ao
		std::string(MATERIALS_DIR) + "empty_white.jpg" // displacement
	};
	

	color = std::make_unique<HmckTexture2D>();
	color->loadFromFile(materialInfo.color.length() != 0 ? materialInfo.color : defaultInfo.color, hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	color->createSampler(hmckDevice);

	// normal
	normal = std::make_unique<HmckTexture2D>();
	normal->loadFromFile(materialInfo.normal.length() != 0 ? materialInfo.normal : defaultInfo.normal, hmckDevice, VK_FORMAT_R8G8B8A8_UNORM); // !!!
	normal->createSampler(hmckDevice);

	// roughness
	roughness = std::make_unique<HmckTexture2D>();
	roughness->loadFromFile(materialInfo.roughness.length() != 0 ? materialInfo.roughness: defaultInfo.roughness, hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	roughness->createSampler(hmckDevice);

	// metalness
	metalness = std::make_unique<HmckTexture2D>();
	metalness->loadFromFile(materialInfo.metalness.length() != 0 ? materialInfo.metalness: defaultInfo.metalness, hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	metalness->createSampler(hmckDevice);

	// ambient occlusion
	ambientOcclusion = std::make_unique<HmckTexture2D>();
	ambientOcclusion->loadFromFile(materialInfo.ambientOcclusion.length() != 0 ? materialInfo.ambientOcclusion: defaultInfo.ambientOcclusion, hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	ambientOcclusion->createSampler(hmckDevice);

	// displacement
	displacement = std::make_unique<HmckTexture2D>();
	displacement->loadFromFile(materialInfo.displacement.length() != 0 ? materialInfo.displacement: defaultInfo.displacement, hmckDevice, VK_FORMAT_R8G8B8A8_SRGB);
	displacement->createSampler(hmckDevice);
}

Hmck::HmckMaterial::~HmckMaterial()
{
	destroy();
}

void Hmck::HmckMaterial::destroy()
{
	if (color != nullptr)
	{
		color->destroy(hmckDevice);
	}
	
	if (normal != nullptr)
	{
		normal->destroy(hmckDevice);
	}
	
	if (roughness != nullptr)
	{
		roughness->destroy(hmckDevice);
	}
	
	if (metalness != nullptr)
	{
		metalness->destroy(hmckDevice);
	}
	
	if (ambientOcclusion != nullptr)
	{
		ambientOcclusion->destroy(hmckDevice);
	}
	
	if (displacement != nullptr)
	{
		displacement->destroy(hmckDevice);
	}
}





