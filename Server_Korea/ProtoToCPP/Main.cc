#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <cassert>
#include <cstring>
#include <cstdlib>

using namespace std;

struct Enum{
	string name;
	list<pair<string, string> > fields;
};

struct Field{
	string rule;
	string type;
	string name;
	string def;
};

struct Message{
	string name;
	list<Enum> enums;
	list<Field> fields;
};

struct Content{
	string name;
	list<string> header;
	list<Message> messages;
};

static inline bool IsNum(char c) {
	return c >= '0' && c <= '9';
}

static inline bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int ReadNextWord(string &line, int begin, string &word) {
	bool enter = false;
	int i;
	for (i = begin; i < line.size(); i++) {
		char c = line[i];
		if (!IsNum(c) && !IsAlpha(c) && c != '_') {
			if (enter)
				break;
			continue;
		}
		if (c == '/') {
			if (i + 1 < line.size()) {
				if (line[i + 1] == '/')
					break;
			}
		}

		enter = true;
		word += c;
	}

	return i;
}

static int ReadNextType(string &line, int begin, string &word) {
	bool enter = false;
	int i;
	for (i = begin; i < line.size(); i++) {
		char c = line[i];
		if (!IsNum(c) && !IsAlpha(c) && c != '_' && c != '.') {
			if (enter)
				break;
			continue;
		}
		if (c == '/') {
			if (i + 1 < line.size()) {
				if (line[i + 1] == '/')
					break;
			}
		}

		enter = true;
		if (c == '.')
			word += "::";
		else
			word += c;
	}

	return i;
}

static int ReadNextValue(string &line, int begin, string &word) {
	bool enter = false;
	int i;
	for (i = begin; i < line.size(); i++) {
		char c = line[i];
		if (!IsNum(c) && !IsAlpha(c) && c != '.' && c != '-') {
			if (enter)
				break;
			continue;
		}
		if (c == '/') {
			if (i + 1 < line.size()) {
				if (line[i + 1] == '/')
					break;
			}
		}

		enter = true;
		word += c;
	}

	return i;
}

static int ReadNextDefault(string &line, int begin, string &def) {
	size_t pos = line.find("default", begin);
	if (pos == string::npos)
		return begin;
	return ReadNextValue(line, (int)pos + strlen("default"), def);
}

static void ToUpper(string &str) {
	for (int i = 0; i < str.size(); i++) {
		if (str[i] >= 'a' && str[i] <= 'z')
			str[i] = 'A' + (str[i]  - 'a');
	}
}

static void ToUpperFirst(string &str) {
	for (int i = 0; i < str.size(); i++) {
		if (str[i] >= 'a' && str[i] <= 'z') {
			str[i] = 'A' + (str[i]  - 'a');
			return;
		}
	}
}

static void WriteDepend(ofstream &out, string &name) {
	string upperName = name;
	ToUpper(upperName);

	out << "#ifndef _" << upperName << "_HPP_" << '\n';
	out << "#define _" << upperName << "_HPP_" << "\n\n";
}

static void EndDepend(ofstream &out) {
	out << "#endif" << '\n';
}

static void WriteInclude(Content &content, ofstream &out) {
	out << "#include \"" << content.name << ".pb.h\"\n";
	for (list<string>::iterator it = content.header.begin(); it != content.header.end(); it++) {
		out << "#include \"" << *it << ".hpp\"" << '\n';
		out << "#include \"" << *it << ".pb.h\"" << '\n';
	}
	out << "#include <sys/types.h>" << '\n';
	out << "#include <cassert>" << "\n";
	out << "#include <string>" << "\n";
	out << "#include <cstring>" << "\n\n";
}

static void ScanContent(ifstream &in, Content &content) {
	string line;
	for (;;) {
		line.clear();
		getline(in, line);
		if (in.eof() || in.fail())
			break;

		size_t pos = line.find("import ");
		if (pos != string::npos) {
			string header;
			ReadNextWord(line, pos + strlen("import "), header);
			assert(!header.empty());
			content.header.push_back(header);
		}

		pos = line.find("message ");
		if (pos != string::npos) {
			Message message;
			ReadNextWord(line, pos + strlen("message "), message.name);
			assert(!message.name.empty());

			for (;;) {
				line.clear();
				getline(in, line);
				assert(!in.eof() && !in.fail());

				if (line.find("}") != string::npos)
					break;

				pos = line.find("enum ");
				if (pos != string::npos) {
					Enum enum_;
					ReadNextWord(line, pos + strlen("enum "), enum_.name);
					assert(!enum_.name.empty());

					for (;;) {
						line.clear();
						getline(in, line);
						assert(!in.eof() && !in.fail());

						if (line.find("}") != string::npos)
							break;

						string key;
						int next = ReadNextWord(line, 0, key);
						if (key.empty())
							continue;

						string value;
						ReadNextWord(line, next, value);
						assert(!value.empty());

						enum_.fields.push_back(make_pair(key, value));
					}

					message.enums.push_back(enum_);
				}

				pos = line.find("optional ");
				if (pos == string::npos)
					pos = line.find("repeated ");
				if (pos != string::npos) {
					Field field;
					int next = ReadNextWord(line, pos, field.rule);
					assert(!field.rule.empty());

					next = ReadNextType(line, next, field.type);
					assert(!field.type.empty());

					next = ReadNextWord(line, next, field.name);
					assert(!field.name.empty());

					ReadNextDefault(line, next, field.def);

					if (field.rule == "repeated" && field.type == "string") {
						cout << "Repeated string is not supported, message: " << message.name << endl;
						continue;
					}
					message.fields.push_back(field);
				}
			}

			content.messages.push_back(message);
		}
	}
}

static void WriteEnum(Message &message, ofstream &out) {
	if (message.enums.empty())
		return;

	out << "\tpublic:\n";

	for (list<Enum>::iterator it = message.enums.begin(); it != message.enums.end(); it++) {
		Enum &enum_ = *it;
		out << "\t\tenum " << enum_.name << " {\n";

		for (list<pair<string, string> >::iterator keyvalue = enum_.fields.begin(); keyvalue != enum_.fields.end(); keyvalue++)
			out << "\t\t\t" << keyvalue->first << " = " << keyvalue->second << ",\n";

		out << "\t\t};\n";

		static char buffer[1024];
		sprintf(buffer, "\t\tstatic bool %s_IsValid(int value) {\n", enum_.name.c_str());
		out << buffer;

		out << "\t\t\tswitch(value) {\n";
		for (list<pair<string, string> >::iterator keyvalue = enum_.fields.begin(); keyvalue != enum_.fields.end(); keyvalue++) {
			sprintf(buffer, "\t\t\t\tcase %s:\n", keyvalue->first.c_str());
			out << buffer;
		}
		if (!enum_.fields.empty())
			out << "\t\t\t\t\treturn true;\n";
		out << "\t\t\t\tdefault:\n";
		out << "\t\t\t\t\treturn false;\n";

		out << "\t\t\t}\n";

		out << "\t\t}\n\n";
	}
}

static bool IsEnum(Message &message, const string &type) {
	for (list<Enum>::iterator it = message.enums.begin(); it != message.enums.end(); it++) {
		if (type == it->name)
			return true;
	}
	if (type.find("::") != string::npos)
		return true;

	return false;
}

static inline bool IsInnerEnum(const string &type) {
	return type.find("::") == string::npos;
}

static void FindEnumValue(Message &message, const string &key, string &value) {
	for (list<Enum>::iterator it = message.enums.begin(); it != message.enums.end(); it++) {
		Enum &enum_ = *it;
		for (list<pair<string, string> >::iterator keyvalue = enum_.fields.begin(); keyvalue != enum_.fields.end(); keyvalue++) {
			if (keyvalue->first == key) {
				value = keyvalue->second;
				return;
			}
		}
	}
}

static void DefaultValue(Message &message, Field &field, string &def) {
	if (field.type == "int32" || field.type == "int64") {
		if (!field.def.empty())
			def = field.def;
		else
			def = "0";
	} else if (field.type == "float") {
		if (!field.def.empty())
			def = field.def;
		else
			def = "0.0f";
	} else if (field.type == "bool") {
		if (!field.def.empty())
			def = field.def;
		else
			def = "false";
	} else if (field.type == "string") {
		if (!field.def.empty())
			cout << "Default value of string is not supported, field: " << field.name << endl;
	} else if (IsEnum(message, field.type)) {
		if (!field.def.empty()) {
			def = field.def;
		} else {
			def = "(" + field.type + ")0";
		}
	}
}

static void WriteConstructor(Message &message, ofstream &out) {
	out << "\tpublic:\n";
	out << "\t\t" << message.name << "() {\n";

	static char buffer[1024];

	for (list<Field>::iterator it = message.fields.begin(); it != message.fields.end(); it++) {
		Field &field = *it;
		const char *type = field.type.c_str();
		const char *name = field.name.c_str();

		string def;
		DefaultValue(message, field, def);
		if (field.type == "int32" || field.type == "int64") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = %s;\n", name, def.c_str());
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
			} else {
				assert(0);
			}
		} else if (field.type == "float") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = %s;\n", name, def.c_str());
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
			} else {
				assert(0);
			}
		} else if (field.type == "bool") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = %s;\n", name, def.c_str());
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
			} else {
				assert(0);
			}
		} else if (field.type == "string") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_[0] = '\\0';\n", name);
				out << buffer;
			}
		} else if (IsEnum(message, field.type)) {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = %s;\n", name, def.c_str());
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
			} else {
				assert(0);
			}
		}
	}

	out << "\t\t}\n\n";
}

static void WriteConvert(Message &message, ofstream &out) {
	out << "\tpublic:\n";

	static char buffer[1024];
	sprintf(buffer, "\t\tvoid ToPB(PB_%s *pb_) const {\n", message.name.c_str());
	out << buffer;

	sprintf(buffer, "\t\t\tpb_->Clear();\n");
	out << buffer;

	for (list<Field>::iterator it = message.fields.begin(); it != message.fields.end(); it++) {
		Field &field = *it;
		const char *type = field.type.c_str();
		const char *name = field.name.c_str();

		if (field.type == "int32"
				|| field.type == "int64"
				|| field.type == "float"
				|| field.type == "bool"
				|| field.type == "string") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\tpb_->set_%s(%s_);\n", name, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tpb_->add_%s(%s_[i]);\n", name, name);
				out << buffer;
			} else {
				assert(0);
			}
		} else if (IsEnum(message, field.type)) {
			if (field.rule == "optional") {
				if (!IsInnerEnum(type))
					sprintf(buffer, "\t\t\tpb_->set_%s((PB_%s)%s_);\n", name, type, name);
				else
					sprintf(buffer, "\t\t\tpb_->set_%s((PB_%s::%s)%s_);\n", name, message.name.c_str(), type, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				if (!IsInnerEnum(type))
					sprintf(buffer, "\t\t\t\tpb_->add_%s((PB_%s)%s_[i]);\n", name, type, name);
				else
					sprintf(buffer, "\t\t\t\tpb_->add_%s((PB_%s::%s)%s_[i]);\n", name, message.name.c_str(), type, name);
				out << buffer;
			} else {
				assert(0);
			}
		} else {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_.ToPB(pb_->mutable_%s());\n", name, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t%s_[i].ToPB(pb_->add_%s());\n", name, name);
				out << buffer;
			} else {
				assert(0);
			}
		}
	}

	out << "\t\t}\n";

	sprintf(buffer, "\t\tvoid FromPB(const PB_%s *pb_) {\n", message.name.c_str());
	out << buffer;

	for (list<Field>::iterator it = message.fields.begin(); it != message.fields.end(); it++) {
		Field &field = *it;
		const char *type = field.type.c_str();
		const char *name = field.name.c_str();

		string def;
		DefaultValue(message, field, def);
		if (field.type == "int32"
				|| field.type == "int64"
				|| field.type == "float"
				|| field.type == "bool") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = pb_->%s();\n", name, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tif (%s_size() <= pb_->%s_size()) {\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = pb_->%s(i);\n", name, name);
				out << buffer;
				out << "\t\t\t} else {\n";
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < pb_->%s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = pb_->%s(i);\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = pb_->%s_size(); i < %s_size(); i++)\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
				out << "\t\t\t}\n";
			} else {
				assert(0);
			}
		} else if (IsEnum(message, field.type)) {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_ = (%s)pb_->%s();\n", name, type, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tif (%s_size() <= pb_->%s_size()) {\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = (%s)pb_->%s(i);\n", name, type, name);
				out << buffer;
				out << "\t\t\t} else {\n";
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < pb_->%s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = (%s)pb_->%s(i);\n", name, type, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = pb_->%s_size(); i < %s_size(); i++)\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = %s;\n", name, def.c_str());
				out << buffer;
				out << "\t\t\t}\n";
			} else {
				assert(0);
			}
		} else if (field.type == "string") {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\tstrncpy(%s_, pb_->%s().c_str(), %s_size() - 1);\n", name, name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t%s_[%s_size() - 1] = '\\0';\n", name, name);
				out << buffer;
			} else {
				assert(0);
			}
		} else {
			if (field.rule == "optional") {
				sprintf(buffer, "\t\t\t%s_.FromPB(&pb_->%s());\n", name, name);
				out << buffer;
			} else if (field.rule == "repeated") {
				sprintf(buffer, "\t\t\tif (%s_size() <= pb_->%s_size()) {\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < %s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i].FromPB(&pb_->%s(i));\n", name, name);
				out << buffer;
				out << "\t\t\t} else {\n";
				sprintf(buffer, "\t\t\t\tfor (int i = 0; i < pb_->%s_size(); i++)\n", name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i].FromPB(&pb_->%s(i));\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\tfor (int i = pb_->%s_size(); i < %s_size(); i++)\n", name, name);
				out << buffer;
				sprintf(buffer, "\t\t\t\t\t%s_[i] = %s();\n", name, type);
				out << buffer;
				out << "\t\t\t}\n";
			} else {
				assert(0);
			}
		}
	}

	out << "\t\t}\n\n";
}

static void WriteNormalAccessor(Field &field, ofstream &out) {
	static char buffer[1024];
	const char *type = field.type.c_str();
	const char *name = field.name.c_str();

	if (field.rule == "optional") {
		if (field.type == "int32" || field.type == "int64") {
			sprintf(buffer, "\t\tinline %s_t %s() const {\n", type, name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(%s_t value) {\n", name, type);
			out << buffer;
			sprintf(buffer, "\t\t\t%s_ = value;\n", name);
			out << buffer;
			out << "\t\t}\n";
		} else if (field.type == "string") {
			sprintf(buffer, "\t\tinline const char * %s() const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(const char * value) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\tstrncpy(%s_, value, %s_size() - 1);\n", name, name);
			out << buffer;
			sprintf(buffer, "\t\t\t%s_[%s_size() - 1] = '\\0';\n", name, name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline int %s_size() const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn (int)(sizeof(%s_) / sizeof(%s_[0]));\n", name, name);
			out << buffer;
			out << "\t\t}\n";
		} else {
			sprintf(buffer, "\t\tinline %s %s() const {\n", type, name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(%s value) {\n", name, type);
			out << buffer;
			sprintf(buffer, "\t\t\t%s_ = value;\n", name);
			out << buffer;
			out << "\t\t}\n";
		}
	} else if (field.rule == "repeated") {
		if (field.type == "int32" || field.type == "int64") {
			sprintf(buffer, "\t\tinline %s_t %s(int index) const {\n", type, name);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_[index];\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(int index, %s_t value) {\n", name, type);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\t%s_[index] = value;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline int %s_size() const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn (int)(sizeof(%s_) / sizeof(%s_[0]));\n", name, name);
			out << buffer;
			out << "\t\t}\n";
		} else if (field.type == "string") {
			sprintf(buffer, "\t\tinline const std::string & %s(int index) const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_[index];\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(int index, const std::string &value) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\t%s_[index] = value;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline int %s_size() const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn (int)(sizeof(%s_) / sizeof(%s_[0]));\n", name, name);
			out << buffer;
			out << "\t\t}\n";
		} else {
			sprintf(buffer, "\t\tinline %s %s(int index) const {\n", type, name);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\treturn %s_[index];\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline void set_%s(int index, %s value) {\n", name, type);
			out << buffer;
			sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\t\tassert(0);\n");
			out << buffer;
			sprintf(buffer, "\t\t\t}\n");
			out << buffer;
			sprintf(buffer, "\t\t\t%s_[index] = value;\n", name);
			out << buffer;
			out << "\t\t}\n";

			sprintf(buffer, "\t\tinline int %s_size() const {\n", name);
			out << buffer;
			sprintf(buffer, "\t\t\treturn (int)(sizeof(%s_) / sizeof(%s_[0]));\n", name, name);
			out << buffer;
			out << "\t\t}\n";
		}
	} else {
		assert(0);
	}
}

static void WriteClassAccessor(Field &field, ofstream &out) {
	static char buffer[1024];
	const char *type = field.type.c_str();
	const char *name = field.name.c_str();

	if (field.rule == "optional") {
		sprintf(buffer, "\t\tinline const %s & %s() const {\n", type, name);
		out << buffer;
		sprintf(buffer, "\t\t\treturn %s_;\n", name);
		out << buffer;
		out << "\t\t}\n";

		sprintf(buffer, "\t\tinline %s * mutable_%s() {\n", type, name);
		out << buffer;
		sprintf(buffer, "\t\t\treturn &%s_;\n", name);
		out << buffer;
		out << "\t\t}\n";
	} else if (field.rule == "repeated") {
		sprintf(buffer, "\t\tinline const %s & %s(int index) const {\n", type, name);
		out << buffer;
		sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
		out << buffer;
		sprintf(buffer, "\t\t\t\tassert(0);\n");
		out << buffer;
		sprintf(buffer, "\t\t\t}\n");
		out << buffer;
		sprintf(buffer, "\t\t\treturn %s_[index];\n", name);
		out << buffer;
		out << "\t\t}\n";

		sprintf(buffer, "\t\tinline %s * mutable_%s(int index) {\n", type, name);
		out << buffer;
		sprintf(buffer, "\t\t\tif (index < 0 || index >= %s_size()) {\n", name);
		out << buffer;
		sprintf(buffer, "\t\t\t\tassert(0);\n");
		out << buffer;
		sprintf(buffer, "\t\t\t}\n");
		out << buffer;
		sprintf(buffer, "\t\t\treturn &%s_[index];\n", name);
		out << buffer;
		out << "\t\t}\n";

		sprintf(buffer, "\t\tinline int %s_size() const {\n", name);
		out << buffer;
		sprintf(buffer, "\t\t\treturn (int)(sizeof(%s_) / sizeof(%s_[0]));\n", name, name);
		out << buffer;
		out << "\t\t}\n";
	} else {
		assert(0);
	}
}

static void WriteAccessor(Message &message, ofstream &out) {
	if (message.fields.empty())
		return;

	out << "\tpublic:\n";

	for (list<Field>::iterator it = message.fields.begin(); it != message.fields.end(); it++) {
		Field &field = *it;

		if (field.type == "int32"
				|| field.type == "int64"
				|| field.type == "float"
				|| field.type == "bool"
				|| field.type == "string"
				|| IsEnum(message, field.type)) {
			WriteNormalAccessor(field, out);
		} else {
			WriteClassAccessor(field, out);
		}

		out << "\n";
	}
}

static void FindArraySize(Message &message, Field &field, string &size) {
	string sizeType = field.name + "SizeType";
	ToUpperFirst(sizeType);
	if (!IsEnum(message, sizeType))
		return;

	string key = field.name + "_SIZE";
	ToUpper(key);
	FindEnumValue(message, key, size);
	if (size.empty())
		return;
}

static void WriteField(Message &message, ofstream &out) {
	if (message.fields.empty())
		return;

	out << "\tprivate:\n";

	for (list<Field>::iterator it = message.fields.begin(); it != message.fields.end(); it++) {
		Field &field = *it;

		static char buffer[1024];

		if (field.rule == "optional") {
			if (field.type == "int32" || field.type == "int64") {
				sprintf(buffer, "\t\t%s_t %s_;\n", field.type.c_str(), field.name.c_str());
			} else if (field.type == "string") {
				string size;
				FindArraySize(message, field, size);
				if (!size.empty()) {
					sprintf(buffer, "\t\tchar %s_[%s];\n", field.name.c_str(), size.c_str());
				} else {
					cout << "You should write array number of field \"" << field.name << "\" in class \"" << message.name << "\" by yourself." << endl;
					sprintf(buffer, "\t\tchar %s_[];\n", field.name.c_str());
				}
			} else {
				sprintf(buffer, "\t\t%s %s_;\n", field.type.c_str(), field.name.c_str());
			}
		} else if (field.rule == "repeated") {
			string size;
			FindArraySize(message, field, size);

			if (size.empty())
				cout << "You should write array number of field \"" << field.name << "\" in class \"" << message.name << "\" by yourself." << endl;

			if (field.type == "int32" || field.type == "int64") {
				if (!size.empty())
					sprintf(buffer, "\t\t%s_t %s_[%s];\n", field.type.c_str(), field.name.c_str(), size.c_str());
				else
					sprintf(buffer, "\t\t%s_t %s_[];\n", field.type.c_str(), field.name.c_str());
			} else if (field.type == "string") {
				if (!size.empty())
					sprintf(buffer, "\t\tstd::string %s_[%s];\n", field.name.c_str(), size.c_str());
				else
					sprintf(buffer, "\t\tstd::string %s_[];\n", field.name.c_str());
			} else {
				if (!size.empty())
					sprintf(buffer, "\t\t%s %s_[%s];\n", field.type.c_str(), field.name.c_str(), size.c_str());
				else
					sprintf(buffer, "\t\t%s %s_[];\n", field.type.c_str(), field.name.c_str());
			}
		} else {
			assert(0);
		}

		out << buffer;
		out << "\n";
	}
}

static void WriteClass(Message &message, ofstream &out) {
	out << "class " << message.name << " {" << "\n\n";
	WriteEnum(message, out);
	WriteConstructor(message, out);
	WriteConvert(message, out);
	WriteAccessor(message, out);
	WriteField(message, out);
	out << "};\n\n";
}

static void WriteContent(Content &content, ofstream &out) {
	WriteInclude(content, out);
	for (list<Message>::iterator it = content.messages.begin(); it != content.messages.end(); it++)
		WriteClass(*it, out);
}

static void Help(const char *me) {
	cout << "Usage: " << me << " -o dir [proto file in cur dir]" << endl;
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		Help(argv[0]);
		exit(0);
	}

	string outDir;
	for (int i = 1; i < argc; i++) {
		string arg = argv[i];
		if (arg == "-o") {
			if (i + 1 < argc) {
				outDir = argv[i + 1];
				if (outDir[outDir.size() - 1] != '/')
					outDir += '/';
				break;
			}
		}
	}
	if (outDir.empty()) {
		Help(argv[0]);
		exit(0);
	}

	for (int i = 1; i < argc; i++) {
		string proto = argv[i];

		if (proto == "-o") {
			i++;
			continue;
		}

		if (proto.find("/") != string::npos) {
			Help(argv[0]);
			exit(0);
		}

		size_t end = proto.find(".proto");
		if (end == string::npos) {
			Help(argv[0]);
			exit(0);
		}

		string protoName = proto.substr(0, end);
		string outName = outDir + protoName + ".hpp";

		ifstream in(proto.c_str());
		if (in.fail()) {
			cerr << "Failed to open file: " << proto << endl;
			continue;
		}

		ofstream out(outName.c_str());
		if (out.fail()) {
			cerr << "Failed to create file: " << outName << endl;
			continue;
		}

		Content content;
		content.name = protoName;
		ScanContent(in, content);

		WriteDepend(out, protoName);
		WriteContent(content, out);
		EndDepend(out);
	}

	return 0;
}
