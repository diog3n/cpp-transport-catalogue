#include "json.h"
#include <stack>


namespace json {

class Builder;

class Builder {
private:

    /* Private classes that represent different contexts */

    class BaseContext;

    class KeyContext;
    
    class DictContext;
    
    class ArrayContext;

    enum ContextType {
        ARRAY,
        DICT,
        KEY,
        GENERAL,
        VALUE
    };

public:

    Builder();

    DictContext StartDict();
    
    Builder& EndDict();
    
    KeyContext Key(std::string key);
    
    Builder& Value(Node val);
    
    ArrayContext StartArray();
    
    Builder& EndArray();
    
    Node Build();
    
    ~Builder() = default;

private:

    void CompleteContainer();
    
    std::stack<ContextType> ctx_stack_;

    std::vector<Node> nodes_stack_;

    Node root_;

};

class Builder::BaseContext {
public:

    BaseContext(Builder& builder);

    DictContext StartDict();
    
    Builder& EndDict();
    
    KeyContext Key(std::string key);
    
    Builder& Value(Node& val);
    
    ArrayContext StartArray();
    
    Builder& EndArray();

protected:
    
    Builder& builder_;

};

class Builder::KeyContext: private BaseContext {
public:

    KeyContext(Builder& builder);

    DictContext Value(Node val);

};

class Builder::DictContext: private BaseContext {
public:

    using BaseContext::Key;
    using BaseContext::EndDict;

    DictContext(Builder& builder);

};

class Builder::ArrayContext: private BaseContext {
public:

    using BaseContext::EndArray;

    ArrayContext(Builder& builder);

    ArrayContext Value(Node val);

};

} // namespace json