#ifndef __UI_EDITOR_ABSTRACT_PROPERTY_H__
#define __UI_EDITOR_ABSTRACT_PROPERTY_H__

#include "Base/BaseObject.h"

class PackageSerializer;

class AbstractProperty : public DAVA::BaseObject
{
public:
    enum ePropertyType
    {
        TYPE_NONE,
        TYPE_HEADER,
        TYPE_VARIANT,
        TYPE_ENUM,
        TYPE_FLAGS,
    };

    enum eEditFrags
    {
        EF_NONE = 0,
        EF_CAN_RESET = 0x01,
        EF_ADD_REMOVE = 0x02,
        EF_INHERITED = 0x04
    };
    
    enum eCloneType
    {
        CT_INHERIT,
        CT_COPY
    };
    
public:
    AbstractProperty();
protected:
    virtual ~AbstractProperty();

public:
    AbstractProperty *GetParent() const;
    void SetParent(AbstractProperty *parent);
    
    virtual int GetCount() const = 0;
    virtual AbstractProperty *GetProperty(int index) const = 0;
    virtual int GetIndex(AbstractProperty *property) const;

    virtual void Refresh();
    virtual AbstractProperty *FindPropertyByPrototype(AbstractProperty *prototype);
    virtual bool HasChanges() const;
    virtual void Serialize(PackageSerializer *serializer) const = 0;

    virtual const DAVA::String &GetName() const = 0;
    virtual ePropertyType GetType() const = 0;
    virtual DAVA::uint32 GetEditFlag() const { return EF_NONE; };

    virtual bool IsReadOnly() const;
    virtual bool CanRemove() const {return false; }
    virtual bool CanCreate() const {return false; }

    virtual DAVA::VariantType GetValue() const;
    virtual void SetValue(const DAVA::VariantType &newValue);
    virtual DAVA::VariantType GetDefaultValue() const;
    virtual void SetDefaultValue(const DAVA::VariantType &newValue);
    virtual const EnumMap *GetEnumMap() const;
    virtual void ResetValue();
    virtual bool IsReplaced() const;

    AbstractProperty *GetRootProperty();
    const AbstractProperty *GetRootProperty() const;

private:
    AbstractProperty *parent;
};


#endif // __UI_EDITOR_ABSTRACT_PROPERTY_H__
