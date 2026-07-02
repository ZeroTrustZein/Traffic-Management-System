"""
Email notification service.

Development: EMAIL_BACKEND = console (emails printed to terminal).
Production: set EMAIL_BACKEND = smtp and configure SMTP settings in settings.py.
"""
import logging
from django.core.mail import send_mail
from django.conf import settings

logger = logging.getLogger(__name__)


def _redact_recipient(recipient: str) -> str:
    if '@' not in recipient:
        return 'redacted'
    local_part, domain = recipient.split('@', 1)
    prefix = local_part[:2] if len(local_part) >= 2 else local_part[:1]
    return f'{prefix}***@{domain}'


def send_fine_notification(owner_email: str, owner_name: str,
                            number_plate: str, violation: str,
                            amount: float, fine_id: int) -> bool:
    subject = f"[Traffic Department] Fine Issued — {number_plate}"
    message = (
        f"Dear {owner_name},\n\n"
        f"A traffic fine has been issued for your vehicle ({number_plate}).\n\n"
        f"Violation:  {violation}\n"
        f"Amount due: EUR {amount:.2f}\n"
        f"Fine ID:    #{fine_id}\n"
        f"Due date:   30 days from today\n\n"
        f"Please pay at your nearest traffic department or via the online portal.\n\n"
        f"Traffic Management Department"
    )
    return _send(subject, message, owner_email)


def send_congestion_alert(owner_email: str, owner_name: str,
                           junction_name: str, congestion_level: str,
                           alternative_routes: list[dict]) -> bool:
    subject = f"[Traffic Alert] Congestion at {junction_name}"
    route_lines = "\n".join(
        f"  • {r.get('via_description', '')} (~{r.get('estimated_time_minutes', '?')} min)"
        for r in alternative_routes
    ) or "  No alternatives currently available."

    message = (
        f"Dear {owner_name},\n\n"
        f"Congestion level at {junction_name} is currently {congestion_level}.\n\n"
        f"Suggested alternative routes:\n{route_lines}\n\n"
        f"Please consider these alternatives to avoid delays.\n\n"
        f"Traffic Management Department"
    )
    return _send(subject, message, owner_email)


def send_emergency_alert(owner_email: str, owner_name: str,
                         emergency_vehicle: str,
                         start_junction: str, target_junction: str,
                         route_steps: list[str]) -> bool:
    subject = f"[Traffic Alert] Emergency vehicle on route — {emergency_vehicle}"
    steps = "\n".join(f"  • {s}" for s in route_steps) or "  Route information unavailable."
    message = (
        f"Dear {owner_name},\n\n"
        f"An emergency vehicle ({emergency_vehicle}) is currently travelling through the city.\n\n"
        f"Route:\n"
        f"Start: {start_junction}\n"
        f"Target: {target_junction}\n"
        f"Steps:\n{steps}\n\n"
        f"Please drive carefully and consider avoiding the route if possible.\n\n"
        f"Traffic Management Department"
    )
    return _send(subject, message, owner_email)


def _send(subject: str, message: str, recipient: str) -> bool:
    try:
        send_mail(
            subject=subject,
            message=message,
            from_email=settings.DEFAULT_FROM_EMAIL,
            recipient_list=[recipient],
            fail_silently=False,
        )
        logger.info('Email sent successfully to %s', _redact_recipient(recipient))
        return True
    except Exception:
        logger.exception('Email delivery failed for %s', _redact_recipient(recipient))
        return False
