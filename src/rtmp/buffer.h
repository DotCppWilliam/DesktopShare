#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <string>

class Buffer
{
public:
	Buffer(int capacity = 2048);
	~Buffer();
public:
	bool Append(std::shared_ptr<char> data, uint32_t size, uint32_t index = 0);
	bool Append(const char* data, uint32_t size, uint32_t index = 0);
	uint32_t ReadAll(std::string& data);
	void RetrieveAll();
	void Retrieve(size_t len);
	void RetrieveUntil(const char* end);
	void Resize(int size);
	void ZeroInit();
	bool Advance(size_t size);

	int ReadableBytes() const;
	int WritableBytes() const;
	int Size() const;
	char* WriteBegin() const;
	char* ReadBegin() const;
private:
	size_t reader_index_ = 0;
	size_t writer_index_ = 0;
	std::vector<uint8_t> buffer_;
};


uint32_t ReadUint32BE(uint8_t* data);	// 读取无符号整型32位 大端
uint32_t ReadUint32LE(uint8_t* data);	// 读取无符号整型32位 小端

uint32_t ReadUint24BE(uint8_t* data);	// 读取无符号整型24位 大端
uint32_t ReadUint24LE(uint8_t* data);	// 读取无符号整型24位 小端

uint32_t ReadUint16BE(uint8_t* data);	// 读取无符号整型16位 大端
uint32_t ReadUint16LE(uint8_t* data);	// 读取无符号整型16位 小端





void WriteUint32BE(uint8_t* p, uint32_t value);
void WriteUint32LE(uint8_t* p, uint32_t value);

void WriteUint24BE(uint8_t* p, uint32_t value);
void WriteUint24LE(uint8_t* p, uint32_t value);

void WriteUint16BE(uint8_t* p, uint16_t value);
void WriteUint16LE(uint8_t* p, uint16_t value);
