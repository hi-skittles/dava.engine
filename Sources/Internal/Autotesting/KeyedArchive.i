%module KeyedArchive

%{
#include "FileSystem/KeyedArchive.h"

%}

%include "std_string.i"

%import Vector.i

namespace DAVA 
{  

class KeyedArchive
{
protected:
	virtual ~KeyedArchive();
public:
	KeyedArchive();
	KeyedArchive(const KeyedArchive &arc);
		
	bool IsKeyExists(const String & key);
	
	bool GetBool(const String & key, bool defaultValue = false);

	int32 GetInt32(const String & key, int32 defaultValue = 0);

	uint32 GetUInt32(const String & key, uint32 defaultValue = 0);

	float32 GetFloat(const String & key, float32 defaultValue = 0.0f);

	String GetString(const String & key, const String & defaultValue = "");

	KeyedArchive * GetArchive(const String & key, KeyedArchive * defaultValue = 0);

	int64 GetInt64(const String & key, int64 defaultValue = 0);
    
	uint64 GetUInt64(const String & key, uint64 defaultValue = 0);

	Vector2 GetVector2(const String & key, const Vector2 & defaultValue = Vector2());
    
	Vector3 GetVector3(const String & key, const Vector3 & defaultValue = Vector3());

	void SetBool(const String & key, bool value);

	void SetInt32(const String & key, int32 value);

	void SetUInt32(const String & key, uint32 value);

	void SetFloat(const String & key, float32 value);

	void SetString(const String & key, const String & value);

	void SetArchive(const String & key, KeyedArchive * archive);

	void SetInt64(const String & key, int64 &value);

	void SetUInt64(const String & key, uint64 &value);

	void SetVector2(const String & key, Vector2 &value);

	void SetVector3(const String & key, Vector3 &value);

	void DeleteKey(const String & key);

	void DeleteAllKeys();

	uint32 Count(const String & key = "");
};

};
