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
		std::string(MATERIALS_DIR) + "empty_black.jpg", // roughnessMetalness
	};

	color = std::make_unique<HmckTexture2D>();
	color->loadFromFile(materialInfo.color.length() != 0 ? materialInfo.color : defaultInfo.color, hmckDevice, VK_FORMAT_R8G8B8A8_UNORM);
	color->createSampler(hmckDevice);
	color->updateDescriptor();

	// normal
	normal = std::make_unique<HmckTexture2D>();
	normal->loadFromFile(materialInfo.normal.length() != 0 ? materialInfo.normal : defaultInfo.normal, hmckDevice, VK_FORMAT_R8G8B8A8_UNORM); // !!!
	normal->createSampler(hmckDevice);
	normal->updateDescriptor();

	// roughness
	occlusionRoughnessMetalness = std::make_unique<HmckTexture2D>();
	occlusionRoughnessMetalness->loadFromFile(materialInfo.occlusionRoughnessMetalness.length() != 0 ? materialInfo.occlusionRoughnessMetalness : defaultInfo.occlusionRoughnessMetalness, hmckDevice, VK_FORMAT_R8G8B8A8_UNORM);
	occlusionRoughnessMetalness->createSampler(hmckDevice);
	occlusionRoughnessMetalness->updateDescriptor();
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
	
	if (occlusionRoughnessMetalness != nullptr)
	{
		occlusionRoughnessMetalness->destroy(hmckDevice);
	}
}





