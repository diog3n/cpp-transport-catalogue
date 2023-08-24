#include "json.hpp"
#include <stack>


namespace json {

class Builder;

class Builder {
private:

    /* Private classes that represent different contexts */

    class BaseContext;

    class ValueContext;
    
    class ContainerContext;
    
    class GeneralContext;
    
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

protected:
    
    Builder& builder_;

};

class Builder::ValueContext: public BaseContext {
public:

    ValueContext(Builder& builder);

};

class Builder::ContainerContext: public BaseContext {
public:

    ContainerContext(Builder& builder);

    DictContext StartDict();

    ArrayContext StartArray();

};

class Builder::GeneralContext: public ContainerContext {
public:
    
    GeneralContext(Builder& builder);

    GeneralContext Value(Node val);

    Builder& EndDict();

    Builder& EndArray();

};

class Builder::KeyContext: public ContainerContext {
public:

    KeyContext(Builder& builder);

    DictContext Value(Node val);

};

class Builder::DictContext: public BaseContext {
public:

    DictContext(Builder& builder);

    KeyContext Key(std::string key);

    Builder& EndDict();

};

class Builder::ArrayContext: public ContainerContext {
public:

    ArrayContext(Builder& builder);

    ArrayContext Value(Node val);

    Builder& EndArray();

};

} // namespace json