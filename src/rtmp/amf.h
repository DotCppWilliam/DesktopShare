#pragma once

/*
	��;: ����RTMPЭ����,���ݿ��������Ԫ������Ϣ.�������ӡ�����һЩ���ò����ȵ�


	AMF0 �ṹ:
	{ type,			data_type }
	   1�ֽ�		�������Ͳ�ͬռ�ô�С��ͬ

	����: NUMBER����:	AMF0 type: 0x00(1�ֽ�), number: 1(��ֵ,8�ֽ�)
		STRING:			AMF0 type: 0x02(1�ֽ�), length: 7(2�ֽ�), "connect"(�ַ���)
		OBJECT����:	type: 0x03(1�ֽ�), { Property����(key,value)������ʽ }
						Property: Name, STRING����
						Name: (length[2�ֽ�], �ַ���),  String(type[1�ֽ�], length[2�ֽ�], �ַ���)
						����и���β��: 00 00 09
*/


enum AMF0DataType
{
	AMF0_NUMBER = 0,	/* type(1�ֽ�), number(8�ֽ�) */
	AMF0_BOOLEAN,
	AMF0_STRING,		/* ���ַ���,��2�ֽڱ�ʾ���� */
	AMF0_OBJECT,
	AMF0_MOVIECLIP,		/* reserved, not used */
	AMF0_NULL,
	AMF0_UNDEFINED,
	AMF0_REFERENCE,
	AMF0_ECMA_ARRAY,
	AMF0_OBJECT_END,
	AMF0_STRICT_ARRAY,
	AMF0_DATE,
	AMF0_LONG_STRING,	/* ���ַ���,��4�ֽڱ�ʾ���� */
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