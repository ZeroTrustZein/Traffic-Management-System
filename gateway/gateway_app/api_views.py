from django.http import HttpResponse, JsonResponse
from django.views.decorators.csrf import csrf_exempt
import requests

from .utils import CrowHttpClient


@csrf_exempt
def proxy_api(request, api_path: str = ''):
    client = CrowHttpClient()
    upstream_path = f'/api/{api_path}' if api_path else '/api/'
    content_type = request.headers.get('Content-Type', 'application/json')

    try:
        response = client.request(
            method=request.method,
            path=upstream_path,
            data=request.body or None,
            headers={'Content-Type': content_type},
            query_string=request.META.get('QUERY_STRING', ''),
        )
    except requests.exceptions.ConnectionError:
        return JsonResponse({'error': 'Crow service unavailable'}, status=503)
    except requests.exceptions.RequestException:
        return JsonResponse({'error': 'Crow request failed'}, status=502)

    return HttpResponse(
        response.content,
        status=response.status_code,
        content_type=response.headers.get('Content-Type', 'application/json'),
    )
