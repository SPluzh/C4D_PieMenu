#include "piemenu_json.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>

namespace cinema
{

static const JsonValue s_nullValue;

JsonValue JsonValue::Object()
{
    JsonValue v;
    v.type = JsonType::Object;
    return v;
}

JsonValue JsonValue::Array()
{
    JsonValue v;
    v.type = JsonType::Array;
    return v;
}

void JsonValue::Set(const std::string& key, const JsonValue& val)
{
    if (type != JsonType::Object)
    {
        type = JsonType::Object;
        objVal.clear();
    }
    for (size_t i = 0; i < objVal.size(); ++i)
    {
        if (objVal[i].first == key)
        {
            objVal[i].second = val;
            return;
        }
    }
    objVal.push_back(std::make_pair(key, val));
}

const JsonValue* JsonValue::Find(const std::string& key) const
{
    if (type != JsonType::Object)
        return nullptr;
    for (size_t i = 0; i < objVal.size(); ++i)
    {
        if (objVal[i].first == key)
            return &objVal[i].second;
    }
    return nullptr;
}

std::string JsonValue::GetString(const std::string& key, const std::string& def) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsString())
        return v->strVal;
    return def;
}

double JsonValue::GetNumber(const std::string& key, double def) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsNumber())
        return v->numVal;
    return def;
}

int64_t JsonValue::GetInt(const std::string& key, int64_t def) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsNumber())
        return (int64_t)v->numVal;
    return def;
}

bool JsonValue::GetBool(const std::string& key, bool def) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsBool())
        return v->boolVal;
    return def;
}

const std::vector<JsonValue>* JsonValue::GetArray(const std::string& key) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsArray())
        return &v->arrVal;
    return nullptr;
}

const JsonValue* JsonValue::GetObject(const std::string& key) const
{
    const JsonValue* v = Find(key);
    if (v && v->IsObject())
        return v;
    return nullptr;
}

void JsonValue::Push(const JsonValue& val)
{
    if (type != JsonType::Array)
    {
        type = JsonType::Array;
        arrVal.clear();
    }
    arrVal.push_back(val);
}

size_t JsonValue::Size() const
{
    if (type == JsonType::Array)
        return arrVal.size();
    if (type == JsonType::Object)
        return objVal.size();
    return 0;
}

const JsonValue& JsonValue::operator[](size_t idx) const
{
    if (type == JsonType::Array && idx < arrVal.size())
        return arrVal[idx];
    return s_nullValue;
}

static void EscapeString(const std::string& in, std::string& out)
{
    out.push_back('"');
    for (size_t i = 0; i < in.size(); ++i)
    {
        char c = in[i];
        switch (c)
        {
            case '"':  out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\b': out.append("\\b"); break;
            case '\f': out.append("\\f"); break;
            case '\n': out.append("\\n"); break;
            case '\r': out.append("\\r"); break;
            case '\t': out.append("\\t"); break;
            default:
            {
                if ((unsigned char)c < 0x20)
                {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    out.append(buf);
                }
                else
                {
                    out.push_back(c);
                }
                break;
            }
        }
    }
    out.push_back('"');
}

static void FormatNumber(double d, std::string& out)
{
    if (std::isnan(d) || std::isinf(d))
    {
        out.append("0");
        return;
    }
    double intPart;
    if (std::modf(d, &intPart) == 0.0 && d >= -9e15 && d <= 9e15)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%lld", (long long)intPart);
        out.append(buf);
    }
    else
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.4f", d);
        // Trim trailing zeros after decimal point
        size_t len = std::strlen(buf);
        while (len > 0 && buf[len - 1] == '0')
        {
            buf[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buf[len - 1] == '.')
        {
            buf[len - 1] = '\0';
            len--;
        }
        out.append(buf);
    }
}

static bool IsPrimitiveNumberArray(const std::vector<JsonValue>& arr)
{
    if (arr.empty() || arr.size() > 4)
        return false;
    for (size_t i = 0; i < arr.size(); ++i)
    {
        if (!arr[i].IsNumber())
            return false;
    }
    return true;
}

void JsonValue::DumpInternal(std::string& out, int indent, int currentIndent) const
{
    switch (type)
    {
        case JsonType::Null:
            out.append("null");
            break;
        case JsonType::Bool:
            out.append(boolVal ? "true" : "false");
            break;
        case JsonType::Number:
            FormatNumber(numVal, out);
            break;
        case JsonType::String:
            EscapeString(strVal, out);
            break;
        case JsonType::Array:
        {
            if (arrVal.empty())
            {
                out.append("[]");
                break;
            }
            if (IsPrimitiveNumberArray(arrVal))
            {
                out.push_back('[');
                for (size_t i = 0; i < arrVal.size(); ++i)
                {
                    if (i > 0)
                        out.append(", ");
                    FormatNumber(arrVal[i].numVal, out);
                }
                out.push_back(']');
                break;
            }
            out.append("[\n");
            int nextIndent = currentIndent + indent;
            for (size_t i = 0; i < arrVal.size(); ++i)
            {
                out.append(nextIndent, ' ');
                arrVal[i].DumpInternal(out, indent, nextIndent);
                if (i + 1 < arrVal.size())
                    out.push_back(',');
                out.push_back('\n');
            }
            out.append(currentIndent, ' ');
            out.push_back(']');
            break;
        }
        case JsonType::Object:
        {
            if (objVal.empty())
            {
                out.append("{}");
                break;
            }
            out.append("{\n");
            int nextIndent = currentIndent + indent;
            for (size_t i = 0; i < objVal.size(); ++i)
            {
                out.append(nextIndent, ' ');
                EscapeString(objVal[i].first, out);
                out.append(": ");
                objVal[i].second.DumpInternal(out, indent, nextIndent);
                if (i + 1 < objVal.size())
                    out.push_back(',');
                out.push_back('\n');
            }
            out.append(currentIndent, ' ');
            out.push_back('}');
            break;
        }
    }
}

std::string JsonValue::Dump(int indent) const
{
    std::string out;
    DumpInternal(out, indent, 0);
    out.push_back('\n');
    return out;
}

// ============================================================================
// JSON PARSER IMPLEMENTATION
// ============================================================================

class JsonParser
{
public:
    JsonParser(const std::string& src) : m_src(src), m_pos(0) {}

    bool Parse(JsonValue& outVal, std::string& outErr)
    {
        // Skip UTF-8 BOM if present
        if (m_src.size() >= 3 &&
            (unsigned char)m_src[0] == 0xEF &&
            (unsigned char)m_src[1] == 0xBB &&
            (unsigned char)m_src[2] == 0xBF)
        {
            m_pos = 3;
        }

        SkipWhitespace();
        if (m_pos >= m_src.size())
        {
            outErr = "Empty input";
            return false;
        }

        if (!ParseValue(outVal))
        {
            outErr = m_err.empty() ? "Syntax error" : m_err;
            return false;
        }

        SkipWhitespace();
        if (m_pos < m_src.size())
        {
            outErr = "Unexpected trailing characters";
            return false;
        }

        return true;
    }

private:
    const std::string& m_src;
    size_t m_pos;
    std::string m_err;

    void SkipWhitespace()
    {
        while (m_pos < m_src.size())
        {
            char c = m_src[m_pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            {
                m_pos++;
            }
            else if (c == '/' && m_pos + 1 < m_src.size())
            {
                if (m_src[m_pos + 1] == '/')
                {
                    m_pos += 2;
                    while (m_pos < m_src.size() && m_src[m_pos] != '\n')
                        m_pos++;
                }
                else if (m_src[m_pos + 1] == '*')
                {
                    m_pos += 2;
                    while (m_pos + 1 < m_src.size() && !(m_src[m_pos] == '*' && m_src[m_pos + 1] == '/'))
                        m_pos++;
                    if (m_pos + 1 < m_src.size())
                        m_pos += 2;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    char Peek()
    {
        SkipWhitespace();
        return (m_pos < m_src.size()) ? m_src[m_pos] : '\0';
    }

    char Get()
    {
        SkipWhitespace();
        return (m_pos < m_src.size()) ? m_src[m_pos++] : '\0';
    }

    bool Match(const char* literal)
    {
        SkipWhitespace();
        size_t len = std::strlen(literal);
        if (m_pos + len <= m_src.size() && m_src.compare(m_pos, len, literal) == 0)
        {
            m_pos += len;
            return true;
        }
        return false;
    }

    bool ParseString(std::string& outStr)
    {
        if (Get() != '"')
            return false;

        outStr.clear();
        while (m_pos < m_src.size())
        {
            char c = m_src[m_pos++];
            if (c == '"')
                return true;
            if (c == '\\')
            {
                if (m_pos >= m_src.size())
                    return false;
                char esc = m_src[m_pos++];
                switch (esc)
                {
                    case '"':  outStr.push_back('"'); break;
                    case '\\': outStr.push_back('\\'); break;
                    case '/':  outStr.push_back('/'); break;
                    case 'b':  outStr.push_back('\b'); break;
                    case 'f':  outStr.push_back('\f'); break;
                    case 'n':  outStr.push_back('\n'); break;
                    case 'r':  outStr.push_back('\r'); break;
                    case 't':  outStr.push_back('\t'); break;
                    case 'u':
                    {
                        if (m_pos + 4 > m_src.size())
                            return false;
                        uint32_t cp = 0;
                        for (int i = 0; i < 4; ++i)
                        {
                            char h = m_src[m_pos++];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= (h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= (h - 'A' + 10);
                            else return false;
                        }
                        if (cp < 0x80)
                        {
                            outStr.push_back((char)cp);
                        }
                        else if (cp < 0x800)
                        {
                            outStr.push_back((char)(0xC0 | (cp >> 6)));
                            outStr.push_back((char)(0x80 | (cp & 0x3F)));
                        }
                        else
                        {
                            outStr.push_back((char)(0xE0 | (cp >> 12)));
                            outStr.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                            outStr.push_back((char)(0x80 | (cp & 0x3F)));
                        }
                        break;
                    }
                    default:
                        outStr.push_back(esc);
                        break;
                }
            }
            else
            {
                outStr.push_back(c);
            }
        }
        return false;
    }

    bool ParseNumber(double& outNum)
    {
        SkipWhitespace();
        size_t start = m_pos;
        if (m_pos < m_src.size() && (m_src[m_pos] == '-' || m_src[m_pos] == '+'))
            m_pos++;

        bool hasDigits = false;
        while (m_pos < m_src.size() && (m_src[m_pos] >= '0' && m_src[m_pos] <= '9'))
        {
            hasDigits = true;
            m_pos++;
        }
        if (m_pos < m_src.size() && m_src[m_pos] == '.')
        {
            m_pos++;
            while (m_pos < m_src.size() && (m_src[m_pos] >= '0' && m_src[m_pos] <= '9'))
            {
                hasDigits = true;
                m_pos++;
            }
        }
        if (hasDigits && m_pos < m_src.size() && (m_src[m_pos] == 'e' || m_src[m_pos] == 'E'))
        {
            m_pos++;
            if (m_pos < m_src.size() && (m_src[m_pos] == '-' || m_src[m_pos] == '+'))
                m_pos++;
            while (m_pos < m_src.size() && (m_src[m_pos] >= '0' && m_src[m_pos] <= '9'))
                m_pos++;
        }

        if (!hasDigits)
            return false;

        std::string numStr = m_src.substr(start, m_pos - start);
        outNum = std::strtod(numStr.c_str(), nullptr);
        return true;
    }

    bool ParseArray(JsonValue& outVal)
    {
        if (Get() != '[')
            return false;

        outVal = JsonValue::Array();
        SkipWhitespace();
        if (Peek() == ']')
        {
            Get();
            return true;
        }

        while (true)
        {
            JsonValue elem;
            if (!ParseValue(elem))
                return false;
            outVal.Push(elem);

            SkipWhitespace();
            char next = Peek();
            if (next == ',')
            {
                Get();
                SkipWhitespace();
                if (Peek() == ']') // allow trailing comma
                {
                    Get();
                    return true;
                }
            }
            else if (next == ']')
            {
                Get();
                return true;
            }
            else
            {
                m_err = "Expected ',' or ']' in array";
                return false;
            }
        }
    }

    bool ParseObject(JsonValue& outVal)
    {
        if (Get() != '{')
            return false;

        outVal = JsonValue::Object();
        SkipWhitespace();
        if (Peek() == '}')
        {
            Get();
            return true;
        }

        while (true)
        {
            SkipWhitespace();
            if (Peek() != '"')
            {
                m_err = "Expected string key in object";
                return false;
            }
            std::string key;
            if (!ParseString(key))
                return false;

            SkipWhitespace();
            if (Get() != ':')
            {
                m_err = "Expected ':' after key in object";
                return false;
            }

            JsonValue val;
            if (!ParseValue(val))
                return false;

            outVal.Set(key, val);

            SkipWhitespace();
            char next = Peek();
            if (next == ',')
            {
                Get();
                SkipWhitespace();
                if (Peek() == '}') // allow trailing comma
                {
                    Get();
                    return true;
                }
            }
            else if (next == '}')
            {
                Get();
                return true;
            }
            else
            {
                m_err = "Expected ',' or '}' in object";
                return false;
            }
        }
    }

    bool ParseValue(JsonValue& outVal)
    {
        SkipWhitespace();
        char c = Peek();
        if (c == '"')
        {
            std::string s;
            if (!ParseString(s))
                return false;
            outVal = JsonValue(s);
            return true;
        }
        if (c == '{')
            return ParseObject(outVal);
        if (c == '[')
            return ParseArray(outVal);
        if (c == 't')
        {
            if (!Match("true")) return false;
            outVal = JsonValue(true);
            return true;
        }
        if (c == 'f')
        {
            if (!Match("false")) return false;
            outVal = JsonValue(false);
            return true;
        }
        if (c == 'n')
        {
            if (!Match("null")) return false;
            outVal = JsonValue();
            return true;
        }
        if (c == '-' || c == '+' || (c >= '0' && c <= '9'))
        {
            double num = 0.0;
            if (!ParseNumber(num))
                return false;
            outVal = JsonValue(num);
            return true;
        }
        m_err = "Unexpected character while parsing value";
        return false;
    }
};

bool JsonValue::Parse(const std::string& text, JsonValue& outValue, std::string& outError)
{
    JsonParser parser(text);
    return parser.Parse(outValue, outError);
}

} // namespace cinema
