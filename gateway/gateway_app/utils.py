import json as _json
import logging
import requests
from django.conf import settings

logger = logging.getLogger(__name__)


class CrowHttpClient:
    def __init__(self, base_url: str | None = None):
        self.base_url = (base_url or settings.CROW_BASE_URL).rstrip('/')
        self.timeout = 10

    def request(self, method: str, path: str, data: bytes | dict | None = None,
                headers: dict | None = None, query_string: str = '') -> requests.Response:
        url = self.base_url + path
        if query_string:
            url = f'{url}?{query_string}'
        logger.debug('Crow %s %s', method.upper(), url)
        if isinstance(data, dict):
            data = _json.dumps(data).encode()
        return requests.request(method=method.upper(), url=url, data=data,
                                headers=headers, timeout=self.timeout)

    def get(self, path: str) -> requests.Response:
        return self.request('GET', path)

    def post(self, path: str, data: bytes | dict | None = None,
             headers: dict | None = None) -> requests.Response:
        if isinstance(data, dict):
            data = _json.dumps(data).encode()
        h = {'Content-Type': 'application/json'}
        if headers:
            h.update(headers)
        return self.request('POST', path, data=data, headers=h)

    def crow_json(self, path: str, method: str = 'GET',
                  payload: dict | None = None) -> tuple[int, dict]:
        """Returns (status_code, parsed_json)."""
        try:
            if method == 'GET':
                resp = self.get(path)
            else:
                resp = self.post(path, data=payload)
            return resp.status_code, resp.json()
        except requests.exceptions.ConnectionError:
            logger.error('Cannot reach Crow at %s', self.base_url)
            return 503, {'error': 'Crow service unavailable'}
        except Exception:
            logger.exception('Crow request error')
            return 500, {'error': 'Internal gateway error'}

    def list_emergency_vehicles(self) -> tuple[int, dict]:
        return self.crow_json('/api/emergency/vehicles')

    def upsert_emergency_vehicle(self, payload: dict) -> tuple[int, dict]:
        return self.crow_json('/api/emergency/vehicles', method='POST', payload=payload)

    def create_emergency_event(self, payload: dict) -> tuple[int, dict]:
        return self.crow_json('/api/emergency/events', method='POST', payload=payload)

    def get_emergency_event(self, event_id: int) -> tuple[int, dict]:
        return self.crow_json(f'/api/emergency/events/{event_id}')

    def get_emergency_affected_drivers(self, event_id: int) -> tuple[int, dict]:
        return self.crow_json(f'/api/emergency/events/{event_id}/affected-drivers')
