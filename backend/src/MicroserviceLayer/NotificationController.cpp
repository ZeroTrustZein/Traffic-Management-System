#include "NotificationController.h"
#include "Response.h"
#include "../Models.h"

crow::response NotificationController::listNotifications(const crow::request& req) {
    try {
        // Optional ?limit=N query param (default 100)
        int limit = 100;
        const auto lp = req.url_params.get("limit");
        if (lp) {
            try {
                limit = std::stoi(lp);
                if (limit < 1 || limit > 500) limit = 100;
            } catch (...) {
                limit = 100;
            }
        }

        auto logs = notifRepo_.findAll(limit);

        std::vector<crow::json::wvalue> items;
        items.reserve(logs.size());
        for (const auto& n : logs) {
            crow::json::wvalue entry;
            entry["id"]       = n.id;
            if (n.ownerId > 0)
                entry["owner_id"] = n.ownerId;
            else
                entry["owner_id"] = nullptr;
            entry["category"] = n.category;
            entry["subject"]  = n.subject;
            entry["message"]  = n.message;
            entry["sent_at"]  = n.sentAt;
            entry["status"]   = n.status;
            items.push_back(std::move(entry));
        }

        crow::json::wvalue res;
        res["count"]         = static_cast<int>(items.size());
        res["notifications"] = std::move(items);
        return jsonResponse(200, std::move(res));

    } catch (const std::exception&) {
        return jsonServerError();
    }
}

