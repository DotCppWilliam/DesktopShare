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
	double amf_number_ = 0.0;	// 1�ֽ����� + 8�ֽ���ֵ
	bool amf_bool_ = false;		// 1�ֽ����� + 1�ֽ���ֵ
};








using AMFObjects = std::unordered_map<std::string, AMFObject>;
class AMFDecoder	// amf������,���ڽ�����amf�ĸ�������
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
	std::pair<std::uint32_t, std::string> key;	// key��һ���ַ���, ����:�ַ�������


	AMFObjectType val_type;		// ָ��value������
	std::shared_ptr<char> val;	// �洢value��ֵ
	std::size_t val_size;		// ���value��string����,��ô��¼����
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
	std::shared_ptr<char> data_;	// �洢amf��Ϣ
	uint32_t size_ = 0;
	uint32_t index_ = 0;	// ��¼��data_�е�ʹ���±�
};


