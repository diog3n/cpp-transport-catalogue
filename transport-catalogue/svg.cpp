#include <cassert>
#include <iostream>
#include <sstream>
#include <variant>

#include "svg.hpp"

namespace svg {

using namespace std::literals;

namespace utils {

std::string GetLineCapString(const StrokeLineCap& linecap) {
    std::ostringstream out;
    out << linecap;
    return out.str();
}

std::string GetLineJoinString(const StrokeLineJoin& join) {
    std::ostringstream out;
    out << join;
    return out.str();
}

std::string ReplaceSpecialChars(const std::string& str) {
    std::string result;
    for (auto iter = str.begin(); iter != str.end(); iter++) {
        if (*iter == '\"') result += "&quot;"s;
        else if (*iter == '\'') result += "&apos;"s;
        else if (*iter == '<') result += "&lt;"s;
        else if (*iter == '>') result += "&gt;"s;
        else if (*iter == '&') result += "&amp;"s;
        else result += *iter;
    }
    return result;
}

std::string GetColorString(const Color& color) {
    std::ostringstream out;
    out << color;
    return out.str();
}

} // namespace svg::utils

bool operator==(const Point& lhs, const Point& rhs) {
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

bool operator!=(const Point& lhs, const Point& rhs) {
    return !(lhs == rhs);
}

bool operator==(const Color& lhs, const Color& rhs) {
    if (std::holds_alternative<std::string>(lhs) && std::holds_alternative<std::string>(rhs)) {
        return std::get<std::string>(lhs) == std::get<std::string>(rhs);
    }
    if (std::holds_alternative<Rgb>(lhs) && std::holds_alternative<Rgb>(rhs)) {
        Rgb left = std::get<Rgb>(lhs);
        Rgb right = std::get<Rgb>(rhs);

        return left.red   == right.red
            && left.green == right.green
            && left.blue  == right.blue;
    }
    if (std::holds_alternative<Rgba>(lhs) && std::holds_alternative<Rgba>(rhs)) {
        Rgba left = std::get<Rgba>(lhs);
        Rgba right = std::get<Rgba>(rhs);

        return left.red     == right.red
            && left.green   == right.green
            && left.blue    == right.blue
            && left.opacity == right.opacity;
    }
    return false;
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
        
    RenderAttrs(out);

    out << "/>"sv;
}

// ---------- PolyLine -------------

Polyline& Polyline::AddPoint(Point point) {
    path_.push_back(point);

    ConvertPathToPoints();

    return *this;
}

void Polyline::ConvertPathToPoints() {
    std::ostringstream result;
    bool is_first = true; 

    for (const Point& p : path_) {
        if (!is_first) {
            result << " ";
        }
        is_first = false;
        result << p.x << "," << p.y;
    }

    points_ = std::move(result.str());
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv << points_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// -------------- Text -----------
Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(utils::ReplaceSpecialChars(data));
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"sv;

    RenderAttrs(out);

    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" dx=\""sv
        << offset_.x << "\" dy=\""sv << offset_.y << "\" font-size=\""sv 
        << font_size_ << "\""sv;    
    
    if (font_family_) out << " font-family=\""sv << *font_family_ << "\""sv;
    if (font_weight_) out << " font-weight=\""sv << *font_weight_ << "\""sv;

    out << ">" << data_ << "</text>";
}

// ----------- Document -------------


void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    object_ptrs_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    RenderContext context(out);
    
    std::cerr << "OBJECT PTRS SIZE: " << object_ptrs_.size() << std::endl;

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    for (std::shared_ptr<Object> obj_ptr : object_ptrs_) {
        obj_ptr->Render(context);
    }

    out << "</svg>"sv;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& linecap) {
    switch (linecap) {
    case StrokeLineCap::BUTT:
        out << "butt";
        break;
    case StrokeLineCap::ROUND:
        out << "round";
        break;
    case StrokeLineCap::SQUARE:
        out << "square";
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& join) {
    switch (join) {
    case StrokeLineJoin::ARCS:
        out << "arcs";
        break;
    case StrokeLineJoin::BEVEL:
        out << "bevel";
        break;
    case StrokeLineJoin::MITER:
        out << "miter";
        break;
    case StrokeLineJoin::MITER_CLIP:
        out << "miter-clip";
        break;
    case StrokeLineJoin::ROUND:
        out << "round";
        break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const Color color) {
    if (std::get_if<std::monostate>(&color)) {
        out << "none";
        return out;
    }

    std::visit([&out](auto color_val) {
        out << color_val;
    }, color);
    return out;
}

std::ostream& operator<<(std::ostream& out, const Rgba rgba) {
    out << "rgba(" << static_cast<int>(rgba.red) << "," << static_cast<int>(rgba.green) 
        << "," << static_cast<int>(rgba.blue) << "," << rgba.opacity << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const Rgb rbg) {
    out << "rgb(" << static_cast<int>(rbg.red) << "," << static_cast<int>(rbg.green) 
        << "," << static_cast<int>(rbg.blue) << ")";
    return out;
}


namespace my_tests {

void TestPolyLineRender() {
    {
        std::ostringstream out;
        RenderContext context(out);

        Polyline pline;
        pline.AddPoint({ 0.1, 11.2 })
             .AddPoint({ 1.0, 2.3 })
             .AddPoint({ 5.0, 10.1 });

        pline.Render(context);

        bool test_render = out.str() == "<polyline points=\"0.1,11.2 1,2.3 5,10.1\"/>\n"s;
        assert(test_render);        
    }

    {
        std::ostringstream out;
        RenderContext context(out);
        
        Polyline empty;

        empty.Render(context);

        bool test_empty = out.str() == "<polyline points=\"\"/>\n"s;
        assert(test_empty);
    }
}

void TestTextRender() {
    {
        std::ostringstream out;
        RenderContext context(out);

        Text text;
        text.SetData("& then I\'ve told <her>: \"Hello world\"")
            .SetPosition({ 1.2, 2.213 })
            .SetOffset({ 0, 0 })
            .SetFontSize(12)
            .SetFontFamily("Georgia")
            .SetFontWeight("normal");

        text.Render(context);
        bool test_text_full = out.str() == "<text x=\"1.2\" y=\"2.213\" dx=\"0\" dy=\"0\" font-size=\"12\" font-family=\"Georgia\" font-weight=\"normal\">&amp; then I&apos;ve told &lt;her&gt;: &quot;Hello world&quot;</text>\n";
        assert(test_text_full);

    }
    {
        std::ostringstream out;
        RenderContext context(out);

        Text text;
        text.SetData("& then I\'ve told <her>: \"Hello world\"")
            .SetPosition({ 1.2, 2.213 })
            .SetOffset({ 0, 0 })
            .SetFontSize(12)
            .SetFontWeight("normal");

        text.Render(context);
        bool test_no_font_family = out.str() == "<text x=\"1.2\" y=\"2.213\" dx=\"0\" dy=\"0\" font-size=\"12\" font-weight=\"normal\">&amp; then I&apos;ve told &lt;her&gt;: &quot;Hello world&quot;</text>\n";
        assert(test_no_font_family);
    }
    {
        std::ostringstream out;
        RenderContext context(out);

        Text text;
        text.SetData("& then I\'ve told <her>: \"Hello world\"")
            .SetPosition({ 1.2, 2.213 })
            .SetOffset({ 0, 0 })
            .SetFontSize(12)
            .SetFontFamily("Georgia");


        text.Render(context);
        bool test_no_font_weight = out.str() == "<text x=\"1.2\" y=\"2.213\" dx=\"0\" dy=\"0\" font-size=\"12\" font-family=\"Georgia\">&amp; then I&apos;ve told &lt;her&gt;: &quot;Hello world&quot;</text>\n";
        assert(test_no_font_weight);
    }
    {
        std::ostringstream out;
        RenderContext context(out);

        Text text;
        text.SetData("& then I\'ve told <her>: \"Hello world\"")
            .SetPosition({ 1.2, 2.213 })
            .SetOffset({ 0, 0 })
            .SetFontSize(12);

        text.Render(context);
        bool test_no_weight_and_family = out.str() == "<text x=\"1.2\" y=\"2.213\" dx=\"0\" dy=\"0\" font-size=\"12\">&amp; then I&apos;ve told &lt;her&gt;: &quot;Hello world&quot;</text>\n";


        assert(test_no_weight_and_family);
    }
}

} // namespace svg::my_tests

}  // namespace svg