#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "FileSystem/VariantType.h"
#include "FileSystem/File.h"

#include "Math/MathConstants.h"
#include "Math/Matrix2.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/Math2D.h"
#include "Math/Color.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
/**
	\ingroup filesystem
	\brief this is a class that should be used for serialization & deserialization of the items
 */
class YamlNode;

VariantType PrepareValueForKeyedArchive(const Any& v, VariantType::eVariantType resultType);

class KeyedArchive : public BaseObject
{
protected:
    virtual ~KeyedArchive();

public:
    KeyedArchive();
    KeyedArchive(const KeyedArchive& arc);

    /**
        \brief Dumps archive to console
	 */
    void Dump() const;

    /**
		\brief Function to check if key is available in this archive.
		\param[in] key string key
		\returns true if key available
	 */
    bool IsKeyExists(const String& key) const;

    /**
		\brief Function to get variable from archive.
		\param[in] key string key
		\param[in] defaultValue this is value that is used if variable with this key do not exists in archive
		\returns value of variable or defaultValue if key isn't available
	 */
    bool GetBool(const String& key, bool defaultValue = false) const;
    /**
		\brief Function to get variable from archive.
		\param[in] key string key
		\param[in] defaultValue this is value that is used if variable with this key do not exists in archive
		\returns value of variable or defaultValue if key isn't available
	 */
    int32 GetInt32(const String& key, int32 defaultValue = 0) const;
    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    uint32 GetUInt32(const String& key, uint32 defaultValue = 0) const;
    /**
		\brief Function to get variable from archive.
		\param[in] key string key
		\param[in] defaultValue this is value that is used if variable with this key do not exists in archive
		\returns value of variable or defaultValue if key isn't available
	 */
    float32 GetFloat(const String& key, float32 defaultValue = 0.0f) const;
    /**
         \brief Function to get variable from archive.
         \param[in] key string key
         \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
         \returns value of variable or defaultValue if key isn't available
     */
    float64 GetFloat64(const String& key, float64 defaultValue = 0.0) const;
    /**
		\brief Functions to get variable from archive.
		\param[in] key string key
		\param[in] defaultValue this is value that is used if variable with this key do not exists in archive
		\returns value of variable or defaultValue if key isn't available
	 */
    String GetString(const String& key, const String& defaultValue = "") const;
    /**
		\brief Function to get variable from archive.
		\param[in] key string key
		\param[in] defaultValue this is value that is used if variable with this key do not exists in archive
		\returns value of variable or defaultValue if key isn't available
    */

    WideString GetWideString(const String& key, const WideString& defaultValue = L"") const;
    /**
        \brief Function to get variable from archive.
        \param[in] key string key
        \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
        \returns value of variable or defaultValue if key isn't available
	 */

    FastName GetFastName(const String& key, const FastName& defaultValue = FastName()) const;
    /**
        \brief Function to get variable from archive.
        \param[in] key string key
        \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
        \returns value of variable or defaultValue if key isn't available
	 */

    const uint8* GetByteArray(const String& key, const uint8* defaultValue = NULL) const;
    /**
        \brief Function to get variable from archive.
        \param[in] key string key
        \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
        \returns value of variable or defaultValue if key isn't available
	 */
    int32 GetByteArraySize(const String& key, int32 defaultValue = 0) const;

    /**
        \brief Function to load data from byte array as keyed archive.
        Call to this function is equivalent to creation of KeyedArchive class. Object returned from this function should be released. 
        If key is unavailable function returns 0
        \param[in] key string key
        \param[in] value we want to set for this key
	 */
    KeyedArchive* GetArchiveFromByteArray(const String& key) const;

    /**
     \brief Function to get archive from archive. Returns pointer to the archive inside.
     \param[in] key string key
     \param[in] defaultValue we want to set for this key
	 */
    KeyedArchive* GetArchive(const String& key, KeyedArchive* defaultValue = 0) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    int64 GetInt64(const String& key, int64 defaultValue = 0) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    uint64 GetUInt64(const String& key, uint64 defaultValue = 0) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Vector2 GetVector2(const String& key, const Vector2& defaultValue = Vector2()) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Vector3 GetVector3(const String& key, const Vector3& defaultValue = Vector3()) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Vector4 GetVector4(const String& key, const Vector4& defaultValue = Vector4()) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Matrix2 GetMatrix2(const String& key, const Matrix2& defaultValue = Matrix2()) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Matrix3 GetMatrix3(const String& key, const Matrix3& defaultValue = Matrix3()) const;

    /**
     \brief Function to get variable from archive.
     \param[in] key string key
     \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
     \returns value of variable or defaultValue if key isn't available
	 */
    Matrix4 GetMatrix4(const String& key, const Matrix4& defaultValue = Matrix4()) const;

    /**
    \brief Function to get variable from archive.
    \param[in] key string key
    \param[in] defaultValue this is value that is used if variable with this key do not exists in archive
    \returns value of variable or defaultValue if key isn't available
    */
    Color GetColor(const String& key, const Color& defaultValue = Color()) const;

    /*
        \brief Function to get object from byte array.
        \param[in] key string key
        \returns object
     */
    template <class T>
    T GetByteArrayAsType(const String& key, const T& defaultValue = T()) const;

    /**
		\brief Function to get variable from archive.
		\param[in] key string key
		\returns value of variable or default VariantType class if value isn't available
	 */
    VariantType* GetVariant(const String& key) const;

    /**
		\brief Function to set variable in archive.
		\param[in] key string key
		\param[in] value we want to set for this key
	 */
    void SetBool(const String& key, bool value);
    /**
		\brief Function to set variable in archive.
		\param[in] key string key
		\param[in] value we want to set for this key
	 */
    void SetInt32(const String& key, int32 value);
    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetUInt32(const String& key, uint32 value);

    /**
		\brief Function to set variable in archive.
		\param[in] key string key
		\param[in] value we want to set for this key
	 */
    void SetFloat(const String& key, float32 value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
     */
    void SetFloat64(const String& key, float64 value);

    /**
		\brief function to set variable in archive
		\param[in] key string key
		\param[in] value we want to set for this key
	 */
    void SetString(const String& key, const String& value);
    /**
		\brief Function to set variable in archive.
		\param[in] key string key
		\param[in] value we want to set for this key
	 */

    void SetWideString(const String& key, const WideString& value);
    /**
        \brief Function to set variable in archive.
        \param[in] key string key
        \param[in] value we want to set for this key
        \param[in] arraySize size fo the array we want tot save
	 */

    void SetFastName(const String& key, const FastName& value);
    /**
        \brief Function to set variable in archive.
        \param[in] key string key
        \param[in] value we want to set for this key
        \param[in] arraySize size fo the array we want tot save
	 */

    void SetByteArray(const String& key, const uint8* value, int32 arraySize);
    /**
		\brief Function to set variable in archive. Variant value is copying inside this method
		\param[in] key string key
		\param[in] value we want to set for this key
	 */
    void SetVariant(const String& key, const VariantType& value);
    void SetVariant(const String& key, VariantType&& value);
    /**
        \brief Function to set another keyed archive as key for this archive.
        \param[in] key string key
        \param[in] value we want to set for this key
	 */
    void SetByteArrayFromArchive(const String& key, KeyedArchive* archive);

    /**
     \brief Function to set another keyed archive as key for this archive.
     Function is copying archive inside. If you need to work with this archive later use GetArchive().
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetArchive(const String& key, KeyedArchive* archive);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetInt64(const String& key, const int64& value);
    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetUInt64(const String& key, const uint64& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetVector2(const String& key, const Vector2& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetVector3(const String& key, const Vector3& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetVector4(const String& key, const Vector4& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetMatrix2(const String& key, const Matrix2& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetMatrix3(const String& key, const Matrix3& value);

    /**
     \brief Function to set variable in archive.
     \param[in] key string key
     \param[in] value we want to set for this key
	 */
    void SetMatrix4(const String& key, const Matrix4& value);

    /**
    \brief Function to set variable in archive.
    \param[in] key string key
    \param[in] value we want to set for this key
    */
    void SetColor(const String& key, const Color& value);

    /**
        \brief Function to set value from template type to byte array.  
        This functionality is added to perform simple storage of complex types, like Vector3, Vector4, Matrix4 and others to byte arrays
        \param[in] key string key
        \param[in] value value we want to set for given key
     */
    template <class T>
    void SetByteArrayAsType(const String& key, const T& value);

    /**
		\brief Function loads data from given file.
		\param[in] pathName relative pathname in application documents folder
	 */
    bool Load(const FilePath& pathName);
    /**
		\brief Function saves data to given file.
		\param[in] pathName relative pathname in application documents folder
	 */
    bool Save(const FilePath& pathName) const;

    /**
        \brief Function loads data from given file.
        \param[in] file to load from
	 */
    bool Load(File* file);
    /**
        \brief Function saves data to given file.
        \param[in] file to save
	 */
    bool Save(File* file) const;

    /**
         \brief Function to save archieve to byte array.
         \param[in] data byte arrat for archieve data, if data is null function returns only requested size of data for serialization
         \param[in] size size of byte array, if size is 0 function returns only requested size of data for serialization
         \returns size of really serialized data
     */
    uint32 Save(uint8* data, uint32 size) const;

    /**
         \brief Function to load archieve from byte array.
         \param[in] data byte arrat with archieve data
         \param[in] size size of byte array
         \returns result of loading
     */
    bool Load(const uint8* data, uint32 size);

    /**
     \brief Function loads data from given yaml file.
     \param[in] pathName relative pathname in application documents folder
	 */
    bool LoadFromYamlFile(const FilePath& pathName);

    /**
     \brief Function saves data to given yaml file.
     \param[in] file to save
	 */
    bool SaveToYamlFile(const FilePath& pathName) const;

    /**
		\brief Deletes named key.
		\param[in] key name of the key to delete
	 */
    void DeleteKey(const String& key);

    /**
		\brief Deletes all keys, making archive empty.
	 */
    void DeleteAllKeys();

    uint32 Count(const String& key = "") const;

    /**
     \brief Function to get all data of archive.
     \returns map of VariantType class with names
	 */
    using UnderlyingMap = UnorderedMap<String, VariantType*>;
    const UnderlyingMap& GetArchieveData() const;

    /**
     \brief Function loads data from given yaml Node.
     \param[in] pathName relative pathname in application documents folder
	 */
    bool LoadFromYamlNode(const YamlNode* rootNode);

    //	yaml
    // 	/**
    // 		\brief this function loads data from given yaml file
    // 		\param[in] pathName relative pathname in application documents folder
    // 	*/
    // 	bool LoadFromYaml(const String & pathName);
    //
    // 	/**
    // 		\brief this function saes data to given yaml file
    // 		\param[in] pathName relative pathname in application documents folder
    // 	*/
    // 	bool SaveToYaml(const String & pathName);

    static const char* GenKeyFromIndex(uint32 index);

    /**
     \brief Assignment operator
     \returns Returns reference to this
     */
    KeyedArchive& operator=(const KeyedArchive& arc);

    INTROSPECTION(KeyedArchive, nullptr)

private:
    template <typename T, typename M>
    void SetVariant(const String& key, const T& value, M setVariantMethod)
    {
        auto iter = objectMap.find(key);
        if (iter != objectMap.end())
        {
            (iter->second->*setVariantMethod)(value);
        }
        else
        {
            objectMap[key] = new VariantType(value);
        }
    }

    friend class KeyedArchiveStructureWrapper;
    UnderlyingMap objectMap;

    DAVA_VIRTUAL_REFLECTION(KeyedArchive, BaseObject);
};

// Implementation

template <class T>
T KeyedArchive::GetByteArrayAsType(const String& key, const T& defaultValue) const
{
    int size = GetByteArraySize(key);
    if (size != 0)
    {
        DVASSERT(size == sizeof(T));

        T value;
        const uint8* arrayData = GetByteArray(key);
        memcpy(&value, arrayData, sizeof(T));
        return value;
    }
    else
    {
        return defaultValue;
    }
}

template <class T>
void KeyedArchive::SetByteArrayAsType(const String& key, const T& value)
{
    SetByteArray(key, reinterpret_cast<const uint8*>(&value), sizeof(T));
}
};
