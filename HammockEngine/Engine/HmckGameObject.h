#pragma once

#include "HmckMesh.h"
#include "HmckMaterial.h"
#include "HmckGLTF.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <memory>
#include <unordered_map>


namespace Hmck
{
	// good reading:
	// https://www.raywenderlich.com/2806-introduction-to-component-based-architecture-in-games
	// https://austinmorlan.com/posts/entity_component_system/

	/*
		GameObject: represents a general-purpose in-game object
		Component: labels GameObject as possessing a particular aspect, and holds the data needed to model that aspect
		System: A system is a process which acts on all GameObjects with the desired components.
	*/

	/*
		TransformComponent
	*/
	struct HmckTransformComponent
	{
		glm::vec3 translation{ 0.0f };
		glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
		glm::vec3 rotation{ 0.0f };

		// Matrix corrsponds to Translate * Ry * Rx * Rz * Scale
		// Rotations correspond to Tait-bryan angles of Y(1), X(2), Z(3)
		// https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
		glm::mat4 mat4();
		glm::mat3 normalMatrix();
	};

	/*
		DescriptorSetComponetn
		This component is used to store the descriptor set of the enitiy
	*/
	struct HmckDescriptorSetComponent
	{
		VkDescriptorSet set{};
	};

	/*
		 WavefrontObjComponent
		 Represents .obj mesh and .mtl material of object
		 Cannot be used when HmckGLTFComponent is used
	*/
	struct HmckWavefrontObjComponent
	{
		std::shared_ptr<HmckMaterial> material;
		std::shared_ptr<HmckMesh> mesh;
	};

	/*
		glTFComponent
		This component represents all the data loaded from the source glTF 2.0 file
		Cannot be used when HmckWavefrontObjComponent is used
	*/
	struct HmckGLTFComponent
	{
		std::shared_ptr<HmckGLTF> glTF;
	};

	/*
		PointLightComponent
		If this component is set, model should NOT be set
		Set this component to make a spot light from the GameObject
		color property is used to make a colored light
	*/
	struct HmckPointLightComponent
	{
		float lightIntensity = 1.0f;
		float quadraticTerm = 1.0f;
		float linearTerm = .0f;
		float constantTerm = .0f;
	};

	/*
		DirectionalLightComponent
		If this component is set, model should NOT be set
		Set this component to make a dorectional light from the GameObject
	*/
	struct HmckDirectionalLightComponent
	{
		float lightIntensity = 1.0f;
		float fov = 50.0f;
		float _near = 0.1f;
		float _far = 100.f;
		glm::vec3 target{0};
	};
	

	/*
		BoundingBoxComponent
		Axis-Aligned Bounding Box is used for collision detection
		TODO make sure to resize when scale changes
	*/
	struct HmckBoundingBoxComponent
	{
		struct HmckBoundingBoxAxis
		{
			float min;
			float max;
		};

		HmckBoundingBoxAxis x;
		HmckBoundingBoxAxis y;
		HmckBoundingBoxAxis z;
	};


	/*
		GameObject
		Class representing a single entity in the game world
		Uses Entity Component System architecture (sort of)
	*/
	class HmckGameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, HmckGameObject>;

		static HmckGameObject createGameObject()
		{
			static id_t currentId = 0;
			return HmckGameObject(currentId++);
		}

		static HmckGameObject createPointLight(
			float intensity = 10.f, 
			float radius = 0.1f, 
			glm::vec3 color = glm::vec3(1));

		static HmckGameObject createDirectionalLight(
			glm::vec3 position = glm::vec3(10,-10,-10), glm::vec3 target = glm::vec3(0),
			glm::vec4 directionalLightColor = glm::vec4( 1.0f, 1.0f, 1.0, 1.0f ),
			float nearClip = 0.1f, float farClip = 100.f, float fov = 180.f);

		static HmckGameObject createFromGLTF(std::string filepath, HmckDevice& device, HmckGLTF::Config config = {});

		void fitBoundingBox(
			HmckBoundingBoxComponent::HmckBoundingBoxAxis x,
			HmckBoundingBoxComponent::HmckBoundingBoxAxis y,
			HmckBoundingBoxComponent::HmckBoundingBoxAxis z);
		void fitBoundingBox(HmckMesh::MeshInfo& modelInfo);

		void setMtlMaterial(std::shared_ptr<HmckMaterial>& material);
		void setObjMesh(std::shared_ptr<HmckMesh>& model);
		
		void bindMtlDescriptorSet(
			std::unique_ptr<HmckDescriptorPool>& pool, 
			std::unique_ptr<HmckDescriptorSetLayout>& setLayout);

		HmckGameObject(const HmckGameObject&) = delete;
		HmckGameObject& operator =(const HmckGameObject&) = delete;
		HmckGameObject(HmckGameObject&&) = default;
		HmckGameObject& operator=(HmckGameObject&&) = default;


		const id_t getId() { return id; }
		const std::string getName() { return name + " #" + std::to_string(id); }
		void setName(std::string name) { this->name = name; }

		// Components
		glm::vec3 colorComponent{}; 

		HmckTransformComponent transformComponent{}; // every game object has this

		std::unique_ptr<HmckDescriptorSetComponent> descriptorSetComponent = nullptr;

		std::unique_ptr<HmckPointLightComponent> pointLightComponent = nullptr;

		std::unique_ptr<HmckBoundingBoxComponent> boundingBoxComponent = nullptr;

		std::unique_ptr<HmckDirectionalLightComponent> directionalLightComponent = nullptr;

		std::unique_ptr<HmckWavefrontObjComponent> wavefrontObjComponent = nullptr;

		std::unique_ptr<HmckGLTFComponent> glTFComponent = nullptr;

	private:
		HmckGameObject(id_t objId) : id{ objId } {}

		id_t id;
		std::string name = "GameObject";
	};
}