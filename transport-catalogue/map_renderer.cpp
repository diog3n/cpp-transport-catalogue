#include <algorithm>

#include "map_renderer.hpp"
#include "svg.hpp"

namespace renderer {

namespace util {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

} // namespace renderer::util

MapRenderer::MapRenderer(RenderSettings render_settings)
    : render_settings(render_settings) {}

void MapRenderer::DrawRoute(const std::vector<svg::Point>& points) {
    doc_.Add(std::move(GetRouteLine(points)));
}

svg::Polyline MapRenderer::GetRouteLine(const std::vector<svg::Point>& points) const {
    svg::Polyline route;
    svg::Color stroke_color = render_settings.color_palette.at(color_counter_ % render_settings.color_palette.size());

    route.SetStrokeColor(stroke_color);
    route.SetFillColor(svg::NoneColor);
    route.SetStrokeWidth(render_settings.line_width);
    route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    std::for_each(points.begin(), points.end(), 
        [&route](const svg::Point& point) {
            route.AddPoint(point);
        });

    return route;
}

svg::Document MapRenderer::GetDoc() const {
    return doc_;
}

namespace tests {

} // namespace renderer::tests

} // namespace renderer