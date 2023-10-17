#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace svg {

// Color defined in red-green-blue format
struct Rgb {
    Rgb() = default;

    Rgb(const uint8_t r, 
        const uint8_t g, 
        const uint8_t b)
        : red(r), green(g), blue(b) {}

    uint8_t red = 0, 
            green = 0, 
            blue = 0;
};

// Color definded in red-green-blue-alpha format
struct Rgba {
    Rgba() = default;

    Rgba(const uint8_t r, const uint8_t g, 
         const uint8_t b, const double opac)
         : red(r), green(g), blue(b), opacity(opac) {}

    uint8_t red = 0, 
            green = 0, 
            blue = 0;
    double opacity = 1.0;
};

// Color can be defined in different ways in SVG
using Color = std::variant<Rgb, Rgba, std::string, std::monostate>;

// Default no-color value 
const Color NoneColor{"none"};

bool operator==(const Color& lhs, const Color& rhs);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

bool operator==(const Point& lhs, const Point& rhs);
bool operator!=(const Point& lhs, const Point& rhs);

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

namespace utils {

// Replaces special characters with given escape sequences
std::string ReplaceSpecialChars(const std::string& str);

} // namespace svg::utils

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& linecap);

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& join);

std::ostream& operator<<(std::ostream& out, const Color color);

std::ostream& operator<<(std::ostream& out, const Rgb rgb);

std::ostream& operator<<(std::ostream& out, const Rgba rbg);

// Path properties common for many shapes
template <typename Owner>
class PathProps {
public:

    Owner& SetFillColor(const Color& color) {
        fill_color_ = color;

        return AsOwner();
    }

    Owner& SetStrokeColor(const Color& color) {
        stroke_color_ = color;
        
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;

        return AsOwner();
    }

    Owner& SetStrokeLineCap(const StrokeLineCap linecap) {
        stroke_linecap_ = linecap;

        return AsOwner();
    }

    Owner& SetStrokeLineJoin(const StrokeLineJoin linejoin) {
        stroke_linejoin_ = linejoin;

        return AsOwner();
    }

protected:
    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        } 
        if (stroke_linecap_) {
            out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
        }
        if (stroke_linejoin_) {
            out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
        }
    }

    ~PathProps() = default;

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;

    std::optional<Color> stroke_color_;
    
    std::optional<double> stroke_width_;
    
    std::optional<StrokeLineCap> stroke_linecap_;
    
    std::optional<StrokeLineJoin> stroke_linejoin_;

};

/* Auxilary struct used for outputting svg with fancy indents */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;

    int indent_step = 0;
    
    int indent = 0;
};

/* Abstract class Object is used for storing different types of 
 * SVG shapes in one container */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

/* An abstract class for any container that houses SVG-objects */
class ObjectContainer {
public:
    ObjectContainer() = default;

    template <typename ObjectDerivative>
    void Add(const ObjectDerivative& obj);

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
    
    virtual ~ObjectContainer() = default;
protected: 
    
    std::vector<std::shared_ptr<Object>> object_ptrs_;
};

/* An interface for any drawable svg object */
class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

template <typename ObjectDerivative>
void ObjectContainer::Add(const ObjectDerivative& obj) {
    AddPtr(std::make_unique<ObjectDerivative>(obj));
}

/*
 * Class Circle is represents <circle> tag in svg graphics
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object
                   , public PathProps<Circle> {
public:
    Circle() = default;

    Circle(Point center, double radius): center_(center), radius_(radius) {}

    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Class Polyline represents <polyline> tag in svg graphics
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object
                     , public PathProps<Polyline> {
public:
    // Adds a vertex to a polyline
    Polyline& AddPoint(Point point);

private:
    
    void RenderObject(const RenderContext& context) const override;
    
    // Converts a vector of points to a string
    void ConvertPathToPoints();

    std::string points_;

    std::vector<Point> path_;
};

/*
 * Class Text represents <text> tag in svg graphics
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object
                 , public PathProps<Text> {
public:
    Text& SetPosition(Point pos);

    Text& SetOffset(Point offset);

    Text& SetFontSize(uint32_t size);

    Text& SetFontFamily(std::string font_family);

    Text& SetFontWeight(std::string font_weight);

    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_;

    Point offset_;

    uint32_t font_size_ = 1;

    std::optional<std::string> font_family_;

    std::optional<std::string> font_weight_;

    std::string data_;
};

class Document : public ObjectContainer {
public:
    // Adds an Object-derivative class to the document
    void AddPtr(std::unique_ptr<Object>&& obj);

    // Renders an svg-document to the output stream
    void Render(std::ostream& out) const;
};

namespace my_tests {

void TestPolyLineRender();
void TestTextRender();

} // namespace svg::my_tests


}  // namespace svg
