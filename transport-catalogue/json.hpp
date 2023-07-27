#pragma once

#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

class Node final : private std::variant<std::nullptr_t, 
                                        Array, Dict, bool, 
                                        int, double, std::string>  {
public:
    using variant::variant;
    using Value = std::variant<std::nullptr_t, 
                               Array, Dict, bool, 
                               int, double, std::string>;
    Node() = default;                        
    
    Node(const int value);
    Node(const bool value);
    Node(const double value);
    Node(const Array value);
    Node(const std::string value);
    Node(const Dict value);
    
    const Value& GetValue() const;

    bool IsInt() const;
    bool IsDouble() const;      // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const;  // Возвращает true, если в Node хранится double.
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;
    double AsDouble() const;  // Возвращает значение типа double, если внутри хранится double 
                              // либо int. В последнем случае возвращается приведённое в 
                              // double значение.
    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const; 
private:
    Value val_;
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

void PrintNode(std::ostream& out, const Node& node);

}  // namespace json