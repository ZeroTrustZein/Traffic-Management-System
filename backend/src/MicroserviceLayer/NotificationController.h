#pragma once
#include <crow.h>
#include "../DataAccessLayer/NotificationRepository.h"

// W13/W14/W15: exposes the notification_logs audit trail via GET /api/notifications.
class NotificationController {
public:
    crow::response listNotifications(const crow::request& req);

private:
    NotificationRepository notifRepo_;
};
