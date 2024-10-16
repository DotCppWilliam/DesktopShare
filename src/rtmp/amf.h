#pragma once

/*
	用途: 用于RTMP协议中,传递控制命令或元数据信息.比如连接、请求、一些设置参数等等


	AMF0 结构:
	{ type,			data_type }
	   1字节		根据类型不同占用大小不同

	比如: NUMBER类型:	AMF0 type: 0x00(1字节), number: 1(数值,8字节)
		STRING:			AMF0 type: 0x02(1字节), length: 7(2字节), "connect"(字符串)
		OBJECT类型:	type: 0x03(1字节), { Property类型(key,value)这样形式 }
						Property: Name, STRING类型
						Name: (length[2字节], 字符串),  String(type[1字节], length[2字节], 字符串)
						最后有个结尾符: 00 00 09
*/


enum AMF0DataType
{
	AMF0_NUMBER = 0,	/* type(1字节), number(8字节) */
	AMF0_BOOLEAN,
	AMF0_STRING,		/* 短字符串,用2字节表示长度 */
	AMF0_OBJECT,
	AMF0_MOVIECLIP,		/* reserved, not used */
	AMF0_NULL,
	AMF0_UNDEFINED,
	AMF0_REFERENCE,
	AMF0_ECMA_ARRAY,
	AMF0_OBJECT_END,
	AMF0_STRICT_ARRAY,
	AMF0_DATE,
	AMF0_LONG_STRING,	/* 长字符串,用4字节表示长度 */
	AMF0_UNSUPPORTED,
	AMF0_RECORDSET,		/* reserved, not used */
	AMF0_XML_DOC,
	AMF0_TYPED_OBJECT,
	AMF0_AVMPLUS,		/* switch to AMF3 */
	AMF0_INVALID = 0xff
};

enum AMF3DataType
{
	AMF3_UNDEFINED = 0,
	AMF3_NULL,
	AMF3_FALSE,
	AMF3_TRUE,
	AMF3_INTEGER,
	AMF3_DOUBLE,
	AMF3_STRING,
	AMF3_XML_DOC,
	AMF3_DATE,
	AMF3_ARRAY,
	AMF3_OBJECT,
	AMF3_XML,
	AMF3_BYTE_ARRAY
};

enum AMFObjectType
{
	AMF_NUMBER,
	AMF_BOOLEAN,
	AMF_STRING,
};