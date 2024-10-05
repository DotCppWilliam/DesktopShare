#pragma once

#include <QDebug>
#define HERROR(expr, str) \
	do { \
		HRESULT hr = expr; \
		if (FAILED(hr)) { \
			qDebug() << "file: " << __FILE__ << " line: " << __LINE__ << " err str: " << str; \
			return -1; \
		} \
	} while(0)

#define HERROR_STR(str) \
	qDebug() << "file: " << __FILE__ << " line: " << __LINE__ << " err str: " << str; \
		
	