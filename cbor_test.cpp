// cbor_test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC  
#include "stdint.h"
#include "stdafx.h"
#include "windows.h"
#include "LOG.H"		// Capture putchar & printf output to a log file
//#include <iostream>
//using namespace std;

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

#undef TRACE
#define TRACE(fmt,...) do { if (dflag) printf(fmt,__VA_ARGS__); } while(0)
//#define TRACE(...)
#undef DEBUG
#define DEBUG(...)

extern void MEM_Dump(unsigned char* data, int len, long base);

int dflag = false;

#undef MEM_Trace
#define MEM_Trace(data,len,base) do { if (dflag) MEM_Dump(data,len,base); } while(0)
//#define MEM_Trace(fmt,...)

extern "C"
{
	void MEM_Dump(uint8_t *data, uint16_t len, uint32_t base)
	{
		uint16_t i, j;

		//if (!CFG_IsTrace(DFLAG_TRC))
		//	return;

		//CON_printf("MEM: @%08x len=%04x\n",data,len);
		for (i = 0; i < len; i += 16)
		{
			printf(" %06x: ", base + i);
			for (j = 0; j < 16; j++)
			{
				if (j != 0)
				{
					if (!(j % 8))
						printf(" ");
					if (!(j % 1))
						printf(" ");
				}
				if ((i + j) < len)
					printf("%02x", data[i + j]);
				else
					printf("  ");
			}
			printf("  ");
			for (j = 0; j < 16 && (i + j) < len; j++)
			{
				if ((i + j) < len)
				{
					if (isprint(data[i + j]))
						printf("%c", data[i + j]);
					else
						printf(".");
				}
				else
					printf(" ");
			}
			printf("\n");
		}
	}
}


#include "cbor/cbor.h"

class CBOR
{
public:
	unsigned char* data;
	unsigned char* p;
	unsigned char* q;
	long base;
	int size;

	cbor_token_t* tokens;
	int tokens_size;
	int token_cnt;

	CBOR()
	{
		TRACE("CBOR()\n");
	}

	CBOR(int n)
	{
		TRACE("CBOR(%d) ", n);
		tokens = new cbor_token_t[n];
		tokens_size = n;
		TRACE("tokens[%d] @%xH\n", n, tokens);
	}

	void Init(unsigned char* data, int size)
	{
		TRACE("Init data @%xH size=%d\n", data, size);
		this->data = data;
		this->size = size;
		token_cnt = 0;
		base = 0L;
		q = data;
		p = q;
	}

	void def_map(int m)
	{
		TRACE(" map(%d)\n", m);
		p = cbor_write_map(q, size - base, m);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void def_array(int m)
	{
		TRACE(" array(%d)\n", m);
		p = cbor_write_array(q, size - base, m);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put()
	{
		TRACE(" break\n");
		p = cbor_write_special(q, size - base, 31);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(int n)
	{
		TRACE(" int(%d)\n", n);
		p = cbor_write_int(q, size - base, n);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(unsigned int n)
	{
		TRACE(" uint(%u)\n", n);
		p = cbor_write_uint(q, size - base, n);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(long long n)
	{
		TRACE(" long(%lld)\n", n);
		p = cbor_write_long(q, size - base, n);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(unsigned long long n)
	{
		TRACE(" ulong(%llu)\n", n);
		p = cbor_write_ulong(q, size - base, n);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(char* s)
	{
		TRACE(" text('%s')\n", s);
		p = cbor_write_text(q, size - base, s);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	void put(unsigned char* bytes, int m)
	{
		TRACE(" bytes([%d]:", m);
		for (int i = 0; i < m; i++)
		{
			TRACE("%02x ", bytes[i]);
		}
		TRACE("\n");
		p = cbor_write_bytes(q, size - base, bytes, m);
		MEM_Trace(q, p - q, base);
		base += p - q;
		q = p;
	}

	const char* typetostr(cbor_token_type_e type);

	cbor_token_t* token;

	cbor_token_t* get_first(cbor_token_type_e type);
	cbor_token_t* get_next(void);
	cbor_token_t* get_key(char* s);
	bool istype(cbor_token_type_e type);
	bool expected_next_type(cbor_token_type_e type);

	void list(void);

	virtual int parse(void);

	~CBOR()
	{
		TRACE("~CBOR() tokens @%xH size=%d:\n", tokens, tokens_size);
		if (tokens_size > 0)
		{
			delete tokens;
		}
	}
};

//#define CBOR_PARSE_MAX_TOKENS	ARRAYSIZE(CBOR_Parser::tokens)

// Find the text key value in the array of parsed tokens and return a pointer to the next token.
cbor_token_t* CBOR::get_key(char* s)
{
	bool key = false;
	for (int i = 0; i < token_cnt; i++)
	{
		if (!key && tokens[i].type == CBOR_TOKEN_TYPE_TEXT)
		{
			if (!strncmp(s, tokens[i].text_value, tokens[i].int_value))
			{
				key = true;
			}
		}
		else
		{
			if (key)
			{
				return &tokens[i];
			}
		}
	}
	return NULL;
}

cbor_token_t* CBOR::get_first(cbor_token_type_e type)
{
	for (int i = 0; i < token_cnt; i++)
	{
		if ((cbor_token_type_e)tokens[i].type == type)
		{
			return &tokens[i];
		}
	}
	return NULL;
}

cbor_token_t* CBOR::get_next(void)
{
	if (token_cnt == 0)
	{
		TRACE("get_next: No tokens[]!\n");
		return NULL;
	}
	if (token == NULL)
	{
		TRACE("get_next: NULL pointer!\n");
		return NULL;
	}
	if ((token + 1) > &tokens[token_cnt - 1])
	{
		TRACE("get_next: EOF\n");
		return NULL;
	}
	token += 1;
	//TRACE("get_next: %s\n", typetostr((cbor_token_type_e)token->type));
	return token;
}

const char* CBOR::typetostr(cbor_token_type_e type)
{
	switch (type)
	{
	case CBOR_TOKEN_TYPE_ERROR:
		return "ERROR";
	case CBOR_TOKEN_TYPE_INCOMPLETE:
		return "INCOMPLETE";
	case CBOR_TOKEN_TYPE_INT:
		return "INT";
	case CBOR_TOKEN_TYPE_LONG:
		return "LONG";
	case CBOR_TOKEN_TYPE_MAP:
		return "MAP";
	case CBOR_TOKEN_TYPE_ARRAY:
		return "ARRAY";
	case CBOR_TOKEN_TYPE_TEXT:
		return "TEXT";
	case CBOR_TOKEN_TYPE_BYTES:
		return "BYTES";
	case CBOR_TOKEN_TYPE_TAG:
		return "TAG";
	case CBOR_TOKEN_TYPE_SPECIAL:
		return "SPECIAL";
	case CBOR_TOKEN_TYPE_BREAK:
		return "BREAK";
	default:
		return "UNKNOWN";
	}
}

void CBOR::list(void)
{
	printf("%d tokens found:\n", token_cnt);
	cbor_token_t* token = tokens;
	for (int i = 0; i < token_cnt; i++, token++)
	{
		printf("%2d:\t(%d)%s\n", i, token->type, typetostr((cbor_token_type_e)token->type));
	}
}

int CBOR::parse(void)
{
	unsigned int offset = 0;
	token_cnt = 0;
	long j = 0;
	while (1)
	{
		//TRACE("cbor_parse data size=%d token_cnt=%d ", size, token_cnt);
		// Build up a list of tokens that are contained in a global array.
		if ((token_cnt + 1) > tokens_size)
		{
			printf("Out of token space!\n");
			MEM_Dump(data, size, 0L);
			return 0;
		}
		cbor_token_t* token = &tokens[token_cnt++];

		offset = cbor_read_token(data, size, offset, token);
		TRACE("cbor_read_token() offset=%d token->type=%d\n", offset, token->type);
		if (token->type == CBOR_TOKEN_TYPE_INCOMPLETE) {
			TRACE(" incomplete\n");
			break;
		}
		if (token->type == CBOR_TOKEN_TYPE_ERROR) {
			printf(" error: %s\n", token->error_value);
			MEM_Dump(data, size, 0L);
			break;
		}
		if (token->type == CBOR_TOKEN_TYPE_BREAK) {
			TRACE(" break\n");
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_INT) {
			TRACE(" int(%s%d)\n", token->sign < 0 ? "-" : "", (int)token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_UINT) {
			TRACE(" uint(%s%u)\n", token->sign < 0 ? "-" : "", token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_LONG) {
			TRACE(" long(%s%lld)\n", token->sign < 0 ? "-" : "", (long long)token->long_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_ULONG) {
			TRACE(" ulong(%s%llu)\n", token->sign < 0 ? "-" : "", token->long_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_ARRAY) {
			if (token->int_value == 31)
				TRACE(" array(*)\n");
			else
				TRACE(" array(%u)\n", token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_MAP) {
			if (token->int_value == 31)
				TRACE(" map(*)\n");
			else
				TRACE(" map(%u)\n", token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_TAG) {
			TRACE(" tag(%u)\n", token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_SPECIAL) {
			TRACE(" special(%u)\n", token->int_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_TEXT) {
			TRACE(" text('%.*s')\n", token->int_value, token->text_value);
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
		if (token->type == CBOR_TOKEN_TYPE_BYTES) {
			TRACE(" bytes([%u]:", token->int_value);
			for (int i = 0; i < token->int_value; i++)
			{
				TRACE("%02x ", token->bytes_value[i]);
			}
			TRACE(")\n");
			MEM_Trace(&data[j], offset - j, j);
			j = offset;
			continue;
		}
	}
	return offset;
}

bool CBOR::istype(cbor_token_type_e type)
{
	return token->type == type;
}

bool CBOR::expected_next_type(cbor_token_type_e type)
{
	return (token = get_next()) != NULL && istype(type);
}

#include <stdio.h>

char qflag = 0;  						// quiet mode off (output to window)

class myCBOR : public CBOR
{
public:
	myCBOR()
	{
		TRACE("myCBOR()\n");
	}

	myCBOR(int n) : CBOR(n)
	{
		TRACE("myCBOR(%d)\n", n);
	}

	~myCBOR()
	{
		TRACE("~myCBOR()\n");
	}

	int parse(void);
};

int myCBOR::parse()
{
	int offset = CBOR::parse();
	printf("parse() offset=%d\n", offset);

	// Expecting map of key pairs (text:array).
	if ((token = get_first(CBOR_TOKEN_TYPE_MAP)) == NULL)
	{
		printf(" Expected %s!\n", typetostr(CBOR_TOKEN_TYPE_MAP));
		return -1;
	}
	printf("%s[", typetostr((cbor_token_type_e)token->type));

	while (!expected_next_type(CBOR_TOKEN_TYPE_BREAK))
	{
		// Expecting text key.
		if (!istype(CBOR_TOKEN_TYPE_TEXT))
		{
			printf(" Expected key %s!\n", typetostr(CBOR_TOKEN_TYPE_TEXT));
			return -1;
		}
		printf("%s:", typetostr((cbor_token_type_e)token->type));

		// Expecting array.
		if (!expected_next_type(CBOR_TOKEN_TYPE_ARRAY))
		{
			printf(" Expected %s!\n", typetostr(CBOR_TOKEN_TYPE_ARRAY));
			return -1;
		}
		printf("%s[", typetostr((cbor_token_type_e)token->type));
		while (!expected_next_type(CBOR_TOKEN_TYPE_BREAK))
		{
			// Expecting map.
			if (!istype(CBOR_TOKEN_TYPE_MAP))
			{
				printf(" Expected map!\n");
				return -1;
			}
			printf("%s[", typetostr((cbor_token_type_e)token->type));
			while (!expected_next_type(CBOR_TOKEN_TYPE_BREAK))
			{
				// Expecting text key.
				if (!istype(CBOR_TOKEN_TYPE_TEXT))
				{
					printf(" Expected key %s!\n", typetostr(CBOR_TOKEN_TYPE_TEXT));
					return -1;
				}
				printf("%s:", typetostr((cbor_token_type_e)token->type));

				// Expecting anything (except break).
				if (expected_next_type(CBOR_TOKEN_TYPE_BREAK))
				{
					printf(" Expected key pair!\n");
					return -1;
				}
				printf("%s,", typetostr((cbor_token_type_e)token->type));
			}
			printf("]");
		}
		printf("]");
	}
	printf("]\n");
	return 0;
}

int main(int argc, char** argv)
{
	static unsigned char buffer[512];
	unsigned int size = sizeof(buffer);
	unsigned char* data = buffer;
	int base;
	unsigned int offset;
	cbor_token_t* token;

	// Open log file to capture output from putchar, puts and printf macros.
	LOG_Init(NULL);

	/* Basic parser test. */
	CBOR cbor(32);

	printf("\ncbor put:\n");
	cbor.Init(data, size);
	cbor.def_map(2);
	cbor.put((char *)"data");
	unsigned char bytes[] = { 0x01,0x02,0x03,0x04,0x05,0x07 };
	cbor.put(bytes, sizeof(bytes)); // size of data (in bytes);
	cbor.put((char*)"foo");
	cbor.put(1234);
	cbor.def_array(5);
	cbor.put(123);
	cbor.put(5);
	cbor.put((unsigned int)65535U);
	cbor.put((char*)"hello");
	cbor.put((char*)"world");
	cbor.put((long long)1231231231LL);
	cbor.put((unsigned long long)12312312311ULL);
	cbor.put((char*)"f:D");
	base = cbor.base;

	printf("\ncbor data:\n");
	MEM_Dump(data, base, 0L);

	printf("\ncbor parse:\n");
	cbor.Init(data, base);
	offset = cbor.parse();
	printf("offset=%d\n", offset);

	printf("\ncbor list:\n");
	cbor.list();

	// Do key lookups based on a 'text' token paired with the token following it.
	// Find key "data":bytes[].
	printf("\ncbor get_key:\n");
	token = cbor.get_key((char*)"data");
	unsigned int n = token->int_value;	// total number of bytes of data[] received
	printf("data[%d]:", n);
	MEM_Dump(token->bytes_value, n, 0L);

	// Find key "foo":int.
	token = cbor.get_key((char*)"foo");
	printf("foo:%d\n", token->int_value);

	/* Custom parser test. */
	myCBOR* mycbor;
	printf("\ncbor put:\n");
	mycbor = new myCBOR(32);
	mycbor->Init(data, size);
	mycbor->def_map(0);				// map
	mycbor->put((char*)"images");	//  text
	mycbor->def_array(0);			//  array
	mycbor->def_map(0);				//   map
	mycbor->put((char*)"slot");		//    text
	mycbor->put(0);					//    int
	mycbor->put((char*)"version");	//    text
	mycbor->put((char*)"1.0.7.0");	//    text
	mycbor->put((char*)"slot");		//    text
	mycbor->put(1);					//    int
	mycbor->put((char*)"version");	//    text
	mycbor->put((char*)"1.0.8.0");	//    text
	mycbor->put();					//   break
	mycbor->put();					//  break
	mycbor->put();					// break
	base = mycbor->base;

	printf("\ncbor data:\n");
	MEM_Dump(data, base, 0L);

	printf("\ncbor parse:\n");
	mycbor->Init(data, base);
	offset = mycbor->parse();
	printf("offset=%d\n", offset);

	printf("\ncbor list:\n");
	mycbor->list();
	delete mycbor;

	// Close capture log file.
	LOG_Term();

	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
