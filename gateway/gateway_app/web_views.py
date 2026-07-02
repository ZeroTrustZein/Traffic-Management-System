"""
Web UI views — Django-rendered HTML pages.
Each view calls the Crow backend via CrowHttpClient and renders a template.
"""
import json
import logging
from django.conf import settings
from django.shortcuts import render, redirect
from django.contrib import messages
from .utils import CrowHttpClient
from .email_service import send_fine_notification, send_congestion_alert, send_emergency_alert

logger = logging.getLogger(__name__)


def _crow():
    return CrowHttpClient()


def home(request):
    return render(request, 'home.html', {
        'api_base_url': '/api/',
        'optional_pages': [
            {'name': 'Vehicles', 'url': 'vehicle_list'},
            {'name': 'Plate Logs', 'url': 'plate_logs'},
            {'name': 'Violations', 'url': 'violations'},
            {'name': 'Fines', 'url': 'fines'},
            {'name': 'Traffic Flow', 'url': 'traffic_flow'},
            {'name': 'Congestion', 'url': 'congestion'},
            {'name': 'Map', 'url': 'traffic_map'},
            {'name': 'Route Guidance', 'url': 'route_guidance'},
            {'name': 'Emergency', 'url': 'emergency_control'},
        ],
    })


def vehicle_list(request):
    client = _crow()
    _, data = client.crow_json('/api/vehicles')
    vehicles = data.get('vehicles', [])
    query = request.GET.get('q', '').upper().strip()
    if query:
        vehicles = [v for v in vehicles if query in v.get('number_plate', '')]
    return render(request, 'vehicles/list.html', {
        'vehicles': vehicles,
        'query': query,
    })


def vehicle_register(request):
    client = _crow()
    if request.method == 'POST':
        payload = {
            'number_plate': request.POST.get('number_plate', '').upper().strip(),
            'vehicle_type': request.POST.get('vehicle_type', 'CAR'),
            'owner_name':   request.POST.get('owner_name', '').strip(),
            'owner_email':  request.POST.get('owner_email', '').strip(),
            'owner_phone':  request.POST.get('owner_phone', '').strip(),
        }
        status, data = client.crow_json('/api/vehicles', method='POST', payload=payload)
        if status == 201:
            messages.success(request, f"Vehicle {payload['number_plate']} registered successfully.")
            return redirect('vehicle_list')
        else:
            messages.error(request, data.get('error', 'Registration failed.'))

    _, junctions = client.crow_json('/api/junctions')
    return render(request, 'vehicles/register.html', {
        'vehicle_types': ['CAR', 'TRUCK', 'MOTORCYCLE', 'BUS', 'EMERGENCY'],
        'junctions': junctions.get('junctions', []),
    })


def vehicle_detail(request, plate):
    client = _crow()

    if request.method == 'POST' and 'log_junction' in request.POST:
        jid = request.POST.get('junction_id')
        event_type = request.POST.get('event_type', 'PASSAGE')
        speed = request.POST.get('speed_kmh', '').strip()
        payload: dict = {
            'number_plate': plate,
            'event_type':   event_type,
        }
        if speed:
            try:
                payload['speed_kmh'] = float(speed)
            except ValueError:
                pass
        status, data = client.crow_json(f'/api/junctions/{jid}/log', method='POST', payload=payload)
        if status == 200:
            messages.success(request, f"Plate logged at junction {jid}.")
        else:
            messages.error(request, data.get('error', 'Logging failed.'))
        return redirect('vehicle_detail', plate=plate)

    _, veh_data = client.crow_json(f'/api/vehicles/{plate}')
    if 'error' in veh_data:
        messages.error(request, f"Vehicle {plate} not found.")
        return redirect('vehicle_list')

    _, violations_data = client.crow_json('/api/violations')
    vehicle_violations = [
        v for v in violations_data.get('violations', [])
        if v.get('vehicle_id') == veh_data.get('id')
    ]

    _, junctions_data = client.crow_json('/api/junctions')

    return render(request, 'vehicles/detail.html', {
        'vehicle':    veh_data,
        'violations': vehicle_violations,
        'junctions':  junctions_data.get('junctions', []),
    })


def plate_logs(request):
    client = _crow()
    jid = request.GET.get('junction_id', '')
    if jid:
        _, data = client.crow_json(f'/api/junctions/{jid}/logs')
        logs = data.get('logs', [])
    else:
        _, data = client.crow_json('/api/plate-logs')
        logs = data.get('logs', [])

    _, junctions_data = client.crow_json('/api/junctions')
    return render(request, 'plate_logs/list.html', {
        'logs':         logs,
        'junctions':    junctions_data.get('junctions', []),
        'selected_jid': jid,
    })


def violations(request):
    client = _crow()

    if request.method == 'POST' and 'detect' in request.POST:
        jid        = request.POST.get('junction_id')
        hours_back = request.POST.get('hours_back', 1)
        try:
            payload = {'junction_id': int(jid), 'hours_back': int(hours_back)}
        except (TypeError, ValueError):
            messages.error(request, 'Junction and hours-back must be numbers.')
            return redirect('violations')
        status, data = client.crow_json('/api/violations/detect', method='POST', payload=payload)
        if status == 200:
            created = data.get('violations_created', 0)
            messages.success(request, f"{created} violation(s) detected and fines issued.")
            # Send fine notifications for newly issued fines
            for result in data.get('results', []):
                fid   = result.get('fine_id')
                plate = result.get('number_plate', '')
                if plate:
                    _, veh = client.crow_json(f'/api/vehicles/{plate}')
                    owner = veh.get('owner', {})
                    if owner.get('email'):
                        send_fine_notification(
                            owner_email=owner['email'],
                            owner_name=owner.get('full_name', 'Driver'),
                            number_plate=plate,
                            violation=result.get('violation', ''),
                            amount=result.get('fine_amount', 0),
                            fine_id=fid,
                        )
        else:
            messages.error(request, data.get('error', 'Detection failed.'))

    _, violations_data = client.crow_json('/api/violations')
    _, junctions_data  = client.crow_json('/api/junctions')
    _, types_data      = client.crow_json('/api/violation-types')

    return render(request, 'violations/list.html', {
        'violations':      violations_data.get('violations', []),
        'junctions':       junctions_data.get('junctions', []),
        'violation_types': types_data.get('types', []),
    })


def fines(request):
    client = _crow()

    if request.method == 'POST':
        fid    = request.POST.get('fine_id')
        action = request.POST.get('action')
        if action == 'pay':
            status, data = client.crow_json(f'/api/fines/{fid}/pay', method='POST')
            if status == 200:
                messages.success(request, f"Fine #{fid} marked as paid.")
                # Send fine paid notification (best-effort)
                _notify_fine_paid(fid, client)
            else:
                messages.error(request, data.get('error', 'Payment failed.'))
        elif action == 'cancel':
            status, data = client.crow_json(f'/api/fines/{fid}/cancel', method='POST')
            if status == 200:
                messages.success(request, f"Fine #{fid} cancelled.")
            else:
                messages.error(request, data.get('error', 'Cancellation failed.'))

    _, fines_data = client.crow_json('/api/fines')
    return render(request, 'fines/list.html', {
        'fines': fines_data.get('fines', []),
    })


def traffic_flow(request):
    client = _crow()
    _, flow_data       = client.crow_json('/api/traffic/flow')
    _, hourly_data     = client.crow_json('/api/traffic/flow/hourly')
    _, junctions_data  = client.crow_json('/api/junctions')
    _, congestion_data = client.crow_json('/api/traffic/congestion')

    flow_junctions   = flow_data.get('junctions', [])
    hourly_junctions = hourly_data.get('junctions', [])
    junctions_list   = junctions_data.get('junctions', [])
    return render(request, 'traffic/flow.html', {
        'flow_data_json':   json.dumps(flow_junctions),
        'hourly_data_json': json.dumps(hourly_junctions),
        'junctions':        junctions_list,
        'congestion_records': congestion_data.get('records', [])[:20],
    })


def congestion(request):
    client = _crow()

    if request.method == 'POST' and 'analyze' in request.POST:
        jid   = request.POST.get('junction_id')
        hours = request.POST.get('window_hours', 1)
        try:
            payload = {'junction_id': int(jid), 'window_hours': int(hours)}
        except (TypeError, ValueError):
            messages.error(request, 'Junction and window hours must be numbers.')
            return redirect('congestion')
        status, data = client.crow_json('/api/traffic/analyze', method='POST', payload=payload)
        if status == 200:
            level = data.get('congestion_level', 'LOW')
            count = data.get('vehicle_count', 0)
            messages.success(
                request,
                f"Analysis complete: {count} vehicles, level = {level}."
            )
            if level in ('HIGH', 'SEVERE'):
                _dispatch_congestion_alerts(int(jid), level, client)
                messages.warning(
                    request,
                    f"Congestion is {level} — alert emails dispatched to affected drivers."
                )
        else:
            messages.error(request, data.get('error', 'Analysis failed.'))

    _, prone_data       = client.crow_json('/api/traffic/congestion-prone')
    _, junctions_data   = client.crow_json('/api/junctions')
    _, congestion_data  = client.crow_json('/api/traffic/congestion')

    # Predictions for each junction (keyed by string ID for JSON compatibility)
    predictions = {}
    junctions_list = junctions_data.get('junctions', [])
    for j in junctions_list:
        _, pred = client.crow_json(f'/api/traffic/predict/{j["id"]}')
        predictions[str(j['id'])] = pred

    prone_list = prone_data.get('junctions', [])
    return render(request, 'traffic/congestion.html', {
        'prone_junctions':      prone_list,
        'prone_junctions_json': json.dumps(prone_list),
        'junctions':            junctions_list,
        'junctions_json':       json.dumps(junctions_list),
        'congestion_records':   congestion_data.get('records', [])[:30],
        'predictions_json':     json.dumps(predictions),
    })


def traffic_map(request):
    """Map view: Google Maps if GOOGLE_MAPS_API_KEY is set, Leaflet otherwise."""
    client = _crow()
    _, network = client.crow_json('/api/traffic/network')
    junctions = network.get('junctions', [])
    roads     = network.get('roads', [])
    api_key   = getattr(settings, 'GOOGLE_MAPS_API_KEY', '') or ''
    return render(request, 'traffic/map.html', {
        'junctions_json':       json.dumps(junctions),
        'roads_json':           json.dumps(roads),
        'google_maps_api_key':  api_key,
        'use_google_maps':      bool(api_key),
        'junction_count':       len(junctions),
    })


def route_guidance(request):
    """Compute a congestion-aware route between two junctions."""
    client = _crow()
    _, junctions_data = client.crow_json('/api/junctions')
    junctions = junctions_data.get('junctions', [])

    route = None
    error = None
    if request.method == 'POST':
        try:
            from_id = int(request.POST.get('from_junction_id') or 0)
            to_id   = int(request.POST.get('to_junction_id') or 0)
        except ValueError:
            from_id = to_id = 0
        if from_id <= 0 or to_id <= 0 or from_id == to_id:
            error = 'Choose two different junctions.'
        else:
            status, data = client.crow_json(
                '/api/traffic/route', method='POST',
                payload={'from_junction_id': from_id, 'to_junction_id': to_id},
            )
            if status == 200:
                route = data
            else:
                error = data.get('error', 'Route lookup failed.')

    api_key = getattr(settings, 'GOOGLE_MAPS_API_KEY', '') or ''
    return render(request, 'traffic/route.html', {
        'junctions':            junctions,
        'junctions_json':       json.dumps(junctions),
        'route':                route,
        'route_json':           json.dumps(route) if route else 'null',
        'error':                error,
        'google_maps_api_key':  api_key,
        'use_google_maps':      bool(api_key),
    })


def emergency_control(request):
    client = _crow()
    _, ev_resp = client.crow_json('/api/emergency/vehicles')
    _, junctions_resp = client.crow_json('/api/junctions')

    emergency_vehicles = ev_resp.get('vehicles', [])
    junctions = junctions_resp.get('junctions', [])
    by_jid = {j.get('id'): j for j in junctions if j.get('id') is not None}

    event = None
    drivers = []

    if request.method == 'POST':
        action = request.POST.get('action')
        if action == 'create':
            try:
                payload = {
                    'emergency_vehicle_id': int(request.POST.get('emergency_vehicle_id') or 0),
                    'start_junction_id': int(request.POST.get('start_junction_id') or 0),
                    'target_junction_id': int(request.POST.get('target_junction_id') or 0),
                }
            except ValueError:
                payload = {'emergency_vehicle_id': 0, 'start_junction_id': 0, 'target_junction_id': 0}
            status, data = client.crow_json('/api/emergency/events', method='POST', payload=payload)
            if status == 201:
                event = data
                if 'id' not in event and event.get('event_id'):
                    event['id'] = event.get('event_id')
                if not event.get('start_junction_name') and payload.get('start_junction_id'):
                    event['start_junction_name'] = by_jid.get(payload['start_junction_id'], {}).get(
                        'name', f"Junction {payload['start_junction_id']}"
                    )
                if not event.get('target_junction_name') and payload.get('target_junction_id'):
                    event['target_junction_name'] = by_jid.get(payload['target_junction_id'], {}).get(
                        'name', f"Junction {payload['target_junction_id']}"
                    )
                messages.success(request, f"Emergency event #{data.get('event_id')} created.")
            else:
                messages.error(request, data.get('error', 'Emergency event failed.'))
        elif action == 'notify':
            event_id = request.POST.get('event_id')
            if not event_id:
                messages.error(request, 'Missing event id.')
            else:
                status_e, ev_data = client.crow_json(f'/api/emergency/events/{event_id}')
                status_d, drivers_data = client.crow_json(f'/api/emergency/events/{event_id}/affected-drivers')
                if status_e != 200:
                    messages.error(request, ev_data.get('error', 'Cannot load event.'))
                elif status_d != 200:
                    messages.error(request, drivers_data.get('error', 'Cannot load affected drivers.'))
                else:
                    event = ev_data
                    drivers = drivers_data.get('drivers', [])
                    route_ids = event.get('route', []) or []
                    route_names = [
                        by_jid.get(jid, {}).get('name', f'Junction {jid}')
                        for jid in route_ids
                    ]
                    vehicle_id = event.get('emergency_vehicle_id')
                    vehicle_label = next(
                        (v.get('identifier') for v in emergency_vehicles if v.get('id') == vehicle_id),
                        f'#{vehicle_id}'
                    )
                    start_name = event.get('start_junction_name', '')
                    target_name = event.get('target_junction_name', '')
                    sent = 0
                    for d in drivers:
                        if not d.get('email'):
                            continue
                        ok = send_emergency_alert(
                            owner_email=d['email'],
                            owner_name=d.get('full_name', 'Driver'),
                            emergency_vehicle=vehicle_label,
                            start_junction=start_name,
                            target_junction=target_name,
                            route_steps=route_names,
                        )
                        if ok:
                            sent += 1
                    messages.warning(request, f"Notifications sent to {sent} driver(s).")

    return render(request, 'emergency/control.html', {
        'emergency_vehicles': emergency_vehicles,
        'junctions': junctions,
        'event': event,
        'drivers': drivers,
    })


def _dispatch_congestion_alerts(jid: int, level: str, client: CrowHttpClient):
    """Send congestion alert emails with alternative routes to all drivers recently seen at the junction."""
    try:
        _, junctions_resp = client.crow_json('/api/junctions')
        junction_name = next(
            (j['name'] for j in junctions_resp.get('junctions', []) if j['id'] == jid),
            f'Junction {jid}'
        )
        _, routes_data = client.crow_json(f'/api/traffic/routes/{jid}')
        routes = routes_data.get('routes', [])

        _, logs_data = client.crow_json(f'/api/junctions/{jid}/logs')
        logs = logs_data.get('logs', [])

        notified: set = set()
        for log in logs:
            vid = log.get('vehicle_id', 0)
            if not vid or vid in notified:
                continue
            notified.add(vid)
            plate = log.get('number_plate', '')
            if not plate:
                continue
            _, veh = client.crow_json(f'/api/vehicles/{plate}')
            owner = veh.get('owner', {})
            if not owner.get('email'):
                continue
            send_congestion_alert(
                owner_email=owner['email'],
                owner_name=owner.get('full_name', 'Driver'),
                junction_name=junction_name,
                congestion_level=level,
                alternative_routes=routes,
            )
    except Exception as e:
        logger.error('Congestion email dispatch failed: %s', e)


def _notify_fine_paid(fine_id: int, client: CrowHttpClient):
    """Best-effort: send a payment acknowledgement email."""
    try:
        _, fine = client.crow_json(f'/api/fines/{fine_id}')
        vid = fine.get('violation_id')
        if not vid:
            return
        _, violation = client.crow_json(f'/api/violations/{vid}')
        vehicle_id = violation.get('vehicle_id')
        if not vehicle_id:
            return
        _, vehicles_data = client.crow_json('/api/vehicles')
        vehicle = next(
            (v for v in vehicles_data.get('vehicles', []) if v['id'] == vehicle_id), None
        )
        if not vehicle:
            return
        plate = vehicle.get('number_plate', '')
        if not plate:
            return
        _, veh = client.crow_json(f'/api/vehicles/{plate}')
        owner = veh.get('owner', {})
        if owner.get('email'):
            from .email_service import _send
            _send(
                subject=f"[Traffic Department] Fine #{fine_id} Payment Received",
                message=(
                    f"Dear {owner.get('full_name', 'Driver')},\n\n"
                    f"We confirm receipt of your payment for fine #{fine_id}.\n"
                    f"No further action is required.\n\n"
                    f"Traffic Management Department"
                ),
                recipient=owner['email'],
            )
    except Exception as e:
        logger.debug('Fine paid email skipped: %s', e)
