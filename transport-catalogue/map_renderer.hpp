#pragma once
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "domain.hpp"
#include "geo.hpp"
#include "svg.hpp"

namespace renderer {

namespace util {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

/* Sphere projector takes latitude-longitude coordinates of a sphere 
 * and transforms them to the x-y coordinates of a plane */
class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates

    /* points_begin and points_end are iterators of a 
     * container of geo::Coordinates */
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // If there are no points, then just return
        if (points_begin == points_end) {
            return;
        }

        // Finds points with max and min longitude
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Finds points with max and min latitude
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Calculates zoom coefficient for the x axis
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Caclulates zoom coefficient for the y axis.
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // if both coefficients are not zeroes, then use the least of them
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // if one of them is not zero, use it 
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // same here
            zoom_coeff_ = *height_zoom;
        }
    }

    // Projects a geo::Coordinates to svg::Point
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

    // Canvas width
    double width;

    // Canvas height
    double height;

    // Canvas padding
    double padding;

    // Radius of the stop circle on the map
    double stop_radius;

    // Line width of the route lines
    double line_width;

    double bus_label_font_size;

    svg::Point bus_label_offset;

    double stop_label_font_size;

    svg::Point stop_label_offset;

    // Color of the underlayer text
    svg::Color underlayer_color;

    // Line width of the underlayer text
    double underlayer_width;

    // Color palette used to color route lines
    std::vector<svg::Color> color_palette;

};

class MapRenderer {
public:

    // Render settings taken from json. Made public for easy access
    const RenderSettings render_settings;

    // Map renderer should not be created without render_settings
    MapRenderer() = delete;

    MapRenderer(RenderSettings render_settings); 

    // Draws the route line and adds it to the document
    void DrawRoute(const std::string_view bus_name, const std::vector<svg::Point>& points);

    // Draws the route label and adds it to the document
    void DrawRouteName(const std::string_view bus_name, const svg::Point& begin, const svg::Point& end);

    /* Draws the round route label and adds it to the document.
     * Round route label is only displayed once */
    void DrawRoundRouteName(const std::string_view bus_name, const svg::Point& begin);

    // Draws a stop circle and adds it to the document
    void DrawStop(const svg::Point& pos);

    // Draws stop label and adds it to the document
    void DrawStopName(const std::string_view stop_name, const svg::Point& pos);

    svg::Document GetDoc() const;

private:

    enum UnderlayerTextType {
        STOP, BUS
    };

    // Is used to match the colors of routes with colors of their labels
    std::unordered_map<std::string_view, const svg::Color*> bus_names_to_colors_; 

    svg::Document doc_;

    // Is used to cycle through the colors given in color_palette render setting
    int color_counter_ = 0;
    
    // Draws a circle with set parameters
    svg::Circle GetStopCircle(const svg::Point pos) const;

    // Draws a text for the stop name with set parameters
    svg::Text GetStopNameText(const std::string_view stop_name, const svg::Point& pos) const;

    // Draws a text for the route name with set parameters
    svg::Text GetRouteNameText(const std::string_view bus_names, const svg::Point& pos) const;

    // Draws a text for the route or stop underlayer text with set parameters
    svg::Text GetUnderlayerText(const std::string_view text, const svg::Point& pos, UnderlayerTextType type) const;

    // Draws a route line with set parameters
    svg::Polyline GetRouteLine(const svg::Color& line_color, const std::vector<svg::Point>& points) const;

};

namespace tests {

void TestSVG();

} // namespace renderer::tests

} // namespace renderer