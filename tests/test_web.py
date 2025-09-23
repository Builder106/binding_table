import json
import os
import sys

import pytest

# Ensure we can import the web app
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
WEB_DIR = os.path.join(REPO_ROOT, 'web')
sys.path.insert(0, WEB_DIR)
from app import create_app  # noqa: E402


@pytest.fixture()
def client():
    app = create_app()
    app.config.update(TESTING=True)
    return app.test_client()


def test_run_binding_table(client):
    resp = client.post('/run', data={'code': 'int x = 5; x = x + 3;'} )
    assert resp.status_code == 200
    data = resp.get_json()
    assert data['ok'] is True
    assert 'S = {x |-> 5}' in data['stdout'] or 'S = {x |-> 8}' in data['stdout']


def test_run_stack_output(client):
    code = 'int i; int x; i = 4; x = 3; while (i < 7) { x = x + i; i = i + 2; }'
    resp = client.post('/run', data={'code': code})
    assert resp.status_code == 200
    data = resp.get_json()
    assert 'Stack evolution by step:' in data['stdout']
