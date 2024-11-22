#pragma once

#include "amf.h"
#include <string>
#include <unordered_map>
#include <memory>

struct AMFObject
{
	AMFObject() = default;
	AMFObject(std::string str)
	{
		type_ = AMF_STRING;
		amf_str_ = str;
	}
	AMFObject(double number)
	{
		type_ = AMF_NUMBER;
		amf_number_ = number;
	}

	AMFObject(bool bool_num)
	{
		type_ = AMF_BOOLEAN;
		amf_bool_ = bool_num;
	}
public:
	AMFObjectType type_ = AMF_NUMBER;
	std::string amf_str_;
	double amf_number_ = 0.0;	// 1字节类型 + 8字节数值
	bool amf_bool_ = false;		// 1字节类型 + 1字节数值
};








using AMFObjects = std::unordered_map<std::string, AMFObject>;
class AMFDecoder	// amf解码器,用于解析出amf的各种类型
{
public:
	AMFDecoder() = default;
	~AMFDecoder() = default;
public:
	int Decode(const char* data, int size, int cnt = -1);
	void Reset();
	std::string GetString() const;
	double GetDoubleNum() const;
	bool HasObject(std::string key) const;
	AMFObject GetAMFObject(std::string key)
	{
		return amf_objs_[key];
	}
	AMFObject GetAMFObject()
	{
		return amf_obj_;
	}
	AMFObjects& GetObjects();
private:
	static int DecodeBoolean(const char* data, int size, bool& amf_bool);
	static int DecodeDoubleNumber(const char* data, int size, double& amf_num);
	static int DecodeString(const char* data, int size, std::string& amf_str);
	static int DecodeObject(const char* data, int size, AMFObjects& amf_objects);

	static uint16_t DecodeInt16(const char* data, int size);
	static uint16_t DecodeInt24(const char* data, int size);
	static uint16_t DecodeInt32(const char* data, int size);
private:
	AMFObject amf_obj_;
	AMFObjects amf_objs_;
};

struct AMFProperty
{
	std::pair<std::uint32_t, std::string> key;	// key是一个字符串, 长度:字符串内容


	AMFObjectType val_type;		// 指定value的类型
	std::shared_ptr<char> val;	// 存储value的值
	std::size_t val_size;		// 如果value是string类型,那么记录长度
};

class AMFEncoder
{
public:
	AMFEncoder(uint32_t size = 1024);
	~AMFEncoder() = default;
public:
	void Reset();
	std::shared_ptr<char> Data();
	uint32_t Size() const;

	void EncodeString(const char* str, int len, bool is_obj = true);
	void EncodeDoubleNumber(double val);
	void EncodeBoolean(int val);
	void EncodeObjects(AMFObjects& objs);
	void EncodeECMA(AMFObjects& objs);
	void EncodeECMA(std::vector<AMFProperty>& ecma_array);
private:
	void EncodeInt8(int8_t val);
	void EncodeInt16(int8_t val);
	void EncodeInt24(int8_t val);
	void EncodeInt32(int8_t val);
	void Realloc(uint32_t size);
private:
	std::shared_ptr<char> data_;	// 存储amf信息
	uint32_t size_ = 0;
	uint32_t index_ = 0;	// 记录在data_中的使用下标
};


