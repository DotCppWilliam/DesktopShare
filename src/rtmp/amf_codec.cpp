#include "amf_codec.h"
#include "buffer.h"



//////////////////////////////////////////////////////////// AMFDecoder
int AMFDecoder::Decode(const char* data, int size, int cnt)
{
	int bytes_used = 0;
	while (size > bytes_used)
	{
		int ret = 0;
		char type = data[bytes_used];
		bytes_used++;

		switch (type)
		{
		case AMF0_NUMBER:
			amf_obj_.type_ = AMF_NUMBER;
			ret = DecodeDoubleNumber(data + bytes_used, size - bytes_used, amf_obj_.amf_number_);
			break;

		case AMF0_BOOLEAN:
			amf_obj_.type_ = AMF_BOOLEAN;
			ret = DecodeBoolean(data + bytes_used, size - bytes_used, amf_obj_.amf_bool_);
			break;

		case AMF0_STRING:
			amf_obj_.type_ = AMF_STRING;
			ret = DecodeString(data + bytes_used, size - bytes_used, amf_obj_.amf_str_);
			break;

		case AMF0_OBJECT:
			ret = DecodeObject(data + bytes_used, size - bytes_used, amf_objs_);
			break;

		case AMF0_OBJECT_END:
			break;

		case AMF0_ECMA_ARRAY:
			ret = DecodeObject(data + bytes_used + 4, size - bytes_used - 4, amf_objs_);
			break;

		case AMF0_NULL:
			break;
		default: break;
		}

		if (bytes_used < 0())
			break;

		bytes_used += ret;
		cnt--;
		if (cnt == 0)
			break;
	}

	return bytes_used;
}

void AMFDecoder::Reset()
{
	amf_obj_.amf_str_ = "";
	amf_obj_.amf_number_ = 0.0;
	amf_objs_.clear();
}

std::string AMFDecoder::GetString() const
{
	return amf_obj_.amf_str_;
}

double AMFDecoder::GetDoubleNum() const
{
	return amf_obj_.amf_number_;
}

bool AMFDecoder::HasObject(std::string key) const
{
	return amf_objs_.find(key) != amf_objs_.end();
}

AMFObject AMFDecoder::GetObject(std::string key)
{
	return amf_objs_[key];
}

AMFObject AMFDecoder::GetObject()
{
	return amf_obj_;
}

AMFObjects& AMFDecoder::GetObjects()
{
	return amf_objs_;
}

int AMFDecoder::DecodeBoolean(const char* data, int size, bool& amf_bool)
{
	if (size < 1)
		return 0;

	amf_bool = (data[0] != 0);
	return 1;
}

int AMFDecoder::DecodeDoubleNumber(const char* data, int size, double& amf_num)
{
	if (size < 8)
		return 0;
	
	for (int beg = 0, rbeg = 7; beg < 8; beg++, rbeg--)
		((char*)&amf_num)[beg] = data[rbeg];

	return 8;
}

int AMFDecoder::DecodeString(const char* data, int size, std::string& amf_str)
{
	if (size < 2)	// 没有STRING类型的length字段,占用2字节
		return 0;

	int bytes_used = 0;
	int str_size = DecodeInt16(data, size);	// 解析STRING类型中的长度
	bytes_used += 2;
	if (str_size > (size - bytes_used))
		return -1;

	// 解析字符串封装到string里
	amf_str = std::string(&data[bytes_used], 0, str_size);
	bytes_used += str_size;

	return bytes_used;
}

/**
 * 解析每个AMF对象,并存放在amf_objects里
 * AMF Object中是key,value这样的形式,结尾是00 00 09结尾符
 */
int AMFDecoder::DecodeObject(const char* data, int size, AMFObjects& amf_objects)
{
	amf_objects.clear();
	int bytes_used = 0;
	while (size > 0)
	{
		// key的字符串长度
		int key_str_len = DecodeInt16(data + bytes_used, size);
		size -= 2;
		if (size < key_str_len)
			return bytes_used;

		// 解析出key
		std::string key(data + bytes_used + 2, 0, key_str_len);
		size -= key_str_len;

		AMFDecoder decoder;
		// 2字节存储key字符串长度 + key字符串长度, 则定位到value的位置
		// 解析出value是什么类型
		int ret = decoder.Decode(data + bytes_used + 2 + key_str_len, size, 1);
		bytes_used += 2 + key_str_len + ret;
		if (ret <= 1)
			break;

		// 保存amf object
		amf_objects.emplace(key, decoder.GetObject());
	}

	return bytes_used;
}

uint16_t AMFDecoder::DecodeInt16(const char* data, int size)
{
	return (uint16_t)ReadUint16BE((uint8_t*)data);
}

uint16_t AMFDecoder::DecodeInt24(const char* data, int size)
{
	return ReadUint24BE((uint8_t*)data);
}

uint16_t AMFDecoder::DecodeInt32(const char* data, int size)
{
	return ReadUint32BE((uint8_t*)data);
}










//////////////////////////////////////////////////////////// AMFEncoder
AMFEncoder::AMFEncoder(uint32_t size)
	: data_(new char[size], std::default_delete<char[]>()),
	size_(size)
{

}

void AMFEncoder::Reset()
{
	index_ = 0;
}

std::shared_ptr<char> AMFEncoder::Data()
{
	return data_;
}

uint32_t AMFEncoder::Size() const
{
	return index_;
}

void AMFEncoder::EncodeString(const char* str, int len, bool is_obj)
{
	// 类型1字节 + 长度2字节(短字符串) + 2字节(额外空间)
	if ((int)(size_ - index_) < (int)(len + 1 + 2 + 2))
		Realloc(size_ + len + 5);

	if (len < 65536)	// 短字符串, 用2字节表示长度
	{
		if (is_obj)
			data_.get()[index_++] = AMF0_STRING;

		EncodeInt16(len);
	}
	else    // 长字符串,用4字节表示长度
	{
		if (is_obj)
			data_.get()[index_++] = AMF0_LONG_STRING;

		EncodeInt32(len);
	}

	// 将字符串信息拷贝到data_中
	memcpy(data_.get() + index_, str, len);
	index_ += len;	// 记录当前data_的下标
}

void AMFEncoder::EncodeDoubleNumber(double val)
{
	if ((size_ - index_) < 9)	// 空间小于9,也就是不满足NUMBER类型大小
		Realloc(size_ + 1024);	// 分配更大的空间

	data_.get()[index_++] = AMF0_NUMBER;	// 设置类型

	for (int i = 0, rbeg = 7; i < 8; i++, rbeg--, index_++)
		data_.get()[index_] = ((char*)&val)[rbeg];
}

void AMFEncoder::EncodeBoolean(int val)
{
	if ((size_ - index_) < 2)	// 不足BOOLEAN类型的大小
		Realloc(size_ + 1024);

	data_.get()[index_++] = AMF0_BOOLEAN;
	data_.get()[index_++] = val ? 0x01 : 0x00;
}

void AMFEncoder::EncodeObjects(AMFObjects& objs)
{
	if (objs.size() == 0)
	{
		EncodeInt8(AMF0_NULL);
		return;
	}

	EncodeInt8(AMF0_OBJECT);
	for (auto it : objs)
	{
		// 设置key(为字符串类型)
		EncodeString(it.first.c_str(), (int)it.first.size(), false);
		switch (it.second.type_)
		{
		case AMF_NUMBER:
			EncodeDoubleNumber(it.second.amf_number_);
			break;
		case AMF_STRING:
			EncodeString(it.second.amf_str_.c_str(), (int)it.second.amf_str_.size());
			break;
		case AMF_BOOLEAN:
			EncodeBoolean(it.second.amf_bool_);
			break;
		default:
			break;
		}
	}

	EncodeString("", 0, false);
	EncodeInt8(AMF0_OBJECT_END);
}

/*
	用于RTMP推流端发送meta info给服务端,
	其中就是使用ECMA array封装 音频、视频信息,发送给服务端
*/
void AMFEncoder::EncodeECMA(AMFObjects& objs)
{
	EncodeInt8(AMF0_ECMA_ARRAY);
	EncodeInt32(0);

	for (auto it : objs)
	{
		EncodeString(it.first.c_str(), (int)it.first.size(), false);
		switch (it.second.type_)
		{
		case AMF_NUMBER:
			EncodeDoubleNumber(it.second.amf_number_);
			break;
		case AMF_STRING:
			EncodeString(it.second.amf_str_.c_str(), (int)it.second.amf_str_.size());
			break;
		case AMF_BOOLEAN:
			EncodeBoolean(it.second.amf_bool_);
			break;
		default: break;
		}
	}
	EncodeString("", 0, false);
	EncodeInt8(AMF0_OBJECT_END);
}

void AMFEncoder::EncodeInt8(int8_t val)
{
	if ((size_ - index_) < 1)
		Realloc(size_ + 1024);

	data_.get()[index_] = val;
}

void AMFEncoder::EncodeInt16(int8_t val)
{
	if ((size_ - index_) < 2)
		Realloc(size_ + 1024);

	WriteUint16BE((uint8_t*)data_.get() + index_, val);
	index_ += 2;
}

void AMFEncoder::EncodeInt24(int8_t val)
{
	if ((size_ - index_) < 3)
		Realloc(size_ + 1024);

	WriteUint24BE((uint8_t*)data_.get() + index_, val);
	index_ += 3;
}

void AMFEncoder::EncodeInt32(int8_t val)
{
	if ((size_ - index_) < 4)
		Realloc(size_ + 1024);

	WriteUint32BE((uint8_t*)data_.get() + index_, val);
	index_ += 4;
}

void AMFEncoder::Realloc(uint32_t size)
{
	if (size <= size_)
		return;

	std::shared_ptr<char> data(new char[size], std::default_delete<char[]>());
	memcpy(data.get(), data_.get(), index_);
	size_ = size;
	data_ = data;
}
