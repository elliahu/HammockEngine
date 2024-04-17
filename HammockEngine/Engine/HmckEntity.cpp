#include "HmckEntity.h"

Hmck::Id Hmck::Entity::currentId = 0;

std::shared_ptr<Hmck::Entity> Hmck::Entity::getChild(Id id)
{
    for (auto child : children)
    {
        if (child->id == id)
            return child;
        
        auto found = child->getChild(id);
        
        if (found != nullptr)
        {
            return found;
        }
    }
    return nullptr;
}
