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
    return (1.0f / scalingFactor()) * glm::mat3(
        { transform[0][0], transform[1][0] , transform[2][0] },
        { transform[0][1], transform[1][1] , transform[2][1] },
        { transform[0][2], transform[1][2] , transform[2][2] });
}

glm::vec3 Hmck::Entity::rotation()
{
    glm::mat3 rotationMatrix = rotationMat();
    glm::quat quaternion = glm::quat(rotationMatrix);
    glm::vec3 eulerAngles = glm::eulerAngles(quaternion);
    return eulerAngles;
}

void Hmck::Entity::translate(glm::vec3 position)
{
    transform[3][0] = position.x;
    transform[3][1] = position.y;
    transform[3][2] = position.z;
}

void Hmck::Entity::rotate(glm::vec3 rotation)
{
    transform = glm::rotate(transform, rotation[0], { 1,0,0 });
    transform = glm::rotate(transform, rotation[1], { 0,1,0 });
    transform = glm::rotate(transform, rotation[2], { 0,0,1 });
}
