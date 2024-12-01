#include "OrbitalMovementController.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "utils/HmckLogger.h"
#include "utils/HmckUtils.h"

void Hmck::OrbitalMovementController::freeOrbit(std::shared_ptr<Entity> entity, const glm::vec3 &center,
                                            Window &window, float frameTime , bool lookAtCenter) {

    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_LSHIFT)) currentStepSize = stepSizeSlow;
    else currentStepSize = stepSizeNormal;

    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_D)) currentAzimuth -= currentStepSize * frameTime;
    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_A)) currentAzimuth += currentStepSize * frameTime;
    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_W)) currentElevation += currentStepSize * frameTime;
    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_S)) currentElevation -= currentStepSize * frameTime;

    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_UP)) currentRadius -= currentStepSize * frameTime;
    if (window.getInputHandler().isKeyboardKeyDown(HMCK_KEY_DOWN)) currentRadius += currentStepSize * frameTime;


    currentElevation = glm::clamp(currentElevation, minElevation, maxElevation);
    currentRadius = glm::clamp(currentRadius, minRadius, maxRadius);


    // calculate the new orbital position
    glm::vec3 newOrbit = Math::orbitalPosition(center, currentRadius, currentAzimuth, currentElevation);
    entity->transform.translation = newOrbit;


    // look at the center
    if (lookAtCenter) {
        entity->transform.rotation = Math::lookAtEulerAngles(entity->transform.translation, center,{0.f, 1.f, 0.f});
    }
}
