#pragma once
#include <algorithm>
#include <deque>
#include <memory>
#include <vector>

#include "transport_catalogue.hpp"

namespace handlers {

struct OutputContext {
    
    explicit OutputContext(std::ostream& out): out(out) {}
    
    virtual ~OutputContext() = default;
    
    std::ostream& out;
};

class InputHandler {
public:

    virtual void ExecuteInputQueries() = 0;

    std::deque<domain::BusInputQuery> bus_input_queries_;

    std::deque<domain::StopInputQuery> stop_input_queries_;

};

class OutputHandler {
public:

    virtual void ExecuteOutputQueries(OutputContext& context) const = 0;

    std::deque<domain::StopOutputQuery> stop_output_queries_;

    std::deque<domain::BusOutputQuery> bus_output_queries_;

};

class QueryHandler : public InputHandler, public OutputHandler {
public:

    QueryHandler();

    QueryHandler(const transport_catalogue::TransportCatalogue& tc);

    QueryHandler(std::unique_ptr<transport_catalogue::TransportCatalogue>&& tc_ptr);

    virtual void ExecuteInputQueries();

    virtual void ExecuteOutputQueries(OutputContext& context) const = 0;

    virtual ~QueryHandler() = default;

protected:

    std::shared_ptr<transport_catalogue::TransportCatalogue> catalogue_ptr_;

};

} // namespace handlers

namespace request_handler {



} // namespace request_handler



// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
/*

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};
*/