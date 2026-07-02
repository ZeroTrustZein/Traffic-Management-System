from pathlib import Path
import os
import secrets

BASE_DIR = Path(__file__).resolve().parent.parent

SECRET_KEY = os.environ.get('DJANGO_SECRET_KEY') or secrets.token_urlsafe(32)
DEBUG = os.environ.get('DJANGO_DEBUG', 'False') == 'True'
ALLOWED_HOSTS = os.environ.get('ALLOWED_HOSTS', 'localhost,127.0.0.1').split(',')

INSTALLED_APPS = [
    'django.contrib.admin',
    'django.contrib.auth',
    'django.contrib.contenttypes',
    'django.contrib.sessions',
    'django.contrib.messages',
    'django.contrib.staticfiles',
    'gateway_app',
]

MIDDLEWARE = [
    'django.middleware.security.SecurityMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django.middleware.common.CommonMiddleware',
    'gateway_app.middleware.ApiAuthMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware',
    'django.contrib.auth.middleware.AuthenticationMiddleware',
    'django.contrib.messages.middleware.MessageMiddleware',
    'django.middleware.clickjacking.XFrameOptionsMiddleware',
]

ROOT_URLCONF = 'gateway.urls'

TEMPLATES = [
    {
        'BACKEND': 'django.template.backends.django.DjangoTemplates',
        'DIRS': [BASE_DIR / 'templates'],
        'APP_DIRS': True,
        'OPTIONS': {
            'context_processors': [
                'django.template.context_processors.debug',
                'django.template.context_processors.request',
                'django.contrib.auth.context_processors.auth',
                'django.contrib.messages.context_processors.messages',
            ],
        },
    },
]

WSGI_APPLICATION = 'gateway.wsgi.application'

# Django uses SQLite for its own tables (sessions, admin).
# All domain data lives in PostgreSQL and is accessed via Crow.
DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.sqlite3',
        'NAME': BASE_DIR / 'db.sqlite3',
    }
}

LANGUAGE_CODE = 'en-us'
TIME_ZONE = 'UTC'
USE_I18N = True
USE_TZ = True

STATIC_URL = '/static/'
STATICFILES_DIRS = [BASE_DIR / 'static'] if (BASE_DIR / 'static').exists() else []

DEFAULT_AUTO_FIELD = 'django.db.models.BigAutoField'

# Crow service URL
CROW_BASE_URL = os.environ.get('CROW_BASE_URL', 'http://localhost:8080')

API_KEY = os.environ.get('DJANGO_API_KEY') or secrets.token_urlsafe(24)

# Email — console backend for development; switch to smtp for production
EMAIL_BACKEND = os.environ.get(
    'EMAIL_BACKEND', 'django.core.mail.backends.console.EmailBackend'
)
EMAIL_HOST          = os.environ.get('EMAIL_HOST', 'smtp.office365.com')
EMAIL_PORT          = int(os.environ.get('EMAIL_PORT', '587'))
EMAIL_USE_TLS       = os.environ.get('EMAIL_USE_TLS', 'True') == 'True'
EMAIL_HOST_USER     = os.environ.get('EMAIL_HOST_USER', '')
EMAIL_HOST_PASSWORD = os.environ.get('EMAIL_HOST_PASSWORD', '')
DEFAULT_FROM_EMAIL  = os.environ.get('DEFAULT_FROM_EMAIL', 'traffic-dept@example.com')
ADMIN_ALERT_EMAIL   = os.environ.get('ADMIN_ALERT_EMAIL', 'admin@example.com')

# Maps: the default and recommended renderer is Leaflet + OpenStreetMap, which
# requires no API key. Setting GOOGLE_MAPS_API_KEY is *optional* — if present
# the Google Maps Javascript API is used instead. Leave blank to use OSM.
GOOGLE_MAPS_API_KEY = os.environ.get('GOOGLE_MAPS_API_KEY', '')
