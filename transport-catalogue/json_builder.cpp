#include "json_builder.hpp"
#include <stdexcept>

namespace json {

Builder::BaseContext::BaseContext(Builder& builder)
    : builder_(builder) {}

Builder::KeyContext::KeyContext(Builder& builder)
    : BaseContext(builder) {}

Builder::DictContext::DictContext(Builder& builder) 
    : BaseContext(builder) {}

Builder::ArrayContext::ArrayContext(Builder& builder)
    : BaseContext(builder) {}

Builder::DictContext Builder::BaseContext::StartDict() {
    return builder_.StartDict();
}

Builder& Builder::BaseContext::EndDict() {
    return builder_.EndDict();
}

Builder::KeyContext Builder::BaseContext::Key(std::string key) {
    return builder_.Key(key);
}

Builder& Builder::BaseContext::Value(Node& val) {
    return builder_.Value(val);
}

Builder::ArrayContext Builder::BaseContext::StartArray() {
    return builder_.StartArray();
}

Builder& Builder::BaseContext::EndArray() {
    return builder_.EndArray();
}

Builder::DictContext Builder::KeyContext::Value(Node val) {
    return builder_.Value(val);
}

Builder::ArrayContext Builder::ArrayContext::Value(Node val) {
    return builder_.Value(val);
}

Builder::Builder(): ctx_stack_({ GENERAL }) {}

Node Builder::Build() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Can't build an empty node");
    }

    if (ctx_stack_.top() != VALUE) {
        throw std::logic_error("Can't build a node from current context");
    }

    return nodes_stack_.front();
}

Builder::DictContext Builder::StartDict() {
    if (ctx_stack_.top() == DICT || ctx_stack_.top() == VALUE) {
        std::logic_error("Can't add an Array to the current context");
    }

    nodes_stack_.emplace_back(Dict());
    ctx_stack_.push(DICT);

    return DictContext(*this);
}

Builder& Builder::EndDict() {
    if (ctx_stack_.top() != DICT) {
        throw std::logic_error("Can't end dict in the current context");
    }

    CompleteContainer();

    return *this;
}

Builder::ArrayContext Builder::StartArray() {
    if (ctx_stack_.top() == DICT || ctx_stack_.top() == VALUE) {
        std::logic_error("Can't add an Array to the current context");
    }

    nodes_stack_.emplace_back(Array());
    ctx_stack_.push(ARRAY);

    return ArrayContext(*this);
}

Builder& Builder::EndArray() {
    if (ctx_stack_.top() != ARRAY) {
        throw std::logic_error("Can't end array in the current context");
    }

    CompleteContainer();

    return *this;
}

void Builder::CompleteContainer() {
    if (ctx_stack_.top() != ARRAY && ctx_stack_.top() != DICT) {
        throw std::logic_error("No container to end");
    }

    /* Returning to the context previous to the container's context */
    ctx_stack_.pop();

    /* Saving node and removing it from the stack */
    Node container_node = nodes_stack_.back();
    nodes_stack_.pop_back();

    /* Adding node to the parent context */
    Value(container_node);
}

Builder::KeyContext Builder::Key(std::string key) {
    if (ctx_stack_.top() != DICT) 
        throw std::logic_error("Key is being used outside of dictionary");

    nodes_stack_.emplace_back(std::move(key));
    ctx_stack_.push(KEY);

    return KeyContext(*this);
}

Builder& Builder::Value(Node val) {
    if (ctx_stack_.top() == KEY) {
        std::string node_key = nodes_stack_.back().AsString();
        nodes_stack_.pop_back();

        /* since there is no way to legitimately modify Node's contents
         * we have to use const_cast to edit what has already been created */
        const_cast<Dict&>(nodes_stack_.back().AsMap())[node_key] = std::move(val);

        /* After a value was assigned to the key, the key context doesn't
         * exist anymore */ 
        ctx_stack_.pop();

        return *this;
    } else if (ctx_stack_.top() == ARRAY) {

        /* Again, there is no legitimate way of modifying the context,
         * hence the const_cast() */
        const_cast<Array&>(nodes_stack_.back().AsArray()).emplace_back(std::move(val));

        return *this;
    } else if (ctx_stack_.top() == GENERAL) {
        nodes_stack_.emplace_back(std::move(val));

        /* VALUE blocks the context so that nothing else can be added */
        ctx_stack_.push(VALUE);

        return *this;
    } else if (ctx_stack_.top() == DICT) {
        throw std::logic_error("Value used without the key");
    } else {
        throw std::logic_error("Can't add value to the current context");
    }
}

} // namespace json