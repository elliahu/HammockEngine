#pragma once
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
	struct TransformComponent
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
	struct DescriptorSetComponent
	{
		VkDescriptorSet set{};
	};

	/*
		glTFComponent
		This component represents all the data loaded from the source glTF 2.0 file
	*/
	struct GltfComponent
	{
		std::shared_ptr<Gltf> glTF;
	};

	/*
		PointLightComponent
		If this component is set, model should NOT be set
		Set this component to make a spot light from the GameObject
		color property is used to make a colored light
	*/
	struct PointLightComponent
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
	struct DirectionalLightComponent
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
	struct BoundingBoxComponent
	{
		struct BoundingBoxAxis
		{
			float min;
			float max;
		};

		BoundingBoxAxis x;
		BoundingBoxAxis y;
		BoundingBoxAxis z;
	};


	/*
		GameObject
		Class representing a single entity in the game world
		Uses Entity Component System architecture (sort of)
	*/
	class GameObject
	{
	public:
		using id_t = unsigned int;
		using Map = std::unordered_map<id_t, GameObject>;

		static GameObject createGameObject()
		{
			static id_t currentId = 0;
			return GameObject(currentId++);
		}

		static GameObject createPointLight(
			float intensity = 10.f, 
			float radius = 0.1f, 
			glm::vec3 color = glm::vec3(1));

		static GameObject createDirectionalLight(
			glm::vec3 position = glm::vec3(10,-10,-10), glm::vec3 target = glm::vec3(0),
			glm::vec4 directionalLightColor = glm::vec4( 1.0f, 1.0f, 1.0, 1.0f ),
			float nearClip = 0.1f, float farClip = 100.f, float fov = 60.f);

		static GameObject createFromGLTF(std::string filepath, Device& device, Gltf::Config config = {});

		void fitBoundingBox(
			BoundingBoxComponent::BoundingBoxAxis x,
			BoundingBoxComponent::BoundingBoxAxis y,
			BoundingBoxComponent::BoundingBoxAxis z);

		GameObject(const GameObject&) = delete;
		GameObject& operator =(const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator=(GameObject&&) = default;


		const id_t getId() { return id; }
		const std::string getName() { return name + " #" + std::to_string(id); }
		void setName(std::string name) { this->name = name; }

		// Components
		glm::vec3 colorComponent{}; 

		TransformComponent transformComponent{}; // every game object has this

		std::unique_ptr<DescriptorSetComponent> descriptorSetComponent = nullptr;

		std::unique_ptr<PointLightComponent> pointLightComponent = nullptr;

		std::unique_ptr<BoundingBoxComponent> boundingBoxComponent = nullptr;

		std::unique_ptr<DirectionalLightComponent> directionalLightComponent = nullptr;

		std::unique_ptr<GltfComponent> glTFComponent = nullptr;

	private:
		GameObject(id_t objId) : id{ objId } {}

		id_t id;
		std::string name = "GameObject";
	};
}