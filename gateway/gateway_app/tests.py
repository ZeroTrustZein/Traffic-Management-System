from django.test import TestCase, override_settings
from unittest.mock import Mock, patch


class EmergencyControlTests(TestCase):
    def _mock_crow_json(self, path, method='GET', payload=None):
        if path == '/api/emergency/vehicles':
            return 200, {
                'count': 1,
                'vehicles': [{'id': 1, 'type': 'AMBULANCE', 'identifier': 'AMB-001', 'is_active': True}],
            }
        if path == '/api/junctions':
            return 200, {
                'count': 2,
                'junctions': [
                    {'id': 1, 'name': 'Junction 1', 'location': '', 'speed_limit_kmh': 50, 'is_active': True},
                    {'id': 2, 'name': 'Junction 2', 'location': '', 'speed_limit_kmh': 50, 'is_active': True},
                ],
            }
        if path == '/api/emergency/events' and method == 'POST':
            return 201, {
                'event_id': 10,
                'status': 'ACTIVE',
                'emergency_vehicle_id': 1,
                'start_junction_id': 1,
                'target_junction_id': 2,
                'estimated_minutes': 5.0,
                'route': [1, 2],
            }
        if path == '/api/emergency/events/10':
            return 200, {
                'id': 10,
                'status': 'ACTIVE',
                'emergency_vehicle_id': 1,
                'start_junction_id': 1,
                'target_junction_id': 2,
                'started_at': '2026-01-01 00:00:00',
                'estimated_minutes': 5.0,
                'route': [1, 2],
                'start_junction_name': 'Junction 1',
                'target_junction_name': 'Junction 2',
            }
        if path == '/api/emergency/events/10/affected-drivers':
            return 200, {
                'event_id': 10,
                'count': 1,
                'drivers': [{'owner_id': 1, 'full_name': 'Demo Driver', 'email': 'demo@example.com'}],
            }
        return 404, {'error': 'not found'}

    @patch('gateway_app.web_views.CrowHttpClient.crow_json')
    def test_emergency_page_loads(self, mock_crow_json):
        mock_crow_json.side_effect = self._mock_crow_json
        resp = self.client.get('/emergency/')
        self.assertEqual(resp.status_code, 200)

    @patch('gateway_app.web_views.send_emergency_alert')
    @patch('gateway_app.web_views.CrowHttpClient.crow_json')
    def test_create_event(self, mock_crow_json, mock_send):
        mock_crow_json.side_effect = self._mock_crow_json
        mock_send.return_value = True
        resp = self.client.post('/emergency/', data={
            'action': 'create',
            'emergency_vehicle_id': '1',
            'start_junction_id': '1',
            'target_junction_id': '2',
        })
        self.assertEqual(resp.status_code, 200)


class ApiProxyTests(TestCase):
    @override_settings(API_KEY='traffic-demo-key')
    @patch('gateway_app.api_views.CrowHttpClient.request')
    def test_get_proxy_forwards_json_response(self, mock_request):
        mock_request.return_value = Mock(
            status_code=200,
            content=b'{"count":1,"vehicles":[]}',
            headers={'Content-Type': 'application/json'},
        )

        resp = self.client.get('/api/vehicles?limit=5', HTTP_X_API_KEY='traffic-demo-key')

        self.assertEqual(resp.status_code, 200)
        self.assertJSONEqual(resp.content, {'count': 1, 'vehicles': []})
        mock_request.assert_called_once_with(
            method='GET',
            path='/api/vehicles',
            data=None,
            headers={'Content-Type': 'application/json'},
            query_string='limit=5',
        )

    @override_settings(API_KEY='traffic-demo-key')
    @patch('gateway_app.api_views.CrowHttpClient.request')
    def test_post_proxy_forwards_request_body(self, mock_request):
        mock_request.return_value = Mock(
            status_code=201,
            content=b'{"vehicle_id":7,"number_plate":"TS01TST"}',
            headers={'Content-Type': 'application/json'},
        )

        payload = '{"number_plate":"TS01TST","vehicle_type":"CAR"}'
        resp = self.client.post('/api/vehicles', data=payload, content_type='application/json',
                                HTTP_X_API_KEY='traffic-demo-key')

        self.assertEqual(resp.status_code, 201)
        self.assertJSONEqual(resp.content, {'vehicle_id': 7, 'number_plate': 'TS01TST'})
        mock_request.assert_called_once_with(
            method='POST',
            path='/api/vehicles',
            data=payload.encode(),
            headers={'Content-Type': 'application/json'},
            query_string='',
        )

    @override_settings(API_KEY='secret-key')
    @patch('gateway_app.api_views.CrowHttpClient.request')
    def test_api_proxy_requires_key_when_configured(self, mock_request):
        resp = self.client.get('/api/vehicles')

        self.assertEqual(resp.status_code, 401)
        mock_request.assert_not_called()
