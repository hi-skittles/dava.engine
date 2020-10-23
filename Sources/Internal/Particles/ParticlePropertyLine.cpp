#include "Particles/ParticlePropertyLine.h"

namespace DAVA
{
template <>
RefPtr<PropertyLine<float32>> PropertyLineYamlReader::CreatePropertyLineInternal<float32>(const YamlNode* node)
{
    if (!node)
        return RefPtr<PropertyLine<float32>>();

    if (node->GetType() == YamlNode::TYPE_STRING)
    {
        return RefPtr<PropertyLine<float32>>(new PropertyLineValue<float32>(node->AsFloat()));
    }
    else if (node->GetType() == YamlNode::TYPE_ARRAY)
    {
        RefPtr<PropertyLineKeyframes<float32>> keyframes(new PropertyLineKeyframes<float32>());

        for (uint32 k = 0; k < node->GetCount() / 2; ++k)
        {
            const YamlNode* time = node->Get(k * 2);
            const YamlNode* value = node->Get(k * 2 + 1);

            if (time && value)
            {
                keyframes->AddValue(time->AsFloat(), value->AsFloat());
            }
        }
        return keyframes;
    }
    return RefPtr<PropertyLine<float32>>();
}

template <>
RefPtr<PropertyLine<Vector2>> PropertyLineYamlReader::CreatePropertyLineInternal<Vector2>(const YamlNode* node)
{
    if (!node)
        return RefPtr<PropertyLine<Vector2>>();

    if (node->GetType() == YamlNode::TYPE_STRING)
    {
        float32 v = node->AsFloat();
        return RefPtr<PropertyLine<Vector2>>(new PropertyLineValue<Vector2>(Vector2(v, v)));
    }
    else if (node->GetType() == YamlNode::TYPE_ARRAY)
    {
        if (node->GetCount() == 2)
        {
            Vector2 res(1.0f, 1.0f);
            res = node->AsPoint();
            return RefPtr<PropertyLine<Vector2>>(new PropertyLineValue<Vector2>(res));
        }

        RefPtr<PropertyLineKeyframes<Vector2>> keyframes(new PropertyLineKeyframes<Vector2>());

        for (uint32 k = 0; k < node->GetCount() / 2; ++k)
        {
            const YamlNode* time = node->Get(k * 2);
            const YamlNode* value = node->Get(k * 2 + 1);

            if (time && value)
            {
                if (value->GetType() == YamlNode::TYPE_ARRAY)
                {
                    keyframes->AddValue(time->AsFloat(), value->AsPoint());
                }
                else
                {
                    float32 v = value->AsFloat();
                    keyframes->AddValue(time->AsFloat(), Vector2(v, v));
                }
            }
        }
        return keyframes;
    }

    return RefPtr<PropertyLine<Vector2>>();
}
template <>
RefPtr<PropertyLine<Vector3>> PropertyLineYamlReader::CreatePropertyLineInternal<Vector3>(const YamlNode* node)
{
    if (!node)
        return RefPtr<PropertyLine<Vector3>>();

    if (node->GetType() == YamlNode::TYPE_STRING)
    {
        float32 v = node->AsFloat();
        return RefPtr<PropertyLine<Vector3>>(new PropertyLineValue<Vector3>(Vector3(v, v, v)));
    }
    else if (node->GetType() == YamlNode::TYPE_ARRAY)
    {
        if (node->GetCount() == 2) // for 2D forces compatibility
        {
            Vector3 res(node->AsVector2());
            res.z = 0.0f;
            return RefPtr<PropertyLine<Vector3>>(new PropertyLineValue<Vector3>(res));
        }
        if (node->GetCount() == 3 || node->GetCount() == 2)
        {
            Vector3 res(0.0f, 0.0f, 0.0f);
            res = node->AsVector3();
            return RefPtr<PropertyLine<Vector3>>(new PropertyLineValue<Vector3>(res));
        }

        RefPtr<PropertyLineKeyframes<Vector3>> keyframes(new PropertyLineKeyframes<Vector3>());

        for (uint32 k = 0; k < node->GetCount() / 2; ++k)
        {
            const YamlNode* time = node->Get(k * 2);
            const YamlNode* value = node->Get(k * 2 + 1);

            if (time && value)
            {
                if (value->GetType() == YamlNode::TYPE_ARRAY)
                {
                    keyframes->AddValue(time->AsFloat(), value->AsVector3());
                }
                else
                {
                    Vector3 v = value->AsVector3();
                    keyframes->AddValue(time->AsFloat(), v);
                }
            }
        }
        return keyframes;
    }

    return RefPtr<PropertyLine<Vector3>>();
}

template <>
float32 PropertyValueHelper::MakeUnityValue<float32>()
{
    return 1.0f;
}

template <>
Vector2 PropertyValueHelper::MakeUnityValue<Vector2>()
{
    return Vector2(1.0f, 1.0f);
}

template <>
Vector3 PropertyValueHelper::MakeUnityValue<Vector3>()
{
    return Vector3(1.0f, 1.0f, 1.0f);
}

template <>
Color PropertyValueHelper::MakeUnityValue<Color>()
{
    return Color();
}

Color ColorFromYamlNode(const YamlNode* node)
{
    Color c;
    c.r = node->Get(0)->AsFloat() / 255.0f;
    c.g = node->Get(1)->AsFloat() / 255.0f;
    c.b = node->Get(2)->AsFloat() / 255.0f;
    c.a = node->Get(3)->AsFloat() / 255.0f;
    return c;
}

template <>
RefPtr<PropertyLine<Color>> PropertyLineYamlReader::CreatePropertyLineInternal<Color>(const YamlNode* node)
{
    if (!node)
        return RefPtr<PropertyLine<Color>>();

    if (node->GetType() == YamlNode::TYPE_ARRAY)
    {
        bool allString = true;
        for (uint32 k = 0; k < node->GetCount(); ++k)
            if (node->Get(k)->GetType() != YamlNode::TYPE_STRING)
                allString = false;

        if (allString && node->GetCount() == 4)
        {
            return RefPtr<PropertyLine<Color>>(new PropertyLineValue<Color>(ColorFromYamlNode(node)));
        }
        else
        {
            RefPtr<PropertyLineKeyframes<Color>> keyframes(new PropertyLineKeyframes<Color>());

            for (uint32 k = 0; k < node->GetCount() / 2; ++k)
            {
                const YamlNode* time = node->Get(k * 2);
                const YamlNode* value = node->Get(k * 2 + 1);

                if (time && value)
                {
                    if (value->GetType() == YamlNode::TYPE_ARRAY)
                    {
                        keyframes->AddValue(time->AsFloat(), ColorFromYamlNode(value));
                    }
                }
            }
            return keyframes;
        }
    }
    return RefPtr<PropertyLine<Color>>();
}

Vector4 PropertyLineYamlWriter::ColorToVector(const Color& color)
{
    return Vector4(color.r * 0xFF, color.g * 0xFF, color.b * 0xFF, color.a * 0xFF);
}
}