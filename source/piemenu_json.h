#ifndef PIEMENU_JSON_H__
#define PIEMENU_JSON_H__

#include <string>
#include <vector>
#include <cstdint>
#include <utility>

namespace cinema
{

enum class JsonType
{
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

class JsonValue
{
public:
    JsonType type = JsonType::Null;
    bool boolVal = false;
    double numVal = 0.0;
    std::string strVal;
    std::vector<JsonValue> arrVal;
    std::vector<std::pair<std::string, JsonValue>> objVal;

    JsonValue() : type(JsonType::Null) {}
    JsonValue(bool b) : type(JsonType::Bool), boolVal(b) {}
    JsonValue(double d) : type(JsonType::Number), numVal(d) {}
    JsonValue(int i) : type(JsonType::Number), numVal((double)i) {}
    JsonValue(int64_t i) : type(JsonType::Number), numVal((double)i) {}
    JsonValue(const char* s) : type(JsonType::String), strVal(s ? s : "") {}
    JsonValue(const std::string& s) : type(JsonType::String), strVal(s) {}

    static JsonValue Object();
    static JsonValue Array();

    bool IsNull() const { return type == JsonType::Null; }
    bool IsBool() const { return type == JsonType::Bool; }
    bool IsNumber() const { return type == JsonType::Number; }
    bool IsString() const { return type == JsonType::String; }
    bool IsArray() const { return type == JsonType::Array; }
    bool IsObject() const { return type == JsonType::Object; }

    // Object helpers
    void Set(const std::string& key, const JsonValue& val);
    const JsonValue* Find(const std::string& key) const;
    bool Has(const std::string& key) const { return Find(key) != nullptr; }

    std::string GetString(const std::string& key, const std::string& def = "") const;
    double GetNumber(const std::string& key, double def = 0.0) const;
    int64_t GetInt(const std::string& key, int64_t def = 0) const;
    bool GetBool(const std::string& key, bool def = false) const;
    const std::vector<JsonValue>* GetArray(const std::string& key) const;
    const JsonValue* GetObject(const std::string& key) const;

    // Array helpers
    void Push(const JsonValue& val);
    size_t Size() const;
    const JsonValue& operator[](size_t idx) const;

    // Serialization & Parsing
    std::string Dump(int indent = 2) const;
    static bool Parse(const std::string& text, JsonValue& outValue, std::string& outError);

private:
    void DumpInternal(std::string& out, int indent, int currentIndent) const;
};

} // namespace cinema

#endif // PIEMENU_JSON_H__
