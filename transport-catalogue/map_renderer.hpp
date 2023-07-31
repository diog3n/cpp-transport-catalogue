#pragma once
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "domain.hpp"
#include "geo.hpp"
#include "svg.hpp"

namespace renderer {

namespace util {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

} // namespace renderer::util

struct RenderSettings {
    double width;
    double height;
    double padding;
    double stop_radius;
    double line_width;
    double bus_label_font_size;
    svg::Point bus_label_offset;
    double stop_label_font_size;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width;
    std::vector<svg::Color> color_palette;
};

class MapRenderer {
public:

    const RenderSettings render_settings;

    MapRenderer() = delete;

    MapRenderer(RenderSettings render_settings); 

    void DrawRoute(const std::string_view bus_name, const std::vector<svg::Point>& points);

    void DrawRouteName(const std::string_view bus_name, const svg::Point& begin, const svg::Point& end);

    void DrawRoundRouteName(const std::string_view bus_name, const svg::Point& begin);

    void DrawStop(const svg::Point& pos);

    void DrawStopName(const std::string_view stop_name, const svg::Point& pos);

    svg::Document GetDoc() const;

private:

    enum UnderlayerTextType {
        STOP, BUS
    };

    std::unordered_map<std::string_view, const svg::Color*> bus_names_to_colors_; 

    svg::Document doc_;

    int color_counter_ = 0;
    
    svg::Circle GetStopCircle(const svg::Point pos) const;

    svg::Text GetStopNameText(const std::string_view stop_name, const svg::Point& pos) const;

    svg::Text GetRouteNameText(const std::string_view bus_names, const svg::Point& pos) const;

    svg::Text GetUnderlayerText(const std::string_view text, const svg::Point& pos, UnderlayerTextType type) const;

    svg::Polyline GetRouteLine(const svg::Color& line_color, const std::vector<svg::Point>& points) const;


};

namespace tests {

void TestSVG();

} // namespace renderer::tests

} // namespace renderer