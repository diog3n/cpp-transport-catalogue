#include <algorithm>
#include <cstdint>

#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace renderer {

using namespace std::literals;

namespace util {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

SphereProjector MakeSphereProjector(const transport_catalogue::TransportCatalogue& catalogue, const MapRenderer& renderer) {
    std::vector<std::string_view> stop_names = catalogue.GetStopNames();
    std::vector<geo::Coordinates> coordinates;
    std::for_each(stop_names.begin(), stop_names.end(), 
        [&catalogue, &coordinates](std::string_view stop_name) {
            const domain::Stop& stop = *catalogue.FindStop(stop_name);
            if (!stop.buses.empty()) {
                coordinates.push_back(stop.coordinates);
            }
        });

    return renderer::util::SphereProjector(
                           coordinates.begin(), 
                           coordinates.end(), 
                           renderer.render_settings.width, 
                           renderer.render_settings.height, 
                           renderer.render_settings.padding);
}

CoordinatesTransformer::CoordinatesTransformer(
            const transport_catalogue::TransportCatalogue& catalogue, 
            const MapRenderer& renderer)
    : projector_(MakeSphereProjector(catalogue, renderer)) {}

// Collects stop points for a given bus
std::vector<svg::Point> CoordinatesTransformer::TransformRouteCoords(
            const transport_catalogue::TransportCatalogue& catalogue, 
            const std::string_view bus_name) const {
    const domain::Bus& bus = *catalogue.FindBus(bus_name);

    std::vector<svg::Point> points(bus.route.size());

    std::transform(bus.route.begin(), bus.route.end(), points.begin(),
        [this, &catalogue](const domain::StopPtr stop_ptr) {
            return TransformStopCoords(catalogue, stop_ptr->name);
        });

    return points;
}

// Projects stop geo coordinates onto a plane
svg::Point CoordinatesTransformer::TransformStopCoords(
            const transport_catalogue::TransportCatalogue& catalogue, 
            const std::string_view stop_name) const {
    const domain::Stop& stop = *catalogue.FindStop(stop_name);
    return projector_(stop.coordinates);
}

} // namespace renderer::util

MapRenderer::MapRenderer(RenderSettings render_settings)
    : render_settings(render_settings) {}

void MapRenderer::DrawRoute(std::string_view bus_name, 
                            const std::vector<svg::Point>& points) {
    const svg::Color& line_color = render_settings.color_palette.at(color_counter_ % render_settings.color_palette.size());
    if (points.empty()) return; 
    
    doc_.Add(std::move(GetRouteLine(line_color, points)));
    bus_names_to_colors_[bus_name] = &line_color;
    color_counter_++;
}


void MapRenderer::DrawRouteName(const std::string_view bus_name, 
                                const svg::Point& begin, 
                                const svg::Point& end) {
    doc_.Add(GetUnderlayerText(bus_name, begin, BUS));
    doc_.Add(GetRouteNameText(bus_name, begin));
    doc_.Add(GetUnderlayerText(bus_name, end, BUS));
    doc_.Add(GetRouteNameText(bus_name, end));
}

void MapRenderer::DrawRoundRouteName(const std::string_view bus_name, 
                                     const svg::Point& begin) {
    doc_.Add(GetUnderlayerText(bus_name, begin, BUS));
    doc_.Add(GetRouteNameText(bus_name, begin));
}

void MapRenderer::DrawStopName(const std::string_view stop_name, 
                               const svg::Point& pos) {
    doc_.Add(GetUnderlayerText(stop_name, pos, STOP));
    doc_.Add(GetStopNameText(stop_name, pos));
}

svg::Text MapRenderer::GetStopNameText(const std::string_view stop_name,
                                       const svg::Point& pos) const {
    svg::Text stop_name_text;
    stop_name_text.SetOffset(render_settings.stop_label_offset)
                  .SetPosition(pos)
                  .SetFontFamily("Verdana"s)
                  .SetData(std::string(stop_name))
                  .SetFillColor("black"s)
                  .SetFontSize(render_settings.stop_label_font_size);

    return stop_name_text;
}

svg::Polyline MapRenderer::GetRouteLine(
                            const svg::Color& line_color, 
                            const std::vector<svg::Point>& points) const {
    svg::Polyline route;

    route.SetStrokeColor(line_color)
         .SetFillColor(svg::NoneColor)
         .SetStrokeWidth(render_settings.line_width)
         .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
         .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    std::for_each(points.begin(), points.end(), 
        [&route](const svg::Point& point) {
            route.AddPoint(point);
        });

    return route;
}

svg::Text MapRenderer::GetRouteNameText(const std::string_view bus_name,
                                        const svg::Point& pos) const {
    svg::Text route_name;

    route_name.SetData(std::string(bus_name))
              .SetPosition(pos)
              .SetOffset(render_settings.bus_label_offset)
              .SetFontFamily("Verdana"s)
              .SetFontSize(render_settings.bus_label_font_size)
              .SetFontWeight("bold"s)
              .SetFillColor(*bus_names_to_colors_.at(bus_name));

    return route_name;
}

svg::Text MapRenderer::GetUnderlayerText(const std::string_view text, 
                                         const svg::Point& pos,
                                         UnderlayerTextType type) const {
    svg::Text underlayer_text;

    svg::Point offset = type == BUS 
                        ? render_settings.bus_label_offset 
                        : render_settings.stop_label_offset;

    uint32_t label_size = type == BUS
                          ? render_settings.bus_label_font_size
                          : render_settings.stop_label_font_size;

    underlayer_text.SetData(std::string(text))
                   .SetPosition(pos)
                   .SetOffset(offset)
                   .SetFontFamily("Verdana"s)
                   .SetFontSize(label_size)
                   .SetFillColor(render_settings.underlayer_color)
                   .SetStrokeWidth(render_settings.underlayer_width)
                   .SetStrokeColor(render_settings.underlayer_color)
                   .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                   .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    if (type == BUS) underlayer_text.SetFontWeight("bold"s);

    return underlayer_text;
}

svg::Circle MapRenderer::GetStopCircle(const svg::Point pos) const {
    svg::Circle stop_circle;

    stop_circle.SetCenter(pos)
               .SetRadius(render_settings.stop_radius)
               .SetFillColor("white");

    return stop_circle;
}

void MapRenderer::DrawStop(const svg::Point& pos) {
    doc_.Add(GetStopCircle(pos));
}

svg::Document MapRenderer::RenderMap(const transport_catalogue::TransportCatalogue& catalogue) {
    renderer::util::CoordinatesTransformer transformer(catalogue, *this);
    std::vector<std::string_view> bus_names = catalogue.GetBusNames();
    std::vector<std::string_view> stop_names = catalogue.GetStopNames();

    std::map<std::string_view, std::vector<svg::Point>> bus_names_to_points;

    // Rendering bus route lines
    std::for_each(bus_names.begin(), bus_names.end(), 
        [this, &transformer, &bus_names_to_points, &catalogue](std::string_view bus_name) {
            std::vector<svg::Point> points = transformer.TransformRouteCoords(catalogue, bus_name);
            DrawRoute(bus_name, points);
            bus_names_to_points[bus_name] = std::move(points);
        });

    // Rendering bus route names
    std::for_each(bus_names_to_points.begin(), bus_names_to_points.end(), 
        [this, &catalogue](const std::pair<std::string_view, std::vector<svg::Point>>& bus_name_to_points) {
            const auto& [bus_name, points] = bus_name_to_points;
                
            const domain::Bus& bus = *catalogue.FindBus(bus_name);

            if (bus.route.empty()) return;

            if (bus.is_roundtrip) {
                DrawRoundRouteName(bus_name, points.front());
            } else {
                svg::Point midpoint = points.at(points.size() / 2);

                if (midpoint == points.front()) {
                    DrawRoundRouteName(bus_name, points.front());
                } else {
                    DrawRouteName(bus_name, points.front(), midpoint);
                }
            }
        });

    // Rendering stop circles
    std::for_each(stop_names.begin(), stop_names.end(), 
        [this, &transformer, &catalogue](const std::string_view& stop_name) {
            const domain::Stop& stop = *catalogue.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = transformer.TransformStopCoords(catalogue, stop_name);

            DrawStop(pos);
        });

    // Rendering stop names
    std::for_each(stop_names.begin(), stop_names.end(),
        [this, &transformer, &catalogue](const std::string_view& stop_name) {
            const domain::Stop& stop = *catalogue.FindStop(stop_name);

            if (stop.buses.empty()) return;

            svg::Point pos = transformer.TransformStopCoords(catalogue, stop_name);

            DrawStopName(stop_name, pos);
        });

    return doc_;

}

svg::Document MapRenderer::GetDoc() const {
    return doc_;
}

namespace tests {

} // namespace renderer::tests

} // namespace renderer