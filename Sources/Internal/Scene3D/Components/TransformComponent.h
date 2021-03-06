#pragma once

#include "Base/BaseTypes.h"
#include "Reflection/Reflection.h"
#include "Scene3D/Systems/TransformSystem.h"
#include "Entity/Component.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class Entity;

class TransformComponent : public Component
{
public:
    inline Matrix4* GetWorldTransformPtr();
    inline const Matrix4& GetWorldTransform();
    inline const Matrix4& GetLocalTransform();
    Matrix4& ModifyLocalTransform();

    void SetWorldTransform(const Matrix4* transform);
    void SetLocalTransform(const Matrix4* transform);
    void SetParent(Entity* node);

    Component* Clone(Entity* toEntity) override;
    void Serialize(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Deserialize(KeyedArchive* archive, SerializationContext* serializationContext) override;

private:
    Matrix4 localMatrix = Matrix4::IDENTITY;
    Matrix4 worldMatrix = Matrix4::IDENTITY;
    Matrix4* parentMatrix = nullptr;
    Entity* parent = nullptr; //Entity::parent should be removed

    friend class TransformSystem;

    DAVA_VIRTUAL_REFLECTION(TransformComponent, Component);
};

const Matrix4& TransformComponent::GetWorldTransform()
{
    return worldMatrix;
}

const Matrix4& TransformComponent::GetLocalTransform()
{
    return localMatrix;
}

Matrix4* TransformComponent::GetWorldTransformPtr()
{
    return &worldMatrix;
}
}
