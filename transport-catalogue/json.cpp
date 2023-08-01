#include "json.hpp"
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <variant>

namespace json {

using namespace std;

namespace {

/* Since int is a subset of double, there is a definite need 
   for a type, such as "number" to parse the document */
using Number = std::variant<double, int>;

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    bool is_correct = false;

    for (char c; input >> c;) {
        if (c != ',') {
            input.putback(c);
        }

        if (c == ']') {
            is_correct = true;
            input >> c;
            break;
        }
        result.push_back(LoadNode(input));
    }

    if (!is_correct) throw ParsingError("Array started with \"[\" but didn't end with \"]\""s);

    return Node(std::move(result));
}


std::string LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

Node LoadDict(istream& input) {
    Dict result;

    bool is_correct = false;

    for (char c; input >> c;) {
        if (c == ',') {
            input >> c;
        }

        if (c == '}') {
            is_correct = true;
            break;
        }

        string key = LoadString(input);
        input >> c;
        result.insert({std::move(key), LoadNode(input)});
    }

    if (!is_correct) throw ParsingError("Map started with \"{\" but didn't end with \"}\""s);

    return Node(std::move(result));
}

std::string GetStringFromInput(std::istream& input) {
    std::string result;
    char c;

    while (input >> c) {
        result += c;
    }

    return result;
}

Number LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected, got \"" + GetStringFromInput(input) + "\""); 
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }   
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadNull(std::istream& input) {
    char c;
    std::string result;
    for (int i = 0; input >> c && i < 4 ; i++) {
        result += c;
    }
    
    if (result != "null"s) throw ParsingError("Parsing failed: expected true, false or null, got" + result);
    
    return Node{nullptr};
}

Node LoadBool(std::istream& input) {
    std::string result;

    char c;

    int token_length = input.peek() == 't' ? 4 : 5;

    for (int i = 0; i < token_length; i++) {
        input >> c;
        result += c;
    }

    if (result == "true") return Node(true);
    else if (result == "false") return Node(false);
    throw ParsingError("Parsing failed: expected true, false of null, got " + result);
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return Node(LoadString(input));
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else {
        input.putback(c);
        Number num = LoadNumber(input);

        if (std::holds_alternative<int>(num)) 
            return Node(std::get<int>(num));
        else 
            return Node(std::get<double>(num));
    }
}

}  // namespace

// Various constructors
Node::Node(int value)          : val_(value) {}
Node::Node(bool value)         : val_(value) {}
Node::Node(double value)       : val_(value) {}
Node::Node(Array value)        : val_(value) {}
Node::Node(std::string value)  : val_(value) {}
Node::Node(Dict value)         : val_(value) {}

const Node::Value& Node::GetValue() const {
    return val_;
}

// Is-methods
bool Node::IsInt() const {
    return std::holds_alternative<int>(val_);
}

bool Node::IsDouble() const {
    return IsInt() || IsPureDouble();
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(val_);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(val_);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(val_);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(val_);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(val_);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(val_);
}

// As-methods
int Node::AsInt() const {
    if (IsInt()) return std::get<int>(val_);
    throw std::logic_error("No int in val_");
}

bool Node::AsBool() const {
    if (IsBool()) return std::get<bool>(val_);
    throw std::logic_error("No bool in val_");
}

const std::string& Node::AsString() const {
    if (IsString()) return std::get<std::string>(val_);
    throw std::logic_error("No std::string in val_");
}

const Array& Node::AsArray() const {
    if (IsArray()) return std::get<Array>(val_);
    throw std::logic_error("No Array (std::vector<Node>) in val_");
}

const Dict& Node::AsMap() const {
    if (IsMap()) return std::get<Dict>(val_);
    throw std::logic_error("No Dict (std::map<std::string, Node>) in val_");
}

double Node::AsDouble() const {
    if (IsPureDouble()) return std::get<double>(val_);
    else if (IsInt()) return static_cast<double>(std::get<int>(val_));
    throw std::logic_error("No double in val_");
}

bool Node::operator==(const Node& other) const {
    return this->GetValue() == other.GetValue(); 
}

bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

bool Document::operator==(const Document& other) const {
    return GetRoot() == other.GetRoot();
}

bool Document::operator!=(const Document& other) const {
    return !(GetRoot() == other.GetRoot());
}

void PrintValue(std::ostream& out, const double value) {
    out << value;
}

void PrintValue(std::ostream& out, const int value) {
    out << value;
}

void PrintValue(std::ostream& out, const Array values) {
    out << "[ ";
    bool is_first = true;
    for (const Node& node : values) {
        if (!is_first) {
            out << ", ";
        }
        is_first = false;
        PrintNode(out, node);
    }
    out << " ]";
}

void PrintValue(std::ostream& out, const std::nullptr_t) {
    out << "null";
}

void PrintValue(std::ostream& out, const std::string& value) {
    out << '\"';
    for (const char c : value) {
        switch (c) {
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            default:
                out << c;
                break;
        }
    }
    out << '\"';
}

void PrintValue(std::ostream& out, const Dict values) {
    out << "{ ";
    bool is_first = true;
    for (const auto& [key, node] : values) {
        if (!is_first) {
            out << ", ";
        }
        is_first = false;
        out << "\"" << key << "\": ";
        PrintNode(out, node);
    }
    out << " }";
}

void PrintValue(std::ostream& out, const bool value) {
    out << (value ? "true" : "false");
}

void PrintNode(std::ostream& out, const Node& value) {
    std::visit([&out](const auto& value) { 
        PrintValue(out, value); 
    }, value.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    PrintNode(output, doc.GetRoot());
}

}  // namespace json