#ifndef __DAVAENGINE_PARTICLES_PROPERTY_LINE_H__
#define __DAVAENGINE_PARTICLES_PROPERTY_LINE_H__

#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Base/RefPtr.h"
#include <limits>

namespace DAVA
{
template <class T>
class PropertyLine : public BaseObject
{
protected:
    ~PropertyLine()
    {
    }

public:
    struct PropertyKey
    {
        float32 t;
        T value;
    };

    Vector<PropertyKey> keys;
    Vector<PropertyKey>& GetValues()
    {
        return keys;
    }

    virtual const T& GetValue(float32 t) = 0;

    virtual PropertyLine<T>* Clone()
    {
        return 0;
    }
};

class PropertyValueHelper
{
public:
    template <class T>
    static T MakeUnityValue();
};

template <class T>
class PropertyLineValue : public PropertyLine<T>
{
protected:
    ~PropertyLineValue()
    {
    }

public:
    PropertyLineValue(T _value)
    {
        typename PropertyLine<T>::PropertyKey v;
        v.t = 0;
        v.value = _value;
        PropertyLine<T>::keys.push_back(v);
    }

    const T& GetValue(float32 /*t*/)
    {
        return PropertyLine<T>::keys[0].value;
    }

    PropertyLine<T>* Clone()
    {
        return new PropertyLineValue<T>(PropertyLine<T>::keys[0].value);
    }
};

template <class T>
class PropertyLineKeyframes : public PropertyLine<T>
{
protected:
    ~PropertyLineKeyframes()
    {
    }

public:
    T resultValue;

    const T& GetValue(float32 t)
    {
        int32 keysSize = static_cast<int32>(PropertyLine<T>::keys.size());
        DVASSERT(keysSize);
        if (t > PropertyLine<T>::keys[keysSize - 1].t)
        {
            return PropertyLine<T>::keys[keysSize - 1].value;
        }
        if (t <= PropertyLine<T>::keys[0].t)
        {
            return PropertyLine<T>::keys[0].value;
        }
        if (PropertyLine<T>::keys.size() == 2)
        {
            if (t < PropertyLine<T>::keys[1].t)
            {
                float ti = (t - PropertyLine<T>::keys[0].t) / (PropertyLine<T>::keys[1].t - PropertyLine<T>::keys[0].t);
                resultValue = PropertyLine<T>::keys[0].value + (PropertyLine<T>::keys[1].value - PropertyLine<T>::keys[0].value) * ti;
                return resultValue;
            }
            else
            {
                return PropertyLine<T>::keys[1].value;
            }
        }
        else if (PropertyLine<T>::keys.size() == 1)
            return PropertyLine<T>::keys[0].value;
        else
        {
            int32 l = BinaryFind(t, 0, static_cast<int32>(PropertyLine<T>::keys.size()) - 1);

            float ti = (t - PropertyLine<T>::keys[l].t) / (PropertyLine<T>::keys[l + 1].t - PropertyLine<T>::keys[l].t);
            resultValue = PropertyLine<T>::keys[l].value + (PropertyLine<T>::keys[l + 1].value - PropertyLine<T>::keys[l].value) * ti;
        }
        return resultValue;
    }

    int32 BinaryFind(float32 t, int32 l, int32 r)
    {
        if (l + 1 == r) // we've found a solution
        {
            return l;
        }

        int32 m = (l + r) >> 1; //l + (r - l) / 2 = l + r / 2 - l / 2 = (l + r) / 2;
        if (t <= PropertyLine<T>::keys[m].t)
            return BinaryFind(t, l, m);
        else
            return BinaryFind(t, m, r);
    }

    void AddValue(float32 t, T value)
    {
        typename PropertyLine<T>::PropertyKey key;
        key.t = t;
        key.value = value;
        PropertyLine<T>::keys.push_back(key);
    }

    PropertyLine<T>* Clone()
    {
        PropertyLineKeyframes<T>* clone = new PropertyLineKeyframes<T>();
        clone->keys = PropertyLine<T>::keys;
        return clone;
    }
};

class ModifiablePropertyLineBase
{
public:
    virtual ~ModifiablePropertyLineBase() = default;
    ModifiablePropertyLineBase(const String& name)
        : externalValueName(name)
    {
    }
    virtual void SetModifier(float32 v) = 0;
    const String& GetValueName()
    {
        return externalValueName;
    }
    void SetValueName(const String& name)
    {
        externalValueName = name;
    }

protected:
    String externalValueName;
};

template <class T>
class ModifiablePropertyLine : public ModifiablePropertyLineBase, public PropertyLine<T>
{
public:
    ModifiablePropertyLine<T>(const String& name)
        : ModifiablePropertyLineBase(name)
    {
    }
    virtual void SetModifier(float32 v);
    void SetModificationLine(RefPtr<PropertyLine<T>> line)
    {
        this->modificationLine = line;
    }
    void SetValueLine(RefPtr<PropertyLine<T>> line)
    {
        this->valueLine = line;
    }

    RefPtr<PropertyLine<T>> GetModificationLine()
    {
        return modificationLine;
    }
    RefPtr<PropertyLine<T>> GetValueLine()
    {
        return valueLine;
    }
    const T& GetValue(float32 t);
    virtual PropertyLine<T>* Clone();

protected:
    T resultValue; //well - this how ProertyLine itself work - err
    T modifier;
    RefPtr<PropertyLine<T>> modificationLine;
    RefPtr<PropertyLine<T>> valueLine;
};

template <class T>
void ModifiablePropertyLine<T>::SetModifier(float32 v)
{
    if (modificationLine)
        modifier = modificationLine->GetValue(v);
    else
        modifier = PropertyValueHelper::MakeUnityValue<T>();
}

template <class T>
const T& ModifiablePropertyLine<T>::GetValue(float32 t)
{
    if (!valueLine)
    {
        resultValue = T();
    }
    else
    {
        resultValue = modifier * (valueLine->GetValue(t));
    }
    return resultValue;
}

template <class T>
PropertyLine<T>* ModifiablePropertyLine<T>::Clone()
{
    ModifiablePropertyLine<T>* clone = new ModifiablePropertyLine<T>(GetValueName());
    if (valueLine)
    {
        clone->valueLine = valueLine->Clone();
        clone->valueLine->Release();
    }
    else
        clone->valueLine = nullptr;
    if (modificationLine)
    {
        clone->modificationLine = modificationLine->Clone();
        clone->modificationLine->Release();
    }
    else
        modificationLine = nullptr;
    clone->modifier = modifier; //not use set modifier!
    return clone;
};

template <class T>
class PropValue
{
public:
    float32 t;
    T v;
    PropValue(float32 t, const T& v)
    {
        this->t = t;
        this->v = v;
    }
};

// A wrapper for Property Line, which allows easy access to the values.
template <class T>
class PropLineWrapper
{
public:
    PropLineWrapper(RefPtr<PropertyLine<T>> propertyLine)
    {
        Init(propertyLine);
    }
    PropLineWrapper()
    {
    }

    virtual ~PropLineWrapper()
    {
    }

    void Init(RefPtr<PropertyLine<T>> propertyLine);

    Vector<PropValue<T>>* GetPropsPtr();
    Vector<PropValue<T>> GetProps() const;
    RefPtr<PropertyLine<T>> GetPropLine() const;

private:
    Vector<PropValue<T>> values;
    float32 minT;
    float32 maxT;
};

template <class T>
void PropLineWrapper<T>::Init(RefPtr<PropertyLine<T>> propertyLine)
{
    values.clear();
    minT = std::numeric_limits<float32>::infinity();
    maxT = -std::numeric_limits<float32>::infinity();

    PropertyLineValue<T>* pv;
    PropertyLineKeyframes<T>* pk;

    pk = dynamic_cast<PropertyLineKeyframes<T>*>(propertyLine.Get());
    pv = dynamic_cast<PropertyLineValue<T>*>(propertyLine.Get());

    if (pk)
    {
        for (uint32 i = 0; i < pk->keys.size(); ++i)
        {
            float32 t = pk->keys[i].t;
            minT = Min(minT, t);
            maxT = Max(maxT, t);

            values.push_back(PropValue<T>(t, pk->keys[i].value));
        }
    }
    else if (pv)
    {
        values.push_back(PropValue<T>(0, pv->GetValue(0)));
    }
}

template <class T>
Vector<PropValue<T>>* PropLineWrapper<T>::GetPropsPtr()
{
    return &values;
}

template <class T>
Vector<PropValue<T>> PropLineWrapper<T>::GetProps() const
{
    return values;
}

template <class T>
RefPtr<PropertyLine<T>> PropLineWrapper<T>::GetPropLine() const
{
    if (values.size() > 1)
    {
        //return PropertyLineKeyframes
        RefPtr<PropertyLineKeyframes<T>> lineKeyFrames(new PropertyLineKeyframes<T>());
        for (uint32 i = 0; i < values.size(); ++i)
        {
            lineKeyFrames->AddValue(values[i].t, values[i].v);
        }
        return lineKeyFrames;
    }
    else if (values.size() == 1)
    {
        //return PropertyLineValue
        RefPtr<PropertyLineValue<T>> lineValue(new PropertyLineValue<T>(values[0].v));
        return lineValue;
    }
    return RefPtr<PropertyLine<T>>();
}

class PropertyLineYamlReader
{
    template <class T>
    static RefPtr<PropertyLine<T>> CreatePropertyLineInternal(const YamlNode* node);

public:
    //this will also
    template <class T>
    static RefPtr<PropertyLine<T>> CreatePropertyLine(const YamlNode* node)
    {
        if (!node)
            return RefPtr<PropertyLine<T>>();
        if (node->GetType() == YamlNode::TYPE_MAP)
        {
            const YamlNode* externalVariable = node->Get("externalVariable");
            if (externalVariable) //ModifiableNode
            {
                ModifiablePropertyLine<T>* resultingLine(new ModifiablePropertyLine<T>(externalVariable->AsString()));
                resultingLine->SetValueLine(PropertyLineYamlReader::CreatePropertyLineInternal<T>(node->Get("value_line")));
                resultingLine->SetModificationLine(PropertyLineYamlReader::CreatePropertyLineInternal<T>(node->Get("modification_line")));
                resultingLine->SetModifier(0.0f);
                return RefPtr<PropertyLine<T>>(resultingLine);
            }
        }
        return CreatePropertyLineInternal<T>(node);
    }
};

class PropertyLineYamlWriter
{
public:
    // Write the single value and whole property line to the YAML node.
    template <class T>
    static void WritePropertyValueToYamlNode(YamlNode* parentNode, const String& propertyName, T propertyValue);
    template <class T>
    inline static void WritePropertyLineToYamlNode(YamlNode* parentNode, const String& propertyName, RefPtr<PropertyLine<T>> propertyLine);

protected:
    // Convert Color to Vector4 value.
    static Vector4 ColorToVector(const Color& color);
    template <class T>
    static void WritePropertyLineToYamlNodeInternal(YamlNode* parentNode, const String& propertyName, RefPtr<PropertyLine<T>> propertyLine);
};

// Writer logic.
template <class T>
void PropertyLineYamlWriter::WritePropertyValueToYamlNode(YamlNode* parentNode, const String& propertyName,
                                                          T propertyValue)
{
    parentNode->Set(propertyName, propertyValue);
}

template <class T>
void PropertyLineYamlWriter::WritePropertyLineToYamlNode(YamlNode* parentNode, const String& propertyName, RefPtr<PropertyLine<T>> propertyLine)
{
    ModifiablePropertyLine<T>* modifiebleLine = dynamic_cast<ModifiablePropertyLine<T>*>(propertyLine.Get());
    if (modifiebleLine)
    {
        YamlNode* node = new YamlNode(YamlNode::TYPE_MAP);
        node->Set("externalVariable", modifiebleLine->GetValueName());
        WritePropertyLineToYamlNodeInternal(node, "value_line", modifiebleLine->GetValueLine());
        WritePropertyLineToYamlNodeInternal(node, "modification_line", modifiebleLine->GetModificationLine());
        parentNode->AddNodeToMap(propertyName, node);
    }
    else
    {
        WritePropertyLineToYamlNodeInternal<T>(parentNode, propertyName, propertyLine);
    }
}

template <class T>
inline void PropertyLineYamlWriter::WritePropertyLineToYamlNodeInternal(YamlNode* parentNode, const String& propertyName, RefPtr<PropertyLine<T>> propertyLine)
{
    // Write the property line.
    Vector<PropValue<T>> wrappedPropertyValues = PropLineWrapper<T>(propertyLine).GetProps();
    if (wrappedPropertyValues.empty())
    {
        return;
    }

    if (wrappedPropertyValues.size() == 1)
    {
        // This has to be single string value.
        parentNode->Set(propertyName, wrappedPropertyValues.at(0).v);
        return;
    }

    // Create the child array node.
    YamlNode* childNode = new YamlNode(YamlNode::TYPE_ARRAY);
    for (typename Vector<PropValue<T>>::iterator iter = wrappedPropertyValues.begin();
         iter != wrappedPropertyValues.end(); iter++)
    {
        childNode->Add((*iter).t);
        childNode->Add((*iter).v);
    }

    parentNode->AddNodeToMap(propertyName, childNode);
}

//for some unknown reasons colors are multiplied/divided by 255 on save/load - better to remove this logic but it will break all saved effects :(
template <>
inline void PropertyLineYamlWriter::WritePropertyLineToYamlNodeInternal<Color>(YamlNode* parentNode, const String& propertyName, RefPtr<PropertyLine<Color>> propertyLine)
{
    // Write the property line.
    Vector<PropValue<Color>> wrappedPropertyValues = PropLineWrapper<Color>(propertyLine).GetProps();
    if (wrappedPropertyValues.empty())
    {
        return;
    }

    if (wrappedPropertyValues.size() == 1)
    {
        // This has to be single string value. Write Colors as Vectors.
        parentNode->Set(propertyName, ColorToVector(wrappedPropertyValues.at(0).v));
        return;
    }

    // Create the child array node.
    YamlNode* childNode = new YamlNode(YamlNode::TYPE_ARRAY);
    for (Vector<PropValue<Color>>::iterator iter = wrappedPropertyValues.begin();
         iter != wrappedPropertyValues.end(); iter++)
    {
        childNode->Add((*iter).t);
        childNode->Add(ColorToVector((*iter).v));
    }

    parentNode->AddNodeToMap(propertyName, childNode);
}

class PropertyLineHelper
{
public:
    /*hmmm*/
    template <class T>
    static void AddIfModifiable(PropertyLine<T>* line, List<ModifiablePropertyLineBase*>& dest)
    {
        ModifiablePropertyLineBase* modifiable = dynamic_cast<ModifiablePropertyLineBase*>(line);
        if (modifiable)
            dest.push_back(modifiable);
    }

    template <class T>
    static RefPtr<PropertyLine<T>> GetValueLine(const RefPtr<PropertyLine<T>>& src)
    {
        ModifiablePropertyLine<T>* modifiable = dynamic_cast<ModifiablePropertyLine<T>*>(src.Get());
        if (modifiable)
            return modifiable->GetValueLine();
        else
            return src;
    }

    template <class T>
    static PropertyLine<T>* GetValueLine(PropertyLine<T>* src)
    {
        ModifiablePropertyLine<T>* modifiable = dynamic_cast<ModifiablePropertyLine<T>*>(src);
        if (modifiable)
            return modifiable->GetValueLine().Get();
        else
            return src;
    }

    template <class T>
    static void SetValueLine(RefPtr<PropertyLine<T>>& dst, RefPtr<PropertyLine<T>>& src)
    {
        ModifiablePropertyLine<T>* modifiable = dynamic_cast<ModifiablePropertyLine<T>*>(dst.Get());
        if (modifiable)
            modifiable->SetValueLine(src);
        else
            dst = src;
    }

    template <class T>
    static RefPtr<PropertyLine<T>> MakeModifiable(RefPtr<PropertyLine<T>>& line)
    {
        RefPtr<PropertyLine<T>> values = GetValueLine(line);
        RefPtr<PropertyLine<T>> modification = RefPtr<PropertyLine<T>>(new PropertyLineValue<T>(PropertyValueHelper::MakeUnityValue<T>()));
        ModifiablePropertyLine<T>* res = new ModifiablePropertyLine<T>("DefaultVariable");
        res->SetValueLine(values);
        res->SetModificationLine(modification);
        res->SetModifier(0);
        line = RefPtr<PropertyLine<T>>(res);
        return line;
    }

    template <class T>
    static RefPtr<PropertyLine<T>> RemoveModifiable(RefPtr<PropertyLine<T>>& line)
    {
        line = GetValueLine(line);
        return line;
    }
};
};

#endif // __DAVAENGINE_PARTICLES_PROPERTY_LINE_H__