#ifndef __DAVAENGINE_YAML_NODE_H__
#define __DAVAENGINE_YAML_NODE_H__

#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Base/FastName.h"

#include "Reflection/ReflectedStructure.h"

namespace DAVA
{
class KeyedArchive;
class VariantType;
/**
    \ingroup yaml
    \brief this class is base yaml node that is used for everything connected with yaml
*/
class YamlNode final
: public BaseObject
{
public:
    enum eType
    {
        TYPE_STRING = 0,
        TYPE_ARRAY,
        TYPE_MAP,
    };

    enum eStringRepresentation //data represent style for TYPE_STRING for storing in file
    {
        SR_PLAIN_REPRESENTATION, //plain represent style
        SR_DOUBLE_QUOTED_REPRESENTATION, //data represent in double-quoted
    };

    enum eArrayRepresentation //data represent style for TYPE_ARRAY for storing in file
    {
        AR_BLOCK_REPRESENTATION, //data represent in multi-line with mark "- "
        AR_FLOW_REPRESENTATION, //data represent one line in square brackets []
    };

    enum eMapRepresentation //data represent style for TYPE_MAP for storing in file
    {
        MR_BLOCK_REPRESENTATION, //data represent in multi-line
        MR_FLOW_REPRESENTATION, //data represent one line in braces {}
    };

protected:
    virtual ~YamlNode();

public:
    YamlNode(eType type);
    static RefPtr<YamlNode> CreateStringNode();
    static RefPtr<YamlNode> CreateArrayNode(eArrayRepresentation representation = AR_FLOW_REPRESENTATION);
    static RefPtr<YamlNode> CreateMapNode(bool orderedSave = true, eMapRepresentation valRepresentation = MR_BLOCK_REPRESENTATION, eStringRepresentation keyRepresentation = SR_PLAIN_REPRESENTATION);

    eType GetType() const
    {
        return type;
    }

    // These functions work only if type of node is string
    bool AsBool() const;
    int32 AsInt() const; //left for old code
    int32 AsInt32() const;
    uint32 AsUInt32() const;
    int64 AsInt64() const;
    uint64 AsUInt64() const;
    float32 AsFloat() const;
    FastName AsFastName() const;
    const String& AsString() const;
    WideString AsWString() const;

    //These functions work only if type of node is array
    const Vector<RefPtr<YamlNode>>& AsVector() const;
    Vector2 AsPoint() const; //Dizz: this one exists cause of Boroda
    Vector2 AsVector2() const;
    Vector3 AsVector3() const;
    Vector4 AsVector4() const;
    Color AsColor() const;
    Rect AsRect() const;

    //These functions work only if type of node is map
    const UnorderedMap<String, RefPtr<YamlNode>>& AsMap() const;
    VariantType AsVariantType() const;

    VariantType AsVariantType(const InspMember* insp) const;
    Any AsAny(const ReflectedStructure::Field* field) const;
    Any AsAny(const Reflection& ref) const;

    //These functions work only if type of node is array or map
    uint32 GetCount() const;
    const YamlNode* Get(uint32 index) const;

    //These functions work only if type of node is map
    const YamlNode* Get(const String& name) const;
    const String& GetItemKeyName(uint32 index) const;

    // "Setters". These methods set data in string node.
    inline void Set(bool value);
    inline void Set(int32 value);
    inline void Set(float32 value);
    inline void Set(const char* value);
    inline void Set(const String& value);
    inline void Set(const WideString& value);

    // "Adders". These methods add data to array node.
    inline void Add(bool value);
    inline void Add(int32 value);
    inline void Add(float32 value);
    inline void Add(const char* value);
    inline void Add(const String& value);
    inline void Add(const WideString& value);
    inline void Add(const Vector2& value);
    inline void Add(const Vector3& value);
    inline void Add(const Vector4& value);
    inline void Add(const VariantType& value);
    DAVA_DEPRECATED(inline void Add(YamlNode* value));
    inline void Add(const RefPtr<YamlNode>& value);

    // "Adders". These methods ADD node to the map, even in case the node with the same name is added.
    inline void Add(const String& name, bool value);
    inline void Add(const String& name, int32 value);
    inline void Add(const String& name, float32 value);
    inline void Add(const String& name, const char* value);
    inline void Add(const String& name, const String& value);
    inline void Add(const String& name, const WideString& value);
    inline void Add(const String& name, const Vector2& value);
    inline void Add(const String& name, const Vector3& value);
    inline void Add(const String& name, const Vector4& value);
    inline void Add(const String& name, const Color& value);
    inline void Add(const String& name, const VariantType& value);
    inline void Add(const String& name, VariantType* varType);
    DAVA_DEPRECATED(inline void Add(const String& name, YamlNode* value));
    inline void Add(const String& name, const RefPtr<YamlNode>& value);

    // "Setters". These methods REPLACE node in the map in case the node with the same name exists.
    inline void Set(const String& name, bool value);
    inline void Set(const String& name, int32 value);
    inline void Set(const String& name, float32 value);
    inline void Set(const String& name, const char* value);
    inline void Set(const String& name, const String& value);
    inline void Set(const String& name, const WideString& value);
    inline void Set(const String& name, const Vector2& value);
    inline void Set(const String& name, const Vector3& value);
    inline void Set(const String& name, const Vector4& value);
    inline void Set(const String& name, const VariantType& value);
    inline void Set(const String& name, VariantType* varType);
    DAVA_DEPRECATED(inline void Set(const String& name, YamlNode* value));
    inline void Set(const String& name, const RefPtr<YamlNode>& value);

    // Specific adder for the whole node.
    DAVA_DEPRECATED(inline void AddNodeToMap(const String& name, YamlNode* value));
    inline void AddNodeToMap(const String& name, const RefPtr<YamlNode>& value);
    // Specific setter for the whole node.
    DAVA_DEPRECATED(inline void SetNodeToMap(const String& name, YamlNode* value));
    inline void SetNodeToMap(const String& name, const RefPtr<YamlNode>& value);
    // Setters for Map/Array nodes.
    DAVA_DEPRECATED(inline void AddNodeToArray(YamlNode* value));
    inline void AddNodeToArray(const RefPtr<YamlNode>& value);

    // Remove node value from map
    void RemoveNodeFromMap(const String& name);
    eStringRepresentation GetStringRepresentation() const;
    void SetStringRepresentation(eStringRepresentation rep);
    eArrayRepresentation GetArrayRepresentation() const;
    eMapRepresentation GetMapRepresentation() const;
    eStringRepresentation GetMapKeyRepresentation() const;
    bool GetMapOrderRepresentation() const;

protected:
    static RefPtr<YamlNode> CreateNodeFromVariantType(const VariantType& varType);

    static eType VariantTypeToYamlNodeType(VariantType::eVariantType variantType);

    bool InitStringFromVariantType(const VariantType& varType);
    bool InitArrayFromVariantType(const VariantType& varType);
    bool InitMapFromVariantType(const VariantType& varType);

    // Internal setters, which can both add or replace value in the map.
    void InternalSetToString(const VariantType& value);
    void InternalSetToString(const String& value);
    void InternalAddToMap(const String& name, const VariantType& value, bool rewritePreviousValue);
    void InternalAddToMap(const String& name, const String& value, bool rewritePreviousValue);
    void InternalAddToArray(const VariantType& value);
    void InternalAddToArray(const String& value);

    void InternalAddNodeToMap(const String& name, const RefPtr<YamlNode>& node, bool rewritePreviousValue);
    void InternalAddNodeToArray(const RefPtr<YamlNode>& node);

    void InternalSetString(const String& value, eStringRepresentation style);
    void InternalSetMatrix(const float32* array, uint32 dimension);
    void InternalSetVector(const float32 array[], uint32 dimension);
    void InternalSetByteArray(const uint8* byteArray, int32 byteArraySize);
    void InternalSetKeyedArchive(KeyedArchive* archive);

private:
    const eType type;
    struct ObjectString
    {
        String nwStringValue;
        eStringRepresentation style;
    };

    struct ObjectArray
    {
        Vector<RefPtr<YamlNode>> array;
        eArrayRepresentation style;
    };

    struct ObjectMap
    {
        UnorderedMap<String, RefPtr<YamlNode>> ordered;
        Vector<std::pair<String, RefPtr<YamlNode>>> unordered;
        eMapRepresentation style;
        eStringRepresentation keyStyle;
        bool orderedSave;
    };

    union
    {
        ObjectString* objectString;
        ObjectArray* objectArray;
        ObjectMap* objectMap;
    };
};

inline void YamlNode::Set(bool value)
{
    InternalSetToString(VariantType(value));
}
inline void YamlNode::Set(int32 value)
{
    InternalSetToString(VariantType(value));
}
inline void YamlNode::Set(float32 value)
{
    InternalSetToString(VariantType(value));
}
inline void YamlNode::Set(const char* value)
{
    InternalSetToString(value);
}
inline void YamlNode::Set(const String& value)
{
    InternalSetToString(value);
}
inline void YamlNode::Set(const WideString& value)
{
    InternalSetToString(VariantType(value));
}

inline void YamlNode::Add(bool value)
{
    InternalAddToArray(VariantType(value));
}
inline void YamlNode::Add(int32 value)
{
    InternalAddToArray(VariantType(value));
}
inline void YamlNode::Add(float32 value)
{
    InternalAddToArray(VariantType(value));
}

inline void YamlNode::Add(const char* value)
{
    InternalAddToArray(value);
}
inline void YamlNode::Add(const String& value)
{
    InternalAddToArray(value);
}
inline void YamlNode::Add(const WideString& value)
{
    InternalAddToArray(VariantType(value));
}

inline void YamlNode::Add(const Vector2& value)
{
    InternalAddToArray(VariantType(value));
}
inline void YamlNode::Add(const Vector3& value)
{
    InternalAddToArray(VariantType(value));
}
inline void YamlNode::Add(const Vector4& value)
{
    InternalAddToArray(VariantType(value));
}

inline void YamlNode::Add(const VariantType& value)
{
    InternalAddToArray(value);
}

inline void YamlNode::Add(YamlNode* value)
{
    InternalAddNodeToArray(RefPtr<YamlNode>(value));
}

inline void YamlNode::Add(const RefPtr<YamlNode>& value)
{
    InternalAddNodeToArray(value);
}

inline void YamlNode::Add(const String& name, bool value)
{
    InternalAddToMap(name, VariantType(value), false);
}
inline void YamlNode::Add(const String& name, int32 value)
{
    InternalAddToMap(name, VariantType(value), false);
}
inline void YamlNode::Add(const String& name, float32 value)
{
    InternalAddToMap(name, VariantType(value), false);
}

inline void YamlNode::Add(const String& name, const char* value)
{
    InternalAddToMap(name, value, false);
}
inline void YamlNode::Add(const String& name, const String& value)
{
    InternalAddToMap(name, value, false);
}
inline void YamlNode::Add(const String& name, const WideString& value)
{
    InternalAddToMap(name, VariantType(value), false);
}

inline void YamlNode::Add(const String& name, const Vector2& value)
{
    InternalAddToMap(name, VariantType(value), false);
}
inline void YamlNode::Add(const String& name, const Vector3& value)
{
    InternalAddToMap(name, VariantType(value), false);
}
inline void YamlNode::Add(const String& name, const Vector4& value)
{
    InternalAddToMap(name, VariantType(value), false);
}
inline void YamlNode::Add(const String& name, const Color& value)
{
    InternalAddToMap(name, VariantType(value), false);
}

inline void YamlNode::Add(const String& name, const VariantType& value)
{
    InternalAddToMap(name, value, false);
}
inline void YamlNode::Add(const String& name, VariantType* varType)
{
    InternalAddToMap(name, *varType, false);
}

inline void YamlNode::Add(const String& name, YamlNode* value)
{
    InternalAddNodeToMap(name, RefPtr<YamlNode>(value), false);
}

inline void YamlNode::Add(const String& name, const RefPtr<YamlNode>& value)
{
    InternalAddNodeToMap(name, value, false);
}

inline void YamlNode::Set(const String& name, bool value)
{
    InternalAddToMap(name, VariantType(value), true);
}
inline void YamlNode::Set(const String& name, int32 value)
{
    InternalAddToMap(name, VariantType(value), true);
}
inline void YamlNode::Set(const String& name, float32 value)
{
    InternalAddToMap(name, VariantType(value), true);
}

inline void YamlNode::Set(const String& name, const char* value)
{
    InternalAddToMap(name, value, true);
}
inline void YamlNode::Set(const String& name, const String& value)
{
    InternalAddToMap(name, value, true);
}
inline void YamlNode::Set(const String& name, const WideString& value)
{
    InternalAddToMap(name, VariantType(value), true);
}

inline void YamlNode::Set(const String& name, const Vector2& value)
{
    InternalAddToMap(name, VariantType(value), true);
}
inline void YamlNode::Set(const String& name, const Vector3& value)
{
    InternalAddToMap(name, VariantType(value), true);
}
inline void YamlNode::Set(const String& name, const Vector4& value)
{
    InternalAddToMap(name, VariantType(value), true);
}

inline void YamlNode::Set(const String& name, const VariantType& value)
{
    InternalAddToMap(name, value, true);
}
inline void YamlNode::Set(const String& name, VariantType* varType)
{
    InternalAddToMap(name, *varType, true);
}

inline void YamlNode::Set(const String& name, YamlNode* value)
{
    InternalAddNodeToMap(name, RefPtr<YamlNode>(value), true);
}

inline void YamlNode::Set(const String& name, const RefPtr<YamlNode>& value)
{
    InternalAddNodeToMap(name, value, true);
}

inline void YamlNode::AddNodeToMap(const String& name, YamlNode* value)
{
    Add(name, value);
}

inline void YamlNode::AddNodeToMap(const String& name, const RefPtr<YamlNode>& value)
{
    Add(name, value);
}

inline void YamlNode::SetNodeToMap(const String& name, YamlNode* value)
{
    Set(name, value);
}

inline void YamlNode::SetNodeToMap(const String& name, const RefPtr<YamlNode>& value)
{
    Set(name, value);
}

inline void YamlNode::AddNodeToArray(YamlNode* value)
{
    Add(value);
}

inline void YamlNode::AddNodeToArray(const RefPtr<YamlNode>& value)
{
    Add(value);
}
};

#endif // __DAVAENGINE_YAML_NODE_H__
