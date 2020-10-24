#pragma once
#include <fstream>
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/PrettyWriter.h"
#include "rapidjson/stringbuffer.h"
using namespace rapidjson;
using namespace std;
struct Json : Document {
	void ReadFile(const char* fn) {
		ifstream ifs(fn);
		if (!ifs.is_open())
			std::cerr << "can't find file " << fn << std::endl;
		string v{ (std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>() };
		toJson(v.c_str());
	}
	// Json 转 String
	string toString() {
		StringBuffer j;
		PrettyWriter<StringBuffer> writer(j);
		Accept(writer);
		return j.GetString();
	}
	// String 转 Json
	Json& toJson(const char* v) {
		Parse(v);
		if (HasParseError()) {
			std::cerr << "json err [" << rapidjson::GetParseError_En(GetParseError()) << "] at " << GetErrorOffset() << std::endl;
		}
		return *this;
	}
};