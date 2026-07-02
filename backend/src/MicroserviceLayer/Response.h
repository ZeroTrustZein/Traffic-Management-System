#pragma once
#include <crow.h>
#include <string>
#include <vector>
#include "../ValidationResult.h"

inline crow::response jsonResponse(int status, crow::json::wvalue body) {
    auto res = crow::response(status, body.dump());
    res.add_header("Content-Type", "application/json");
    return res;
}

inline crow::response jsonError(int status, const std::string& msg) {
    crow::json::wvalue body;
    body["error"] = msg;
    return jsonResponse(status, std::move(body));
}

inline crow::response jsonServerError() {
    return jsonError(500, "Internal server error");
}

inline crow::response jsonFieldError(int status, const std::string& field, const std::string& msg) {
    crow::json::wvalue body;
    body["error"] = msg;
    body["field"] = field;
    return jsonResponse(status, std::move(body));
}

inline crow::response jsonValidationError(int status, const std::vector<ValidationResult>& errors) {
    crow::json::wvalue body;
    body["error"] = "Validation failed";
    if (!errors.empty()) {
        body["field"] = errors[0].fieldName;
    }

    std::vector<crow::json::wvalue> list;
    list.reserve(errors.size());
    for (const auto& e : errors) {
        crow::json::wvalue entry;
        entry["field"] = e.fieldName;
        entry["message"] = e.errorMessage;
        list.push_back(std::move(entry));
    }
    body["validation_errors"] = std::move(list);
    return jsonResponse(status, std::move(body));
}
