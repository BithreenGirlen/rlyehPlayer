#ifndef WIN_HTTP_SESSION_H_
#define WIN_HTTP_SESSION_H_


#include <string>

struct ResponseData
{
	std::wstring header;
	std::string content;
	unsigned long ulErrorCode = 0;
	std::string error;
};

namespace win_http_session
{
	bool RequestHttpPost(const wchar_t* url, const wchar_t* header, const char* payload, unsigned long payloadLength, ResponseData& r);
	bool RequestHttpGet(const wchar_t* url, const wchar_t* header, ResponseData& r);
}

#endif //WIN_HTTP_SESSION_H_