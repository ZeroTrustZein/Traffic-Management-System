import hmac
import logging
from django.http import JsonResponse
from django.conf import settings

logger = logging.getLogger(__name__)

class ApiAuthMiddleware:
    def __init__(self, get_response):
        self.get_response = get_response

    def __call__(self, request):
        if request.path.startswith('/api/'):
            required = getattr(settings, 'API_KEY', '') or ''
            if required:
                key = request.META.get('HTTP_X_API_KEY', '')
                if not key:
                    return JsonResponse({'error': 'Authentication required'}, status=401)
                if not hmac.compare_digest(key, required):
                    return JsonResponse({'error': 'Invalid key'}, status=401)

        return self.get_response(request)
