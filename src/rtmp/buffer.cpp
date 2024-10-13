#include "buffer.h"

Buffer::Buffer(int capacity)
{
	buffer_.resize(capacity);
}

Buffer::~Buffer()
{
	buffer_.clear();
}

bool Buffer::Append(std::shared_ptr<char> data, uint32_t size, uint32_t index)
{
	return Append(data.get(), size, index);
}

bool Buffer::Append(const char* data, uint32_t size, uint32_t index)
{
	if (size < index || index >= buffer_.size())
		return false;

	if ((index + size) < (buffer_.size() - writer_index_))
		buffer_.resize(buffer_.size() * 1.5);

	memcpy(buffer_.data() + writer_index_, data, size);

	return 0;
}

uint32_t Buffer::ReadAll(std::string& data)
{
	int size = ReadableBytes();
	if (size > 0)
	{
		data.assign(ReadBegin(), size);
		writer_index_ = reader_index_ = 0;
	}

	return size;
}

void Buffer::RetrieveAll()
{
	writer_index_ = reader_index_ = 0;
}

void Buffer::Retrieve(size_t len)
{
	if (len <= ReadableBytes())
	{
		reader_index_ += len;
		if (reader_index_ == writer_index_)
			reader_index_ = writer_index_ = 0;
	}
	else
		RetrieveAll();
}

void Buffer::RetrieveUntil(const char* end)
{
	Retrieve(ReadBegin() - end);
}

int Buffer::ReadableBytes() const
{
	return writer_index_ - reader_index_;
}

int Buffer::WritableBytes() const
{
	return buffer_.size() - writer_index_;
}

int Buffer::Size() const
{
	return buffer_.size();
}

char* Buffer::WriteBegin() const
{
	return (char*)buffer_.data() + writer_index_;
}

char* Buffer::ReadBegin() const
{
	return (char*)buffer_.data() + reader_index_;
}







uint32_t ReadUint32BE(uint8_t* data)
{
	uint32_t value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	return value;
}

uint32_t ReadUint32LE(uint8_t* data)
{
	uint32_t value = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
	return value;
}

uint32_t ReadUint24BE(uint8_t* data)
{
	uint32_t value = (data[0] << 16) | (data[1] << 8) | data[2];
	return value;
}

uint32_t ReadUint24LE(uint8_t* data)
{
	uint32_t value = (data[2] << 16) | (data[1] << 8) | data[0];
	return value;
}

uint32_t ReadUint16BE(uint8_t* data)
{
	uint16_t value = (data[0] << 8) | data[1];
	return value;
}

uint32_t ReadUint16LE(uint8_t* data)
{
	uint16_t value = (data[1] << 8) | data[0];
	return value;
}



void WriteUint32BE(uint8_t* p, uint32_t value)
{
	p[0] = value >> 24;
	p[1] = value >> 16;
	p[2] = value >> 8;
	p[3] = value & 0xff;
}

void WriteUint32LE(uint8_t* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
	p[3] = value >> 24;
}

void WriteUint24BE(uint8_t* p, uint32_t value)
{
	p[0] = value >> 16;
	p[1] = value >> 8;
	p[2] = value & 0xff;
}

void WriteUint24LE(uint8_t* p, uint32_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
	p[2] = value >> 16;
}

void WriteUint16BE(uint8_t* p, uint16_t value)
{
	p[0] = value >> 8;
	p[1] = value & 0xff;
}

void WriteUint16LE(uint8_t* p, uint16_t value)
{
	p[0] = value & 0xff;
	p[1] = value >> 8;
}

