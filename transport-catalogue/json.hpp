#pragma once

#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// An error that should be thrown when JSON is invalid
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

    // Returns a new context with deeper level of indentation
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

    const Value& GetValue() const;

    bool IsInt() const;

    // Returns true if val_ contains either double or int
    bool IsDouble() const;      

    // Returns true only if val_ contains double 
    bool IsPureDouble() const;

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
    
    double AsDouble() const;
    
    
    bool operator==(const Node& other) const;
    
    bool operator!=(const Node& other) const; 
    
};

// Houses JSON nodes
class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;

    bool operator!=(const Document& other) const;

private:

    Node root_;

};

// Loads a document from the input stream
Document Load(std::istream& input);

// Prints the whole json document
void Print(const Document& doc, std::ostream& output);

// Prints a node of the json
void PrintNode(std::ostream& out, const Node& node);

}  // namespace json