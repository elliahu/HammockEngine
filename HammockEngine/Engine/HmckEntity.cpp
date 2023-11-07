#include "HmckEntity.h"

Hmck::Id Hmck::Entity::currentId = 0;

float Hmck::Entity::scalingFactor()
{
    return sqrt(transform[0][0] * transform[0][0] + transform[1][0] * transform[1][0] + transform[2][0] * transform[2][0]);
}

glm::vec3 Hmck::Entity::translation()
{
    return glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
}

glm::mat3 Hmck::Entity::rotationMat()
{
    return glm::mat3(transform);
}

glm::vec3 Hmck::Entity::rotation()
{
    glm::mat3 rotationMatrix = rotationMat();
    glm::quat quaternion = glm::quat(rotationMatrix);
    glm::vec3 eulerAngles = glm::eulerAngles(quaternion);
    return eulerAngles;
}

glm::vec3 Hmck::Entity::scale()
{
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;
    
    decompose(scale, rotation, translation, skew, perspective);

    return scale;
}

void Hmck::Entity::decompose(glm::vec3& scale, glm::quat& rotation, glm::vec3& translation, glm::vec3& skew, glm::vec4& perspective)
{
    glm::decompose(transform, scale, rotation, translation, skew, perspective);
}

void Hmck::Entity::translate(glm::vec3 position)
{
    transform[3][0] = position.x;
    transform[3][1] = position.y;
    transform[3][2] = position.z;
}

void Hmck::Entity::rotate(glm::vec3 rotation)
{
    auto rx = glm::rotate(glm::mat4(1.0f), rotation.x, { 1,0,0 });
    auto ry = glm::rotate(glm::mat4(1.0f), rotation.y, { 0,1,0 });
    auto rz = glm::rotate(glm::mat4(1.0f), rotation.z, { 0,0,1 });

    transform = rx * ry * rz * transform;
}

void Hmck::Entity::scale(glm::vec3 scale)
{
    transform = glm::scale(transform, scale);
}
